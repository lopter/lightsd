#include "jsonrpc.c"

#define LGTD_TESTING_SET_LIGHT_FROM_HSBK
#include "test_jsonrpc_utils.h"

static bool set_light_called = false;

void
lgtd_proto_set_light_from_hsbk(struct lgtd_client *client,
                               const struct lgtd_proto_target_list *targets,
                               int hue,
                               int saturation,
                               int brightness,
                               int kelvin,
                               int transition_msecs)
{
    (void)client;
    (void)targets;
    (void)hue;
    (void)saturation;
    (void)brightness;
    (void)kelvin;
    (void)transition_msecs;
    set_light_called = true;
}

static void
test_request(const char *json)
{
    jsmntok_t tokens[32];
    int parsed = parse_json(
        tokens, LGTD_ARRAY_SIZE(tokens), json, strlen(json)
    );

    bool ok;
    struct lgtd_jsonrpc_request req = TEST_REQUEST_INITIALIZER;
    struct lgtd_client client = {
        .io = NULL, .current_request = &req, .json = json
    };
    ok = lgtd_jsonrpc_check_and_extract_request(&req, tokens, parsed, json);
    if (!ok) {
        errx(1, "can't parse request");
    }

    lgtd_jsonrpc_check_and_call_set_light_from_hsbk(&client);

    if (set_light_called) {
        errx(1, "lgtd_proto_power_off was called");
    }

    reset_client_write_buf();
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
            "\"kelvin\": -4200,"
            "\"transition\": 42"
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
            "\"kelvin\": 4200,"
            "\"transition\": 42"
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
            "\"kelvin\": 4200,"
            "\"transition\": 42"
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
            "\"kelvin\": 4200,"
            "\"transition\": 42"
        "},"
        "\"id\": \"42\""
    "}");

    // negative transition
    test_request("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_light_from_hsbk\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 224.2341514, "
            "\"saturation\": 0.234, "
            "\"brightness\": -1.0, "
            "\"kelvin\": 4200,"
            "\"transition\": -42"
        "},"
        "\"id\": \"42\""
    "}");
    return 0;
}
