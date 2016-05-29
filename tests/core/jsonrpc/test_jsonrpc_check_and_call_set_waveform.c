#include "jsonrpc.c"

#include "mock_client_buf.h"
#include "mock_log.h"
#define MOCKED_LGTD_PROTO_SET_WAVEFORM
#include "mock_proto.h"
#define MOCKED_LGTD_LIFX_WIRE_WAVEFORM_STRING_ID_TO_TYPE
#include "mock_wire_proto.h"

#include "test_jsonrpc_utils.h"

enum lgtd_lifx_waveform_type
lgtd_lifx_wire_waveform_string_id_to_type(const char *s, int len)
{
    if (len != 3) {
        errx(1, "err = %d (expected 3)", len);
    }

    if (strncmp(s, "SAW", 3)) {
        errx(1, "s = %.3s (expected SAW)", s);
    }

    return LGTD_LIFX_WAVEFORM_SAW;
}

static int set_waveform_call_count = 0;

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
    if (waveform != LGTD_LIFX_WAVEFORM_SAW) {
        errx(
            1, "Invalid waveform %d: expected: %d",
            waveform, LGTD_LIFX_WAVEFORM_SAW
        );
    }
    switch (set_waveform_call_count++) {
    case 0:
        if (transient) {
            errx(1, "Invalid transient: true (expected false)");
        }
        break;
    case 1:
        if (!transient) {
            errx(1, "Invalid transient: false (expected true)");
        }
        break;
    default:
        errx(1, "set_waveform called too many times");
    }
}

int
main(void)
{
    jsmntok_t tokens[32];
    int parsed;
    bool ok;
    struct lgtd_jsonrpc_request req;
    struct lgtd_client client = { .io = NULL, .current_request = &req };

    const char *json = ("{"
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
            "\"transient\": false,"
            "\"waveform\": \"SAW\""
        "},"
        "\"id\": \"42\""
    "}");
    client.json = json;
    parsed = parse_json(tokens, LGTD_ARRAY_SIZE(tokens), json, strlen(json));
    memset(&req, 0, sizeof(req));
    ok = lgtd_jsonrpc_check_and_extract_request(&req, tokens, parsed, json);
    if (!ok) {
        errx(1, "can't parse request");
    }
    lgtd_jsonrpc_check_and_call_set_waveform(&client);
    if (set_waveform_call_count != 1) {
        errx(1, "lgtd_proto_set_waveform wasn't called");
    }

    // optional transient argument
    json = ("{"
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
            "\"waveform\": \"SAW\""
        "},"
        "\"id\": \"42\""
    "}");
    client.json = json;
    parsed = parse_json(tokens, LGTD_ARRAY_SIZE(tokens), json, strlen(json));
    memset(&req, 0, sizeof(req));
    ok = lgtd_jsonrpc_check_and_extract_request(&req, tokens, parsed, json);
    if (!ok) {
        errx(1, "can't parse request");
    }
    lgtd_jsonrpc_check_and_call_set_waveform(&client);
    if (set_waveform_call_count != 2) {
        errx(1, "lgtd_proto_set_waveform wasn't called");
    }

    return 0;
}
