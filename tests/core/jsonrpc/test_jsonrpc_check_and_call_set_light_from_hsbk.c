#include "jsonrpc.c"

#define LGTD_TESTING_SET_LIGHT_FROM_HSBK
#include "test_jsonrpc_utils.h"

static bool set_light_called = false;

bool
lgtd_proto_set_light_from_hsbk(const struct lgtd_proto_target_list *targets,
                               int hue,
                               int saturation,
                               int brightness,
                               int kelvin,
                               int transition_msecs)
{
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
    if (transition_msecs != 60) {
        errx(
            1, "Invalid transition duration: %d, expected: 60", transition_msecs
        );
    }
    set_light_called = true;
    return true;
}

int
main(void)
{
    jsmntok_t tokens[32];
    const char json[] = ("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_light_from_hsbk\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 324.2341514, "
            "\"saturation\": 0.234, "
            "\"brightness\": 1.0, "
            "\"kelvin\": 4200,"
            "\"transition\": 60"
        "},"
        "\"id\": \"42\""
    "}");
    int parsed = parse_json(
        tokens, LGTD_ARRAY_SIZE(tokens), json, sizeof(json)
    );

    bool ok;
    struct lgtd_client client = { .io = NULL };
    struct lgtd_jsonrpc_request req = TEST_REQUEST_INITIALIZER;
    ok = lgtd_jsonrpc_check_and_extract_request(&req, tokens, parsed, json);
    if (!ok) {
        errx(1, "can't parse request");
    }

    lgtd_jsonrpc_check_and_call_set_light_from_hsbk(&client, &req, json);

    const char response[] = ("{"
        "\"jsonrpc\": \"2.0\", "
        "\"id\": \"42\", "
        "\"result\": true"
    "}");

    if (strcmp(client_write_buf, response)) {
        errx(
            1, "invalid response: %s (expected: %s)",
            client_write_buf, response
        );
    }

    if (!set_light_called) {
        errx(1, "lgtd_proto_set_light_from_hsbk wasn't called");
    }

    return 0;
}
