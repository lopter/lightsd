#include "jsonrpc.c"

#define LGTD_TESTING_SET_WAVEFORM
#include "test_jsonrpc_utils.h"

static bool set_waveform_called = false;

bool
lgtd_proto_set_waveform(const struct lgtd_proto_target_list *targets,
                        enum lgtd_lifx_waveform_type waveform,
                        int hue, int saturation,
                        int brightness, int kelvin,
                        int period, float cycles,
                        int skew_ratio, bool transient)
{
    (void)targets;
    (void)waveform;
    (void)hue;
    (void)saturation;
    (void)brightness;
    (void)kelvin;
    (void)period;
    (void)cycles;
    (void)skew_ratio;
    (void)transient;
    set_waveform_called = true;
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

    lgtd_jsonrpc_check_and_call_set_waveform(&client, &req, json);

    if (!strstr(client_write_buf, "-32602")) {
        errx(1, "no error returned, client_write_buf=[%s]", client_write_buf);
    }

    if (set_waveform_called) {
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
        "\"method\": \"set_waveform\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 324.2341514, "
            "\"saturation\": 0.234, "
            "\"brightness\": 1.0, "
            "\"kelvin\": -4200,"
            "\"cycles\": 10,"
            "\"period\": 1000,"
            "\"skew_ratio\": 0.5,"
            "\"transient\": true,"
            "\"waveform\": \"SAW\""
        "},"
        "\"id\": \"42\""
    "}");

    // saturation to big
    test_request("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_waveform\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 324.2341514, "
            "\"saturation\": 3.234, "
            "\"brightness\": 1.0, "
            "\"kelvin\": 4200,"
            "\"cycles\": 10,"
            "\"period\": 1000,"
            "\"skew_ratio\": 0.5,"
            "\"transient\": true,"
            "\"waveform\": \"SAW\""
        "},"
        "\"id\": \"42\""
    "}");

    // hue too big
    test_request("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_waveform\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 424.2341514, "
            "\"saturation\": 0.234, "
            "\"brightness\": 1.0, "
            "\"kelvin\": 4200,"
            "\"cycles\": 10,"
            "\"period\": 1000,"
            "\"skew_ratio\": 0.5,"
            "\"transient\": true,"
            "\"waveform\": \"SAW\""
        "},"
        "\"id\": \"42\""
    "}");

    // brightness too small
    test_request("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_waveform\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 224.2341514, "
            "\"saturation\": 0.234, "
            "\"brightness\": -1.0, "
            "\"kelvin\": 4200,"
            "\"cycles\": 10,"
            "\"period\": 1000,"
            "\"skew_ratio\": 0.5,"
            "\"transient\": true,"
            "\"waveform\": \"SAW\""
        "},"
        "\"id\": \"42\""
    "}");

    // cycles must be > 0
    test_request("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_waveform\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 224.2341514, "
            "\"saturation\": 0.234, "
            "\"brightness\": 1.0, "
            "\"kelvin\": 4200,"
            "\"cycles\": 0,"
            "\"period\": 1000,"
            "\"skew_ratio\": 0.5,"
            "\"transient\": true,"
            "\"waveform\": \"SAW\""
        "},"
        "\"id\": \"42\""
    "}");

    // skew ratio too big
    test_request("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_waveform\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 224.2341514, "
            "\"saturation\": 0.234, "
            "\"brightness\": 1.0, "
            "\"kelvin\": 4200,"
            "\"cycles\": 10,"
            "\"period\": 1000,"
            "\"skew_ratio\": 1.5,"
            "\"transient\": true,"
            "\"waveform\": \"SAW\""
        "},"
        "\"id\": \"42\""
    "}");

    // skew ratio too small
    test_request("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_waveform\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 224.2341514, "
            "\"saturation\": 0.234, "
            "\"brightness\": 1.0, "
            "\"kelvin\": 4200,"
            "\"cycles\": 10,"
            "\"period\": 1000,"
            "\"skew_ratio\": -1.5,"
            "\"transient\": true,"
            "\"waveform\": \"SAW\""
        "},"
        "\"id\": \"42\""
    "}");

    // invalid waveform
    test_request("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_waveform\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 224.2341514, "
            "\"saturation\": 0.234, "
            "\"brightness\": 1.0, "
            "\"kelvin\": 4200,"
            "\"cycles\": 10,"
            "\"period\": 1000,"
            "\"skew_ratio\": 0.5,"
            "\"transient\": true,"
            "\"waveform\": \"TEST\""
        "},"
        "\"id\": \"42\""
    "}");

    // invalid transient
    test_request("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_waveform\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 224.2341514, "
            "\"saturation\": 0.234, "
            "\"brightness\": 1.0, "
            "\"kelvin\": 4200,"
            "\"cycles\": 10,"
            "\"period\": 1000,"
            "\"skew_ratio\": 0.5,"
            "\"transient\": 42,"
            "\"waveform\": \"SAW\""
        "},"
        "\"id\": \"42\""
    "}");

    // period must be > 0
    test_request("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_waveform\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 224.2341514, "
            "\"saturation\": 0.234, "
            "\"brightness\": 1.0, "
            "\"kelvin\": 4200,"
            "\"cycles\": 10,"
            "\"period\": 0,"
            "\"skew_ratio\": 0.5,"
            "\"transient\": true,"
            "\"waveform\": \"SAW\""
        "},"
        "\"id\": \"42\""
    "}");
}
