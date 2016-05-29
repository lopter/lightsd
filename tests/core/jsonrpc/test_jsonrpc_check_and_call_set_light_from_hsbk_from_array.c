#include "jsonrpc.c"

#include "mock_client_buf.h"
#include "mock_log.h"
#define MOCKED_LGTD_PROTO_SET_LIGHT_FROM_HSBK
#include "mock_proto.h"
#include "mock_wire_proto.h"

#include "test_jsonrpc_utils.h"

static int set_light_call_count = 0;

void
lgtd_proto_set_light_from_hsbk(struct lgtd_client *client,
                               const struct lgtd_proto_target_list *targets,
                               int hue,
                               int saturation,
                               int brightness,
                               int kelvin,
                               int transition_msecs)
{
    if (!client) {
        errx(1, "missing client!");
    }

    if (strcmp(SLIST_FIRST(targets)->target, "*")) {
        errx(
            1, "Invalid target [%s] (expected=[*])",
            SLIST_FIRST(targets)->target
        );
    }
    int expected_hue = lgtd_jsonrpc_float_range_to_uint16(
        "324.2341", strlen("324.2341"), 0, 360
    );
    if (hue != expected_hue) {
        errx(1, "Invalid hue: %d, expected: %d", hue, expected_hue);
    }
    int expected_saturation = lgtd_jsonrpc_float_range_to_uint16(
        "0.234", strlen("0.234"), 0, 1
    );
    if (saturation != expected_saturation) {
        errx(
            1, "Invalid saturation: %d, expected: %d",
            saturation, expected_saturation
        );
    }
    if (brightness != UINT16_MAX) {
        errx(
            1, "Invalid brightness: %d, expected: %d",
            brightness, UINT16_MAX
        );
    }
    if (kelvin != 4200) {
        errx(
            1, "Invalid temperature: %d, expected: 4200", kelvin
        );
    }
    switch (set_light_call_count++) {
    case 0:
        if (transition_msecs != 600) {
            errx(
                1, "Invalid transition duration: %d, expected: 600",
                transition_msecs
            );
        }
        break;
    case 1:
        if (transition_msecs != 0) {
            errx(
                1, "Invalid transition duration: %d, expected: 0",
                transition_msecs
            );
        }
        break;
    default:
        errx(1, "set_light_from_hsbk called too many times");
    }
}

int
main(void)
{
    jsmntok_t tokens[32];
    int parsed;
    bool ok;
    struct lgtd_jsonrpc_request req;
    struct lgtd_client client = {
        .io = NULL, .current_request = &req,
    };

    const char *json = ("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_light_from_hsbk\","
        "\"params\": [\"*\", 324.2341514, 0.234, 1.0, 4200, 600],"
        "\"id\": \"42\""
    "}");
    client.json = json;
    parsed = parse_json(tokens, LGTD_ARRAY_SIZE(tokens), json, strlen(json));
    memset(&req, 0, sizeof(req));
    ok = lgtd_jsonrpc_check_and_extract_request(&req, tokens, parsed, json);
    if (!ok) {
        errx(1, "can't parse request");
    }
    lgtd_jsonrpc_check_and_call_set_light_from_hsbk(&client);
    if (set_light_call_count != 1) {
        errx(1, "lgtd_proto_set_light_from_hsbk wasn't called");
    }

    // optional transition
    json = ("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_light_from_hsbk\","
        "\"params\": [\"*\", 324.2341514, 0.234, 1.0, 4200],"
        "\"id\": \"42\""
    "}");
    client.json = json;
    parsed = parse_json(tokens, LGTD_ARRAY_SIZE(tokens), json, strlen(json));
    memset(&req, 0, sizeof(req));
    ok = lgtd_jsonrpc_check_and_extract_request(&req, tokens, parsed, json);
    if (!ok) {
        errx(1, "can't parse request");
    }
    lgtd_jsonrpc_check_and_call_set_light_from_hsbk(&client);
    if (set_light_call_count != 2) {
        errx(1, "lgtd_proto_set_light_from_hsbk wasn't called");
    }

    return 0;
}
