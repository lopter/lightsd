#include "jsonrpc.c"

#include "mock_client_buf.h"

#define LGTD_TESTING_SET_WAVEFORM
#include "test_jsonrpc_utils.h"

static bool set_waveform_called = false;

void
lgtd_proto_set_waveform(struct lgtd_client *client,
                        const struct lgtd_proto_target_list *targets,
                        enum lgtd_lifx_waveform_type waveform,
                        int hue, int saturation,
                        int brightness, int kelvin,
                        int period, float cycles,
                        int skew_ratio, bool transient)
{
    if (!client) {
        errx(1, "missing client");
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
    if (period != 1000) {
        errx(1, "Invalid period: %d, expected: 1000", period);
    }
    if (cycles != 10) {
        errx(1, "Invalid cycles: %d, expected: 10", (int)cycles);
    }
    if (skew_ratio != 0) {
        errx(1, "Invalid skew_ratio: %d, expected: 0", skew_ratio);
    }
    if (!transient) {
        errx(1, "transient is false instead of true");
    }
    if (waveform != LGTD_LIFX_WAVEFORM_SAW) {
        errx(
            1, "Invalid waveform %d: expected: %d",
            waveform, LGTD_LIFX_WAVEFORM_SAW
        );
    }
    set_waveform_called = true;
}

int
main(void)
{
    jsmntok_t tokens[32];
    const char json[] = ("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"set_waveform\","
        "\"params\": {"
            "\"target\": \"*\", "
            "\"hue\": 324.2341514, "
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
    int parsed = parse_json(
        tokens, LGTD_ARRAY_SIZE(tokens), json, sizeof(json)
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

    lgtd_jsonrpc_check_and_call_set_waveform(&client);

    if (!set_waveform_called) {
        errx(1, "lgtd_proto_set_waveform wasn't called");
    }

    return 0;
}
