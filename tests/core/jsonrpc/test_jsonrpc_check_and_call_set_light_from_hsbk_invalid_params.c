#include "jsonrpc.c"

#define LGTD_TESTING_SET_LIGHT_FROM_HSBK
#include "test_jsonrpc_utils.h"

static bool set_light_called = false;

bool
lgtd_proto_set_light_from_hsbk(const char *target,
                               int hue,
                               int saturation,
                               int brightness,
                               int kelvin,
                               int transition_msecs)
{
    (void)target;
    (void)hue;
    (void)saturation;
    (void)brightness;
    (void)kelvin;
    (void)transition_msecs;
    set_light_called = true;
    return true;
}

static void
test_request(const char *json)
{
    jsmntok_t tokens[32];
    int parsed = parse_json(
        tokens, LGTD_ARRAY_SIZE(tokens), json, strlen(json)
    );

    bool ok;
    struct lgtd_client client = { .io = NULL };
    struct lgtd_jsonrpc_request req = TEST_REQUEST_INITIALIZER;
    ok = lgtd_jsonrpc_check_and_extract_request(&req, tokens, parsed, json);
    if (!ok) {
        errx(1, "can't parse request");
    }

    lgtd_jsonrpc_check_and_call_set_light_from_hsbk(&client, &req, json);

    if (!strstr(client_write_buf, "-32602")) {
        errx(1, "no error returned, client_write_buf=[%s]", client_write_buf);
    }

    if (set_light_called) {
        errx(1, "lgtd_proto_power_off was called");
    }

    memset(client_write_buf, 0, sizeof(client_write_buf));
    client_write_buf_idx = 0;
}

int
main(void)
{
    // invalid temperature:
    test_request("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_light_from_hsbk\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 324.2341514, "
            "\"saturation\": 0.234, "
            "\"brightness\": 1.0, "
            "\"kelvin\": -4200"
        "},"
        "\"id\": \"42\""
    "}");

    // saturation to big
    test_request("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_light_from_hsbk\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 324.2341514, "
            "\"saturation\": 3.234, "
            "\"brightness\": 1.0, "
            "\"kelvin\": 4200"
        "},"
        "\"id\": \"42\""
    "}");

    // hue too big
    test_request("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_light_from_hsbk\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 424.2341514, "
            "\"saturation\": 0.234, "
            "\"brightness\": 1.0, "
            "\"kelvin\": 4200"
        "},"
        "\"id\": \"42\""
    "}");

    // brightness too small
    test_request("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_light_from_hsbk\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 224.2341514, "
            "\"saturation\": 0.234, "
            "\"brightness\": -1.0, "
            "\"kelvin\": 4200"
        "},"
        "\"id\": \"42\""
    "}");

    return 0;
}
