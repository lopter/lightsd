#include "jsonrpc.c"

#include "mock_client_buf.h"

#define LGTD_TESTING_POWER_OFF
#include "test_jsonrpc_utils.h"

static bool power_off_called = false;

void
lgtd_proto_power_off(struct lgtd_client *client,
                     const struct lgtd_proto_target_list *targets)
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
    power_off_called = true;
}

int
main(void)
{
    jsmntok_t tokens[32];
    const char json[] = ("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"power_off\","
        "\"params\": {\"target\": \"*\"},"
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

    lgtd_jsonrpc_check_and_call_power_off(&client);

    if (!power_off_called) {
        errx(1, "lgtd_proto_power_off wasn't called");
    }

    return 0;
}
