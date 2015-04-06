//
// This file is part of lighstd.
//
// lighstd is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// lighstd is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with lighstd.  If not, see <http://www.gnu.org/licenses/>.

#include <sys/queue.h>
#include <sys/tree.h>
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <event2/bufferevent.h>
#include <event2/util.h>

#include "lifx/wire_proto.h"
#include "jsmn.h"
#include "client.h"
#include "jsonrpc.h"
#include "proto.h"
#include "lightsd.h"

static bool
lgtd_jsonrpc_type_integer(const jsmntok_t *t, const char *json)
{
    if (t->type != JSMN_PRIMITIVE) {
        return false;
    }

    const char *endptr = NULL;
    errno = 0;
    strtol(&json[t->start], (char **)&endptr, 10);
    return endptr == json + t->end && errno != ERANGE;
}

static bool
lgtd_jsonrpc_type_float_between_0_and_1(const jsmntok_t *t,
                                        const char *json)
{
    if (t->type != JSMN_PRIMITIVE) {
        return false;
    }

    int i = t->start;
    bool dot_seen = false;
    bool first_digit_is_one = false;
    for (; i < t->end; i++) {
        if (json[i] == '.') {
            if (dot_seen) {
                return false;
            }
            dot_seen = true;
        } else if (dot_seen) {
            if (json[i] < '0' || json[i] > '9') {
                return false;
            } if (first_digit_is_one && json[i] != '0') {
                return false;
            }
        } else {
            if (first_digit_is_one) {
                return false;
            } else if (json[i] == '1') {
                first_digit_is_one = true;
            } else if (json[i] != '0') {
                return false;
            }
        }
    }

    return true;
}

static bool
lgtd_jsonrpc_type_float_between_0_and_360(const jsmntok_t *t,
                                          const char *json)
{
    if (t->type != JSMN_PRIMITIVE) {
        return false;
    }

    char c = json[t->start];
    if (c != '-' && (c > '9' || c < '0')) {
        return false;
    }

    const char *endptr = NULL;
    errno = 0;
    long intpart = strtol(&json[t->start], (char **)&endptr, 10);
    if ((endptr != json + t->end && *endptr != '.')
        || errno == ERANGE || intpart < 0 || intpart > 360
        || (intpart == 0 && c == '-')) {
        return false;
    }
    if (endptr == json + t->end) {
        return true;
    }
    long fracpart = strtol(++endptr, (char **)&endptr, 10);
    return endptr == json + t->end && errno != ERANGE
        && fracpart >= 0 && (intpart < 360 || fracpart == 0);
}

static bool
lgtd_jsonrpc_type_number(const jsmntok_t *t, const char *json)
{
    if (t->type != JSMN_PRIMITIVE) {
        return false;
    }

    char c = json[t->start];
    return c == '-' || (c >= '0' && c <= '9');
}

static bool __attribute__((unused))
lgtd_jsonrpc_type_bool(const jsmntok_t *t, const char *json)
{
    if (t->type != JSMN_PRIMITIVE) {
        return false;
    }

    char c = json[t->start];
    return c == 't' || c == 'f';
}

static bool
lgtd_jsonrpc_type_null(const jsmntok_t *t, const char *json)
{
    return memcmp(
        &json[t->start], "null", LGTD_MIN(t->size, (int)sizeof("null"))
    );
}

static bool
lgtd_jsonrpc_type_string(const jsmntok_t *t, const char *json)
{
    (void)json;
    return t->type == JSMN_STRING;
}

static bool
lgtd_jsonrpc_type_array(const jsmntok_t *t, const char *json)
{
    (void)json;
    return t->type == JSMN_ARRAY;
}

static bool
lgtd_jsonrpc_type_object_or_array(const jsmntok_t *t, const char *json)
{
    (void)json;
    return t->type == JSMN_OBJECT || t->type == JSMN_ARRAY;
}

