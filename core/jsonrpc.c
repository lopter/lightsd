// Copyright (c) 2014, 2015, Louis Opter <kalessin@kalessin.fr>
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
#include <endian.h>
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
#include "jsonrpc.h"
#include "client.h"
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

static bool
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
lgtd_jsonrpc_type_object(const jsmntok_t *t, const char *json)
{
    (void)json;
    return t->type == JSMN_OBJECT;
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

    int range;
    range = stop * LGTD_JSONRPC_FLOAT_PREC - start * LGTD_JSONRPC_FLOAT_PREC;
    const char *dot = NULL;
    long fracpart = 0;
    long intpart = strtol(s, (char **)&dot, 10) * LGTD_JSONRPC_FLOAT_PREC;
    if (dot - s != len && *dot == '.') {
        for (int i = dot - s + 1, multiplier = LGTD_JSONRPC_FLOAT_PREC / 10;
             i != len && multiplier != 0;
             i++, multiplier /= 10) {
            fracpart += (s[i] - '0') * multiplier;
        }
    }
    return ((int64_t)intpart + (int64_t)fracpart) * UINT16_MAX / (int64_t)range;
}

void
lgtd_jsonrpc_uint16_range_to_float_string(uint16_t encoded, int start, int stop,
                                          char *out, int size)
{
    assert(out);
    assert(size > 1);
    assert(start < stop);

    if (size < 2) {
        if (size) {
            *out = '\0';
        }
        return;
    }

    int range;
    range = stop * LGTD_JSONRPC_FLOAT_PREC - start * LGTD_JSONRPC_FLOAT_PREC;
    int value = (uint64_t)encoded * (uint64_t)range / UINT16_MAX;

    if (!value) {
        out[0] = '0';
        out[1] = '\0';
        return;
    }

    int multiplier = 1;
    while (value / (multiplier * 10)) {
        multiplier *= 10;
    }

    int i = 0;

    if (LGTD_JSONRPC_FLOAT_PREC / 10 > multiplier) {
        out[i++] = '0';
        if (i != size) {
            out[i++] = '.';
        }
        for (int divider = 10;
             LGTD_JSONRPC_FLOAT_PREC / divider > multiplier && i != size;
             divider *= 10) {
            out[i++] = '0';
        }
    }

    do {
        if (multiplier == LGTD_JSONRPC_FLOAT_PREC / 10) {
            if (i == 0) {
                out[i++] = '0';
            }
            if (i != size) {
                out[i++] = '.';
            }
        }
        if (i != size) {
            out[i++] = '0' + value / multiplier;
        }
        value -= value / multiplier * multiplier;
        multiplier /= 10;
    } while ((value || multiplier >= LGTD_JSONRPC_FLOAT_PREC)
              && multiplier && i != size);

    out[LGTD_MIN(i, size - 1)] = '\0';

    assert(i <= size);
}