static bool
lgtd_jsonrpc_type_string_number_or_null(const jsmntok_t *t,
                                        const char *json)
{
    return lgtd_jsonrpc_type_number(t, json)
        || lgtd_jsonrpc_type_null(t, json)
        || lgtd_jsonrpc_type_string(t, json);
}

static bool
lgtd_jsonrpc_type_string_or_number(const jsmntok_t *t,
                                   const char *json)
{
    return lgtd_jsonrpc_type_string(t, json)
        || lgtd_jsonrpc_type_number(t, json);
}

static bool __attribute__((unused))
lgtd_jsonrpc_type_string_or_array(const jsmntok_t *t, const char *json)
{
    return lgtd_jsonrpc_type_string(t, json)
        || lgtd_jsonrpc_type_array(t, json);
}

static bool
lgtd_jsonrpc_type_string_number_or_array(const jsmntok_t *t, const char *json)
{
    return lgtd_jsonrpc_type_string(t, json)
        || lgtd_jsonrpc_type_number(t, json)
        || lgtd_jsonrpc_type_array(t, json);
}

static int
lgtd_jsonrpc_float_range_to_uint16(const char *s, int len, int start, int stop)
{
    assert(s);
    assert(len > 0);
    assert(start < stop);

    int range = stop * 10E5 - start * 10E5;
    const char *dot = NULL;
    long fracpart = 0;
    long intpart = strtol(s, (char **)&dot, 10) * 10E5;
    if (dot - s != len && *dot == '.') {
        for (int i = dot - s + 1, multiplier = 10E4;
             i != len && multiplier != 0;
             i++, multiplier /= 10) {
            fracpart += (s[i] - '0') * multiplier;
        }
    }
    return ((intpart + fracpart) * UINT16_MAX) / range;
}

static int
lgtd_jsonrpc_consume_object_or_array(const jsmntok_t *tokens,
                                     int ti,
                                     int parsed,
                                     const char *json)
{
    assert(tokens[ti].type == JSMN_OBJECT || tokens[ti].type == JSMN_ARRAY);
    assert(ti < parsed);

    int objsize = tokens[ti].size;
    if (tokens[ti++].type == JSMN_OBJECT) {
        ti++; // move to the value
    }
    while (objsize-- && ti < parsed) {
        if (tokens[ti].type == JSMN_OBJECT || tokens[ti].type == JSMN_ARRAY) {
            ti = lgtd_jsonrpc_consume_object_or_array(tokens, ti, parsed, json);
       } else {
            ti += tokens[ti].size + 1; // move to the next value
       }
    }

    return ti;
}

static bool
lgtd_jsonrpc_extract_values_from_schema_and_dict(void *output,
                                                 const struct lgtd_jsonrpc_node *schema,
                                                 int schema_size,
                                                 const jsmntok_t *tokens,
                                                 int ntokens,
                                                 const char *json)
{
    if (!ntokens || tokens[0].type != JSMN_OBJECT) {
        return false;
    }

    for (int ti = 1; ti < ntokens;) {
        // make sure it's a key:
        if (tokens[ti].type != JSMN_STRING) {
            return false;
        }

        int si = 0;
        for (;; si++) {
            if (si == schema_size) {
                ti++; // nothing matched, skip the key
                break;
            }

            int tokenlen = LGTD_JSONRPC_TOKEN_LEN(&tokens[ti]);
            if (schema[si].keylen != tokenlen) {
                continue;
            }
            int diff = memcmp(
                schema[si].key, &json[tokens[ti].start], tokenlen
            );
            if (!diff) {
                ti++; // keys looks good, move to the value
                if (!schema[si].type_cmp(&tokens[ti], json)) {
                    lgtd_debug(
                        "jsonrpc client sent an invalid value for %s",
                        schema[si].key
                    );
                    return false;
                }
                if (schema[si].value_offset != -1) {
                    const jsmntok_t *seen = LGTD_JSONRPC_GET_JSMNTOK(
                        output, schema[si].value_offset
                    );
                    if (seen) { // duplicate key
                        lgtd_debug(
                            "jsonrpc client sent duplicate parameter %s",
                            schema[si].key
                        );
                        return false;
                    }
                    LGTD_JSONRPC_SET_JSMNTOK(
                        output, schema[si].value_offset, &tokens[ti]
                    );
                }
                break;
            }
        }

        // skip the value, if it's an object or an array we need to
        // skip everything in it:
        int value_ntokens = ti;
        if (tokens[ti].type == JSMN_OBJECT || tokens[ti].type == JSMN_ARRAY) {
            ti = lgtd_jsonrpc_consume_object_or_array(
                tokens, ti, ntokens, json
            );
        } else {
            ti++;
        }
        value_ntokens = ti - value_ntokens;
        if (si < schema_size && schema[si].ntokens_offset != -1) {
            LGTD_JSONRPC_SET_NTOKENS(
                output, schema[si].ntokens_offset, value_ntokens
            );
        }
    }

    for (int si = 0; si != schema_size; si++) {
        if (!schema[si].optional) {
            const jsmntok_t *seen = LGTD_JSONRPC_GET_JSMNTOK(
                output, schema[si].value_offset
            );
            if (!seen) {
                lgtd_debug("missing jsonrpc parameter %s", schema[si].key);
                return false;
            }
            lgtd_debug("got jsonrpc parameter %s", schema[si].key);
        }
    }

    return true;
}

static bool
lgtd_jsonrpc_extract_values_from_schema_and_array(void *output,
                                                  const struct lgtd_jsonrpc_node *schema,
                                                  int schema_size,
                                                  const jsmntok_t *tokens,
                                                  int ntokens,
                                                  const char *json)
{
    if (!ntokens || tokens[0].type != JSMN_ARRAY) {
        return false;
    }

    int si, ti;
    for (si = 0, ti = 1; si < schema_size && ti < ntokens; si++) {
        if (!schema[si].type_cmp(&tokens[ti], json)) {
            lgtd_debug(
                "jsonrpc client sent an invalid value for %s",
                schema[si].key
            );
            return false;
        }
        if (schema[si].value_offset != -1) {
            LGTD_JSONRPC_SET_JSMNTOK(
                output, schema[si].value_offset, &tokens[ti]
            );
        }
        // skip the value, if it's an object or an array we need to
        // skip everything in it:
        int value_ntokens = ti;
        if (tokens[ti].type == JSMN_OBJECT || tokens[ti].type == JSMN_ARRAY) {
            ti = lgtd_jsonrpc_consume_object_or_array(
                tokens, ti, ntokens, json
            );
        } else {
            ti++;
        }
        value_ntokens = ti - value_ntokens;
        if (schema[si].ntokens_offset != -1) {
            LGTD_JSONRPC_SET_NTOKENS(
                output, schema[si].ntokens_offset, value_ntokens
            );
        }
    }

    return si == schema_size;
}

static bool
lgtd_jsonrpc_extract_and_validate_params_against_schema(void *output,
                                                        const struct lgtd_jsonrpc_node *schema,
                                                        int schema_size,
                                                        const jsmntok_t *tokens,
                                                        int ntokens,
                                                        const char *json)
{
    switch (tokens[0].type) {
    case JSMN_OBJECT:
        return lgtd_jsonrpc_extract_values_from_schema_and_dict(
            output, schema, schema_size, tokens, ntokens, json
        );
    case JSMN_ARRAY:
        return lgtd_jsonrpc_extract_values_from_schema_and_array(
            output, schema, schema_size, tokens, ntokens, json
        );
    default:
        return false;
    }
}

static void
lgtd_jsonrpc_write_id(struct lgtd_client *client,
                      const struct lgtd_jsonrpc_request *request,
                      const char *json)
{
    if (!request->id) {
        LGTD_CLIENT_WRITE_STRING(client, "null");
        return;
    }

    int start, stop;
    if (request->id->type == JSMN_STRING) { // get the quotes
        start = request->id->start - 1;
        stop = request->id->end + 1;
    } else {
        start = request->id->start;
        stop = request->id->end;
    }
    bufferevent_write(client->io, &json[start], stop - start);
}