static int
lgtd_jsonrpc_consume_object_or_array(const jsmntok_t *tokens,
                                     int ti,
                                     int parsed,
                                     const char *json)
{
    assert(lgtd_jsonrpc_type_object_or_array(&tokens[ti], json));
    assert(ti < parsed);

    jsmntype_t container_type = tokens[ti].type;
    int objsize = tokens[ti++].size;
    while (objsize-- && ti < parsed) {
        if (container_type == JSMN_OBJECT) {
            ti++; // move to the value
        }
        if (lgtd_jsonrpc_type_object_or_array(&tokens[ti], json)) {
            ti = lgtd_jsonrpc_consume_object_or_array(tokens, ti, parsed, json);
        } else {
            ti++;
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
        // make sure it's a key, otherwise we reached the end of the object:
        if (tokens[ti].type != JSMN_STRING) {
            break;
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
        if (!schema[si].optional && schema[si].value_offset != -1) {
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

    int si, ti, objsize = tokens[0].size;
    for (si = 0, ti = 1; si < schema_size && ti < ntokens && objsize--; si++) {
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

    return !objsize || (si < schema_size && schema[si].optional);
}

static bool
lgtd_jsonrpc_extract_and_validate_params_against_schema(void *output,
                                                        const struct lgtd_jsonrpc_node *schema,
                                                        int schema_size,
                                                        const jsmntok_t *tokens,
                                                        int ntokens,
                                                        const char *json)
{
    if (!ntokens) {
        // "params" were omitted, make sure no args were required or that they
        // are all optional:
        while (schema_size--) {
            if (!schema[schema_size].optional) {
                return false;
            }
        }
        return true;
    }

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
lgtd_jsonrpc_write_id(struct lgtd_client *client)
{
    if (!client->current_request || !client->current_request->id) {
        lgtd_client_write_string(client, "null");
        return;
    }

    int start, stop;
    if (client->current_request->id->type == JSMN_STRING) { // get the quotes
        start = client->current_request->id->start - 1;
        stop = client->current_request->id->end + 1;
    } else {
        start = client->current_request->id->start;
        stop = client->current_request->id->end;
    }
    lgtd_client_write_buf(client, &client->json[start], stop - start);
}

void
lgtd_jsonrpc_send_error(struct lgtd_client *client,
                        enum lgtd_jsonrpc_error_code code,
                        const char *message)
{
    assert(client);
    assert(message);

    lgtd_client_write_string(client, "{\"jsonrpc\": \"2.0\", \"id\": ");
    lgtd_jsonrpc_write_id(client);
    lgtd_client_write_string(client, ", \"error\": {\"code\": ");
    char str_code[8] = { 0 };
    snprintf(str_code, sizeof(str_code), "%d", code);
    lgtd_client_write_string(client, str_code);
    lgtd_client_write_string(client, ", \"message\": \"");
    lgtd_client_write_string(client, message);
    lgtd_client_write_string(client, "\"}}");
}

void
lgtd_jsonrpc_send_response(struct lgtd_client *client,
                           const char *result)
{
    assert(client);
    assert(result);

    lgtd_client_write_string(client, "{\"jsonrpc\": \"2.0\", \"id\": ");
    lgtd_jsonrpc_write_id(client);
    lgtd_client_write_string(client, ", \"result\": ");
    lgtd_client_write_string(client, result);
    lgtd_client_write_string(client, "}");
}

void
lgtd_jsonrpc_start_send_response(struct lgtd_client *client)
{
    assert(client);

    lgtd_client_write_string(client, "{\"jsonrpc\": \"2.0\", \"id\": ");
    lgtd_jsonrpc_write_id(client);
    lgtd_client_write_string(client, ", \"result\": ");
}

void
lgtd_jsonrpc_end_send_response(struct lgtd_client *client)
{
    lgtd_client_write_string(client, "}");
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

    bool ok = lgtd_jsonrpc_extract_values_from_schema_and_dict(
        request,
        request_schema,
        LGTD_ARRAY_SIZE(request_schema),
        tokens,
        ntokens,
        json
    );
    if (!ok) {
        return false;
    }

    request->request_ntokens = 1 + 2 + 2; // dict itself + jsonrpc + method
    if (request->params) {
        request->request_ntokens += 1 + request->params_ntokens;
    }
    if (request->id) {
        request->request_ntokens += 2;
    }

    return true;
}

static bool
lgtd_jsonrpc_build_target_list(struct lgtd_proto_target_list *targets,
                               struct lgtd_client *client,
                               const jsmntok_t *target,
                               int target_ntokens)
{
    assert(targets);
    assert(client);
    assert(target);

    if (target_ntokens < 1) {
        return false;
    }

    if (lgtd_jsonrpc_type_array(target, client->json)) {
        target_ntokens -= 1;
        target++;
    } else if (target_ntokens != 1) {
        return false;
    }

    for (int ti = target_ntokens; ti--;) {
        int token_len = LGTD_JSONRPC_TOKEN_LEN(&target[ti]);
        if (lgtd_jsonrpc_type_string_or_number(&target[ti], client->json)) {
            struct lgtd_proto_target *t = malloc(sizeof(*t) + token_len + 1);
            if (!t) {
                lgtd_warn("can't allocate a new target");
                lgtd_jsonrpc_send_error(
                    client, LGTD_JSONRPC_INTERNAL_ERROR, "Can't allocate memory"
                );
                goto error;
            }
            memcpy(t->target, client->json + target[ti].start, token_len);
            t->target[token_len] = '\0';
            SLIST_INSERT_HEAD(targets, t, link);
        } else {
            lgtd_debug(
                "invalid target value %.*s",
                token_len,
                client->json + target[ti].start
            );
            lgtd_jsonrpc_send_error(
                client, LGTD_JSONRPC_INVALID_PARAMS, "Invalid parameters"
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
lgtd_jsonrpc_check_and_call_set_light_from_hsbk(struct lgtd_client *client)
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
            true
        ),
    };

    bool ok = lgtd_jsonrpc_extract_and_validate_params_against_schema(
        &params,
        schema,
        LGTD_ARRAY_SIZE(schema),
        client->current_request->params,
        client->current_request->params_ntokens,
        client->json
    );
    if (!ok) {
        goto error_invalid_params;
    }

    int h = lgtd_jsonrpc_float_range_to_uint16(
        &client->json[params.h->start], LGTD_JSONRPC_TOKEN_LEN(params.h), 0, 360
    );
    int s = lgtd_jsonrpc_float_range_to_uint16(
        &client->json[params.s->start], LGTD_JSONRPC_TOKEN_LEN(params.s), 0, 1
    );
    int b = lgtd_jsonrpc_float_range_to_uint16(
        &client->json[params.b->start], LGTD_JSONRPC_TOKEN_LEN(params.b), 0, 1
    );
    errno = 0;
    int k = strtol(&client->json[params.k->start], NULL, 10);
    if (k < 2500 || k > 9000 || errno == ERANGE) {
        goto error_invalid_params;
    }
    int t = 0;
    if (params.t) {
        t = strtol(&client->json[params.t->start], NULL, 10);
        if (t < 0 || errno == ERANGE) {
            goto error_invalid_params;
        }
    }

    struct lgtd_proto_target_list targets = SLIST_HEAD_INITIALIZER(&targets);
    ok = lgtd_jsonrpc_build_target_list(
        &targets, client, params.target, params.target_ntokens
    );
    if (!ok) {
        return;
    }

    lgtd_proto_set_light_from_hsbk(client, &targets, h, s, b, k, t);
    lgtd_proto_target_list_clear(&targets);
    return;

error_invalid_params:
    lgtd_jsonrpc_send_error(
        client, LGTD_JSONRPC_INVALID_PARAMS, "Invalid parameters"
    );
}

static void
lgtd_jsonrpc_check_and_call_set_waveform(struct lgtd_client *client)
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
            true
        ),
    };

    bool ok = lgtd_jsonrpc_extract_and_validate_params_against_schema(
        &params,
        schema,
        LGTD_ARRAY_SIZE(schema),
        client->current_request->params,
        client->current_request->params_ntokens,
        client->json
    );
    if (!ok) {
        goto error_invalid_params;
    }

    enum lgtd_lifx_waveform_type waveform;
    waveform = lgtd_lifx_wire_waveform_string_id_to_type(
        &client->json[params.waveform->start], LGTD_JSONRPC_TOKEN_LEN(params.waveform)
    );
    if (waveform == LGTD_LIFX_WAVEFORM_INVALID) {
        goto error_invalid_params;
    }

    int h = lgtd_jsonrpc_float_range_to_uint16(
        &client->json[params.h->start], LGTD_JSONRPC_TOKEN_LEN(params.h), 0, 360
    );
    int s = lgtd_jsonrpc_float_range_to_uint16(
        &client->json[params.s->start], LGTD_JSONRPC_TOKEN_LEN(params.s), 0, 1
    );
    int b = lgtd_jsonrpc_float_range_to_uint16(
        &client->json[params.b->start], LGTD_JSONRPC_TOKEN_LEN(params.b), 0, 1
    );
    errno = 0;
    int k = strtol(&client->json[params.k->start], NULL, 10);
    if (k < 2500 || k > 9000 || errno == ERANGE) {
        goto error_invalid_params;
    }
    int period = strtol(&client->json[params.period->start], NULL, 10);
    if (period <= 0 || errno == ERANGE) {
        goto error_invalid_params;
    }
    int cycles = strtol(&client->json[params.cycles->start], NULL, 10);
    if (cycles <= 0 || errno == ERANGE) {
        goto error_invalid_params;
    }
    int skew_ratio = lgtd_jsonrpc_float_range_to_uint16(
        &client->json[params.skew_ratio->start],
        LGTD_JSONRPC_TOKEN_LEN(params.skew_ratio),
        0, 1
    );
    skew_ratio -= UINT16_MAX / 2;
    bool transient = params.transient ?
        client->json[params.transient->start] == 't' : true;

    struct lgtd_proto_target_list targets = SLIST_HEAD_INITIALIZER(&targets);
    ok = lgtd_jsonrpc_build_target_list(
        &targets, client, params.target, params.target_ntokens
    );
    if (!ok) {
        return;
    }

    lgtd_proto_set_waveform(
        client, &targets,
        waveform, h, s, b, k,
        period, cycles, skew_ratio, transient
    );
    lgtd_proto_target_list_clear(&targets);
    return;

error_invalid_params:
    lgtd_jsonrpc_send_error(
        client, LGTD_JSONRPC_INVALID_PARAMS, "Invalid parameters"
    );
}

static bool
lgtd_jsonrpc_extract_target_list(struct lgtd_proto_target_list *targets,
                                 struct lgtd_client *client)
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

    struct lgtd_jsonrpc_request *req = client->current_request;
    bool ok = lgtd_jsonrpc_extract_and_validate_params_against_schema(
        &params, schema, 1, req->params, req->params_ntokens, client->json
    );
    if (!ok) {
        lgtd_jsonrpc_send_error(
            client, LGTD_JSONRPC_INVALID_PARAMS, "Invalid parameters"
        );
        return false;
    }

    return lgtd_jsonrpc_build_target_list(
        targets, client, params.target, params.target_ntokens
    );
}

#define CHECK_AND_CALL_TARGETS_ONLY_METHOD(proto_method)                       \
static void                                                                    \
lgtd_jsonrpc_check_and_call_##proto_method(struct lgtd_client *client)         \
{                                                                              \
    struct lgtd_proto_target_list targets = SLIST_HEAD_INITIALIZER(&targets);  \
    bool ok = lgtd_jsonrpc_extract_target_list(&targets, client);              \
    if (!ok) {                                                                 \
        return;                                                                \
    }                                                                          \
                                                                               \
    lgtd_proto_##proto_method(client, &targets);                               \
    lgtd_proto_target_list_clear(&targets);                                    \
}

CHECK_AND_CALL_TARGETS_ONLY_METHOD(power_on);
CHECK_AND_CALL_TARGETS_ONLY_METHOD(power_off);
CHECK_AND_CALL_TARGETS_ONLY_METHOD(power_toggle);
CHECK_AND_CALL_TARGETS_ONLY_METHOD(get_light_state);

static void
lgtd_jsonrpc_check_and_call_proto_tag_or_untag_or_set_label(
        struct lgtd_client *client,
        void (*lgtd_proto_fn)(struct lgtd_client *,
                              const struct lgtd_proto_target_list *,
                              const char *))

{
    struct lgtd_jsonrpc_target_args {
        const jsmntok_t *target;
        int             target_ntokens;
        const jsmntok_t *label;
    } params = { NULL, 0, NULL };
    static const struct lgtd_jsonrpc_node schema[] = {
        LGTD_JSONRPC_NODE(
            "target",
            offsetof(struct lgtd_jsonrpc_target_args, target),
            offsetof(struct lgtd_jsonrpc_target_args, target_ntokens),
            lgtd_jsonrpc_type_string_number_or_array,
            false
        ),
        LGTD_JSONRPC_NODE(
            "label",
            offsetof(struct lgtd_jsonrpc_target_args, label),
            -1,
            lgtd_jsonrpc_type_string,
            false
        )
    };

    struct lgtd_jsonrpc_request *req = client->current_request;
    bool ok = lgtd_jsonrpc_extract_and_validate_params_against_schema(
        &params,
        schema,
        LGTD_ARRAY_SIZE(schema),
        req->params,
        req->params_ntokens,
        client->json
    );
    if (!ok) {
        lgtd_jsonrpc_send_error(
            client, LGTD_JSONRPC_INVALID_PARAMS, "Invalid parameters"
        );
        return;
    }

    struct lgtd_proto_target_list targets = SLIST_HEAD_INITIALIZER(&targets);
    ok = lgtd_jsonrpc_build_target_list(
        &targets, client, params.target, params.target_ntokens
    );
    if (!ok) {
        return;
    }

    char *label = strndup(
        &client->json[params.label->start], LGTD_JSONRPC_TOKEN_LEN(params.label)
    );
    if (!label) {
        lgtd_warn("can't allocate a label");
        lgtd_jsonrpc_send_error(
            client, LGTD_JSONRPC_INTERNAL_ERROR, "Can't allocate memory"
        );
        goto error_strdup;
    }

    lgtd_proto_fn(client, &targets, label);

    free(label);

error_strdup:
    lgtd_proto_target_list_clear(&targets);
}

static void
lgtd_jsonrpc_check_and_call_tag(struct lgtd_client *client)
{
    return lgtd_jsonrpc_check_and_call_proto_tag_or_untag_or_set_label(
        client, lgtd_proto_tag
    );
}

static void
lgtd_jsonrpc_check_and_call_untag(struct lgtd_client *client)
{
    return lgtd_jsonrpc_check_and_call_proto_tag_or_untag_or_set_label(
        client, lgtd_proto_untag
    );
}

static void
lgtd_jsonrpc_check_and_call_set_label(struct lgtd_client *client)
{
    return lgtd_jsonrpc_check_and_call_proto_tag_or_untag_or_set_label(
        client, lgtd_proto_set_label
    );
}

static void
lgtd_jsonrpc_batch_prepare_next_part(struct lgtd_client *client,
                                     const int *batch_sent)
{
    assert(client);

    if (batch_sent) {
        if (*batch_sent == 1) {
            lgtd_client_write_string(client, "[");
        } else if (*batch_sent) {
            lgtd_client_write_string(client, ",");
        }
    }
}

static int
lgtd_jsonrpc_dispatch_one(struct lgtd_client *client,
                          const jsmntok_t *tokens,
                          int ntokens,
                          int *batch_sent)
{
    static const struct lgtd_jsonrpc_method methods[] = {
        LGTD_JSONRPC_METHOD("power_on", lgtd_jsonrpc_check_and_call_power_on),
        LGTD_JSONRPC_METHOD("power_off", lgtd_jsonrpc_check_and_call_power_off),
        LGTD_JSONRPC_METHOD(
            "power_toggle", lgtd_jsonrpc_check_and_call_power_toggle
        ),
        LGTD_JSONRPC_METHOD(
            "set_light_from_hsbk",
            lgtd_jsonrpc_check_and_call_set_light_from_hsbk
        ),
        LGTD_JSONRPC_METHOD(
            "set_waveform", lgtd_jsonrpc_check_and_call_set_waveform
        ),
        LGTD_JSONRPC_METHOD(
            "get_light_state", lgtd_jsonrpc_check_and_call_get_light_state
        ),
        LGTD_JSONRPC_METHOD("tag", lgtd_jsonrpc_check_and_call_tag),
        LGTD_JSONRPC_METHOD("untag", lgtd_jsonrpc_check_and_call_untag),
        LGTD_JSONRPC_METHOD("set_label", lgtd_jsonrpc_check_and_call_set_label)
    };

    if (batch_sent) {
        ++*batch_sent;
    }

    enum lgtd_jsonrpc_error_code error_code;
    const char *error_msg;

    struct lgtd_jsonrpc_request request;
    memset(&request, 0, sizeof(request));
    bool ok = lgtd_jsonrpc_check_and_extract_request(
        &request, tokens, ntokens, client->json
    );
    client->current_request = &request;
    if (!ok) {
        error_code = LGTD_JSONRPC_INVALID_REQUEST;
        error_msg = "Invalid request";
        request.request_ntokens = lgtd_jsonrpc_consume_object_or_array(
            tokens, 0, ntokens, client->json
        );
        goto error;
    }

    assert(request.method);
    assert(request.request_ntokens);

    for (int i = 0; i != LGTD_ARRAY_SIZE(methods); i++) {
        int parsed_method_namelen = LGTD_JSONRPC_TOKEN_LEN(request.method);
        if (parsed_method_namelen != methods[i].namelen) {
            continue;
        }
        int diff = memcmp(
            methods[i].name,
            &client->json[request.method->start],
            methods[i].namelen
        );
        if (!diff) {
            struct bufferevent *client_io = NULL; // keep compilers happy...
            if (!request.id) {
                // Ugly hack to behave correctly on jsonrpc notifications, it's
                // not worth doing it properly right now. It is especially ugly
                // since we can't properly close that client now (but we don't
                // do that in lgtd_proto and signals are deferred with the
                // event loop).
                client_io = client->io;
                client->io = NULL;
                if (batch_sent) {
                    --*batch_sent;
                }
            } else {
                lgtd_jsonrpc_batch_prepare_next_part(client, batch_sent);
            }
            methods[i].method(client);
            if (!request.id) {
                client->io = client_io;
            }
            client->current_request = NULL;
            return request.request_ntokens;
        }
    }

    error_code = LGTD_JSONRPC_METHOD_NOT_FOUND;
    error_msg = "Method not found";

error:
    lgtd_jsonrpc_batch_prepare_next_part(client, batch_sent);
    lgtd_jsonrpc_send_error(client, error_code, error_msg);
    client->current_request = NULL;
    return request.request_ntokens;
}

void
lgtd_jsonrpc_dispatch_request(struct lgtd_client *client, int parsed)
{
    assert(client);
    assert(parsed >= 0);

    if (!parsed || !client->jsmn_tokens[0].size) {
        lgtd_jsonrpc_send_error(
            client, LGTD_JSONRPC_INVALID_REQUEST, "Invalid request"
        );
        return;
    }

    if (!lgtd_jsonrpc_type_array(client->jsmn_tokens, client->json)) {
        lgtd_jsonrpc_dispatch_one(client, client->jsmn_tokens, parsed, NULL);
        return;
    }

    int batch_sent = 0;
    for (int ti = 1; ti < parsed;) {
        const jsmntok_t *tok = &client->jsmn_tokens[ti];

        if (lgtd_jsonrpc_type_object(tok, client->json)) {
            ti += lgtd_jsonrpc_dispatch_one(
                client, tok, parsed - ti, &batch_sent
            );
        } else {
            batch_sent++;
            lgtd_jsonrpc_batch_prepare_next_part(client, &batch_sent);
            lgtd_jsonrpc_send_error(
                client, LGTD_JSONRPC_INVALID_REQUEST, "Invalid request"
            );
            if (lgtd_jsonrpc_type_array(tok, client->json)) {
                ti = lgtd_jsonrpc_consume_object_or_array(
                    client->jsmn_tokens, ti, parsed, client->json
                );
            } else {
                ti++;
            }
        }
    }

    if (batch_sent) {
        lgtd_client_write_string(client, "]");
    }
}