void
lgtd_jsonrpc_send_error(struct lgtd_client *client,
                        const struct lgtd_jsonrpc_request *request,
                        const char *json,
                        enum lgtd_jsonrpc_error_code code,
                        const char *message)
{
    LGTD_CLIENT_WRITE_STRING(client, "{\"jsonrpc\": \"2.0\", \"id\": ");
    lgtd_jsonrpc_write_id(client, request, json);
    LGTD_CLIENT_WRITE_STRING(client, ", \"error\": {\"code\": ");
    char str_code[8] = { 0 };
    snprintf(str_code, sizeof(str_code), "%d", code);
    LGTD_CLIENT_WRITE_STRING(client, str_code);
    LGTD_CLIENT_WRITE_STRING(client, ", \"message\": \"");
    LGTD_CLIENT_WRITE_STRING(client, message);
    LGTD_CLIENT_WRITE_STRING(client, "\"}}");
}

void
lgtd_jsonrpc_send_response(struct lgtd_client *client,
                           const struct lgtd_jsonrpc_request *request,
                           const char *json,
                           const char *result)
{
    LGTD_CLIENT_WRITE_STRING(client, "{\"jsonrpc\": \"2.0\", \"id\": ");
    lgtd_jsonrpc_write_id(client, request, json);
    LGTD_CLIENT_WRITE_STRING(client, ", \"result\": ");
    LGTD_CLIENT_WRITE_STRING(client, result);
    LGTD_CLIENT_WRITE_STRING(client, "}");
}

static bool
lgtd_jsonrpc_check_and_extract_request(struct lgtd_jsonrpc_request *request,
                                       const jsmntok_t *tokens,
                                       int ntokens,
                                       const char *json)
{
    static const struct lgtd_jsonrpc_node request_schema[] = {
        LGTD_JSONRPC_NODE(
            "jsonrpc", -1, -1, lgtd_jsonrpc_type_string, false
        ),
        LGTD_JSONRPC_NODE(
            "method",
            offsetof(struct lgtd_jsonrpc_request, method),
            -1,
            lgtd_jsonrpc_type_string,
            false
        ),
        LGTD_JSONRPC_NODE(
            "params",
            offsetof(struct lgtd_jsonrpc_request, params),
            offsetof(struct lgtd_jsonrpc_request, params_ntokens),
            lgtd_jsonrpc_type_object_or_array,
            true
        ),
        LGTD_JSONRPC_NODE(
            "id",
            offsetof(struct lgtd_jsonrpc_request, id),
            -1,
            lgtd_jsonrpc_type_string_number_or_null,
            true
        )
    };

    return lgtd_jsonrpc_extract_values_from_schema_and_dict(
        request,
        request_schema,
        LGTD_ARRAY_SIZE(request_schema),
        tokens,
        ntokens,
        json
    );
}

static bool
lgtd_jsonrpc_build_target_list(struct lgtd_proto_target_list *targets,
                               struct lgtd_client *client,
                               const struct lgtd_jsonrpc_request *request,
                               const jsmntok_t *target,
                               int target_ntokens,
                               const char *json)
{
    assert(targets);
    assert(client);
    assert(request);
    assert(target);
    assert(target_ntokens >= 1);
    assert(json);

    if (lgtd_jsonrpc_type_array(target, json)) {
        target_ntokens -= 1;
        target++;
    }

    for (int ti = target_ntokens; ti--;) {
        int token_len = LGTD_JSONRPC_TOKEN_LEN(&target[ti]);
        if (lgtd_jsonrpc_type_string_or_number(&target[ti], json)) {
            struct lgtd_proto_target *t = malloc(sizeof(*t) + token_len + 1);
            if (!t) {
                lgtd_warn("can't allocate a new target");
                lgtd_jsonrpc_send_error(
                    client, request, json, LGTD_JSONRPC_INTERNAL_ERROR,
                    "Can't allocate memory"
                );
                goto error;
            }
            memcpy(t->target, json + target[ti].start, token_len);
            t->target[token_len] = '\0';
            SLIST_INSERT_HEAD(targets, t, link);
        } else {
            lgtd_debug(
                "invalid target value %.*s", token_len, json + target[ti].start
            );
            lgtd_jsonrpc_send_error(
                client, request, json, LGTD_JSONRPC_INVALID_PARAMS,
                "Invalid parameters"
            );
            goto error;
        }
    }

    return true;

error:
    lgtd_proto_target_list_clear(targets);
    return false;
}

static void
lgtd_jsonrpc_check_and_call_set_light_from_hsbk(struct lgtd_client *client,
                                                const struct lgtd_jsonrpc_request *request,
                                                const char *json)
{
    struct lgtd_jsonrpc_set_light_from_hsbk_args {
        const jsmntok_t *target;
        int             target_ntokens;
        const jsmntok_t *h;
        const jsmntok_t *s;
        const jsmntok_t *b;
        const jsmntok_t *k;
        const jsmntok_t *t;
    } params = { NULL, 0, NULL, NULL, NULL, NULL, NULL };
    static const struct lgtd_jsonrpc_node schema[] = {
        LGTD_JSONRPC_NODE(
            "target",
            offsetof(struct lgtd_jsonrpc_set_light_from_hsbk_args, target),
            offsetof(struct lgtd_jsonrpc_set_light_from_hsbk_args, target_ntokens),
            lgtd_jsonrpc_type_string_number_or_array,
            false
        ),
        LGTD_JSONRPC_NODE(
            "hue",
            offsetof(struct lgtd_jsonrpc_set_light_from_hsbk_args, h),
            -1,
            lgtd_jsonrpc_type_float_between_0_and_360,
            false
        ),
        LGTD_JSONRPC_NODE(
            "saturation",
            offsetof(struct lgtd_jsonrpc_set_light_from_hsbk_args, s),
            -1,
            lgtd_jsonrpc_type_float_between_0_and_1,
            false
        ),
        LGTD_JSONRPC_NODE(
            "brightness",
            offsetof(struct lgtd_jsonrpc_set_light_from_hsbk_args, b),
            -1,
            lgtd_jsonrpc_type_float_between_0_and_1,
            false
        ),
        LGTD_JSONRPC_NODE(
            "kelvin",
            offsetof(struct lgtd_jsonrpc_set_light_from_hsbk_args, k),
            -1,
            lgtd_jsonrpc_type_integer,
            false
        ),
        LGTD_JSONRPC_NODE(
            "transition",
            offsetof(struct lgtd_jsonrpc_set_light_from_hsbk_args, t),
            -1,
            lgtd_jsonrpc_type_integer,
            false
        ),
    };

    bool ok = lgtd_jsonrpc_extract_and_validate_params_against_schema(
        &params,
        schema,
        LGTD_ARRAY_SIZE(schema),
        request->params,
        request->params_ntokens,
        json
    );
    if (!ok) {
        goto error_invalid_params;
    }

    int h = lgtd_jsonrpc_float_range_to_uint16(
        &json[params.h->start], LGTD_JSONRPC_TOKEN_LEN(params.h), 0, 360
    );
    int s = lgtd_jsonrpc_float_range_to_uint16(
        &json[params.s->start], LGTD_JSONRPC_TOKEN_LEN(params.s), 0, 1
    );
    int b = lgtd_jsonrpc_float_range_to_uint16(
        &json[params.b->start], LGTD_JSONRPC_TOKEN_LEN(params.b), 0, 1
    );
    errno = 0;
    int k = strtol(&json[params.k->start], NULL, 10);
    if (k < 2500 || k > 9000 || errno == ERANGE) {
        goto error_invalid_params;
    }
    int t = strtol(&json[params.t->start], NULL, 10);
    if (t < 0 || errno == ERANGE) {
        goto error_invalid_params;
    }

    struct lgtd_proto_target_list targets = SLIST_HEAD_INITIALIZER(&targets);
    ok = lgtd_jsonrpc_build_target_list(
        &targets, client, request, params.target, params.target_ntokens, json
    );
    if (!ok) {
        return;
    }

    ok = lgtd_proto_set_light_from_hsbk(&targets, h, s, b, k, t);
    lgtd_proto_target_list_clear(&targets);
    if (ok) {
        lgtd_jsonrpc_send_response(client, request, json, "true");
        return;
    }

error_invalid_params:
    lgtd_jsonrpc_send_error(
        client, request, json, LGTD_JSONRPC_INVALID_PARAMS,
        "Invalid parameters"
    );
}

static bool
lgtd_jsonrpc_extract_target_list(struct lgtd_proto_target_list *targets,
                                 struct lgtd_client *client,
                                 const struct lgtd_jsonrpc_request *request,
                                 const char *json)
{
    struct lgtd_jsonrpc_target_args {
        const jsmntok_t *target;
        int             target_ntokens;
    } params = { NULL, 0 };
    static const struct lgtd_jsonrpc_node schema[] = {
        LGTD_JSONRPC_NODE(
            "target",
            offsetof(struct lgtd_jsonrpc_target_args, target),
            offsetof(struct lgtd_jsonrpc_target_args, target_ntokens),
            lgtd_jsonrpc_type_string_number_or_array,
            false
        )
    };

    bool ok = lgtd_jsonrpc_extract_and_validate_params_against_schema(
        &params, schema, 1, request->params, request->params_ntokens, json
    );
    if (!ok) {
        lgtd_jsonrpc_send_error(
            client, request, json, LGTD_JSONRPC_INVALID_PARAMS,
            "Invalid parameters"
        );
        return false;
    }

    return lgtd_jsonrpc_build_target_list(
        targets, client, request, params.target, params.target_ntokens, json
    );
}

static void
lgtd_jsonrpc_check_and_call_power_on(struct lgtd_client *client,
                                     const struct lgtd_jsonrpc_request *request,
                                     const char *json)
{

    struct lgtd_proto_target_list targets = SLIST_HEAD_INITIALIZER(&targets);
    bool ok = lgtd_jsonrpc_extract_target_list(&targets, client, request, json);
    if (!ok) {
        return;
    }

    ok = lgtd_proto_power_on(&targets);
    lgtd_proto_target_list_clear(&targets);
    if (ok) {
        lgtd_jsonrpc_send_response(client, request, json, "true");
        return;
    }

    lgtd_jsonrpc_send_error(
        client, request, json, LGTD_JSONRPC_INVALID_PARAMS,
        "Invalid parameters"
    );
}

static void
lgtd_jsonrpc_check_and_call_set_waveform(struct lgtd_client *client,
                                         const struct lgtd_jsonrpc_request *request,
                                         const char *json)
{
    struct lgtd_jsonrpc_set_waveform_args {
        const jsmntok_t *target;
        int             target_ntokens;
        const jsmntok_t *waveform;
        const jsmntok_t *h;
        const jsmntok_t *s;
        const jsmntok_t *b;
        const jsmntok_t *k;
        const jsmntok_t *period;
        const jsmntok_t *cycles;
        const jsmntok_t *skew_ratio;
        const jsmntok_t *transient;
    } params = { NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };
    static const struct lgtd_jsonrpc_node schema[] = {
        LGTD_JSONRPC_NODE(
            "target",
            offsetof(struct lgtd_jsonrpc_set_waveform_args, target),
            offsetof(struct lgtd_jsonrpc_set_waveform_args, target_ntokens),
            lgtd_jsonrpc_type_string_number_or_array,
            false
        ),
        LGTD_JSONRPC_NODE(
            "waveform",
            offsetof(struct lgtd_jsonrpc_set_waveform_args, waveform),
            -1,
            lgtd_jsonrpc_type_string,
            false
        ),
        LGTD_JSONRPC_NODE(
            "hue",
            offsetof(struct lgtd_jsonrpc_set_waveform_args, h),
            -1,
            lgtd_jsonrpc_type_float_between_0_and_360,
            false
        ),
        LGTD_JSONRPC_NODE(
            "saturation",
            offsetof(struct lgtd_jsonrpc_set_waveform_args, s),
            -1,
            lgtd_jsonrpc_type_float_between_0_and_1,
            false
        ),
        LGTD_JSONRPC_NODE(
            "brightness",
            offsetof(struct lgtd_jsonrpc_set_waveform_args, b),
            -1,
            lgtd_jsonrpc_type_float_between_0_and_1,
            false
        ),
        LGTD_JSONRPC_NODE(
            "kelvin",
            offsetof(struct lgtd_jsonrpc_set_waveform_args, k),
            -1,
            lgtd_jsonrpc_type_integer,
            false
        ),
        LGTD_JSONRPC_NODE(
            "period",
            offsetof(struct lgtd_jsonrpc_set_waveform_args, period),
            -1,
            lgtd_jsonrpc_type_integer,
            false
        ),
        LGTD_JSONRPC_NODE(
            "cycles",
            offsetof(struct lgtd_jsonrpc_set_waveform_args, cycles),
            -1,
            lgtd_jsonrpc_type_integer,
            false
        ),
        LGTD_JSONRPC_NODE(
            "skew_ratio",
            offsetof(struct lgtd_jsonrpc_set_waveform_args, skew_ratio),
            -1,
            lgtd_jsonrpc_type_float_between_0_and_1,
            false
        ),
        LGTD_JSONRPC_NODE(
            "transient",
            offsetof(struct lgtd_jsonrpc_set_waveform_args, transient),
            -1,
            lgtd_jsonrpc_type_bool,
            false
        ),
    };

    bool ok = lgtd_jsonrpc_extract_and_validate_params_against_schema(
        &params,
        schema,
        LGTD_ARRAY_SIZE(schema),
        request->params,
        request->params_ntokens,
        json
    );
    if (!ok) {
        goto error_invalid_params;
    }

    enum lgtd_lifx_waveform_type waveform;
    waveform = lgtd_lifx_wire_waveform_string_id_to_type(
        &json[params.waveform->start], LGTD_JSONRPC_TOKEN_LEN(params.waveform)
    );
    if (waveform == LGTD_LIFX_WAVEFORM_INVALID) {
        goto error_invalid_params;
    }

    int h = lgtd_jsonrpc_float_range_to_uint16(
        &json[params.h->start], LGTD_JSONRPC_TOKEN_LEN(params.h), 0, 360
    );
    int s = lgtd_jsonrpc_float_range_to_uint16(
        &json[params.s->start], LGTD_JSONRPC_TOKEN_LEN(params.s), 0, 1
    );
    int b = lgtd_jsonrpc_float_range_to_uint16(
        &json[params.b->start], LGTD_JSONRPC_TOKEN_LEN(params.b), 0, 1
    );
    errno = 0;
    int k = strtol(&json[params.k->start], NULL, 10);
    if (k < 2500 || k > 9000 || errno == ERANGE) {
        goto error_invalid_params;
    }
    int period = strtol(&json[params.period->start], NULL, 10);
    if (period <= 0 || errno == ERANGE) {
        goto error_invalid_params;
    }
    int cycles = strtol(&json[params.cycles->start], NULL, 10);
    if (cycles <= 0 || errno == ERANGE) {
        goto error_invalid_params;
    }
    int skew_ratio = lgtd_jsonrpc_float_range_to_uint16(
        &json[params.skew_ratio->start],
        LGTD_JSONRPC_TOKEN_LEN(params.skew_ratio),
        0, 1
    );
    skew_ratio -= UINT16_MAX / 2;
    bool transient = json[params.transient->start] == 't';

    struct lgtd_proto_target_list targets = SLIST_HEAD_INITIALIZER(&targets);
    ok = lgtd_jsonrpc_build_target_list(
        &targets, client, request, params.target, params.target_ntokens, json
    );
    if (!ok) {
        return;
    }

    ok = lgtd_proto_set_waveform(
        &targets, waveform, h, s, b, k, period, cycles, skew_ratio, transient
    );
    lgtd_proto_target_list_clear(&targets);
    if (ok) {
        lgtd_jsonrpc_send_response(client, request, json, "true");
        return;
    }

error_invalid_params:
    lgtd_jsonrpc_send_error(
        client, request, json, LGTD_JSONRPC_INVALID_PARAMS,
        "Invalid parameters"
    );
}

static void
lgtd_jsonrpc_check_and_call_power_off(struct lgtd_client *client,
                                      const struct lgtd_jsonrpc_request *request,
                                      const char *json)
{

    struct lgtd_proto_target_list targets = SLIST_HEAD_INITIALIZER(&targets);
    bool ok = lgtd_jsonrpc_extract_target_list(&targets, client, request, json);
    if (!ok) {
        return;
    }

    ok = lgtd_proto_power_off(&targets);
    lgtd_proto_target_list_clear(&targets);
    if (ok) {
        lgtd_jsonrpc_send_response(client, request, json, "true");
        return;
    }

    lgtd_jsonrpc_send_error(
        client, request, json, LGTD_JSONRPC_INVALID_PARAMS,
        "Invalid parameters"
    );
}

void
lgtd_jsonrpc_dispatch_request(struct lgtd_client *client,
                              const char *json,
                              int parsed)
{
    static const struct lgtd_jsonrpc_method methods[] = {
        LGTD_JSONRPC_METHOD(
            "power_on", 1, // t
            lgtd_jsonrpc_check_and_call_power_on
        ),
        LGTD_JSONRPC_METHOD(
            "power_off", 1, // t
            lgtd_jsonrpc_check_and_call_power_off
        ),
        LGTD_JSONRPC_METHOD(
            "set_light_from_hsbk", 6, // t, h, s, b, k, t
            lgtd_jsonrpc_check_and_call_set_light_from_hsbk
        ),
        LGTD_JSONRPC_METHOD(
            // t, waveform, h, s, b, k, period, cycles, skew_ratio, transient
            "set_waveform", 10,
            lgtd_jsonrpc_check_and_call_set_waveform
        ),
    };

    assert(client);
    assert(parsed >= 0);

    const jsmntok_t *tokens = client->jsmn_tokens;

    // TODO: batch requests

    struct lgtd_jsonrpc_request request;
    memset(&request, 0, sizeof(request));
    bool ok = lgtd_jsonrpc_check_and_extract_request(
        &request,
        tokens,
        parsed,
        json
    );
    if (!ok) {
        lgtd_jsonrpc_send_error(
            client, &request, json, LGTD_JSONRPC_INVALID_REQUEST,
            "Invalid request"
        );
        return;
    }

    assert(request.method);
    assert(request.id);

    for (int i = 0; i != LGTD_ARRAY_SIZE(methods); i++) {
        int parsed_method_namelen = LGTD_JSONRPC_TOKEN_LEN(request.method);
        if (parsed_method_namelen != methods[i].namelen) {
            continue;
        }
        int diff = memcmp(
            methods[i].name, &json[request.method->start], methods[i].namelen
        );
        if (!diff) {
            int params_count = request.params->size;
            if (params_count != methods[i].params_count) {
                lgtd_jsonrpc_send_error(
                    client, &request, json, LGTD_JSONRPC_INVALID_PARAMS,
                    "Invalid number of parameters"
                );
                return;
            }
            methods[i].method(client, &request, json);
            return;
        }
    }

    lgtd_jsonrpc_send_error(
        client, &request, json, LGTD_JSONRPC_METHOD_NOT_FOUND,
        "Method not found"
    );
}
