#include "jsonrpc.c"

#define LGTD_TESTING_POWER_ON
#include "test_jsonrpc_utils.h"

static bool power_on_called = false;

bool
lgtd_proto_power_on(const char *target)
{
    if (strcmp(target, "*")) {
        errx(1, "Invalid target [%s] (expected=[*])", target);
    }
    power_on_called = true;
    return true;
}

int
main(void)
{
    jsmntok_t tokens[32];
    const char json[] = ("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"power_on\","
        "\"params\": {\"target\": \"*\"},"
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

    lgtd_jsonrpc_check_and_call_power_on(&client, &req, json);

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

    if (!power_on_called) {
        errx(1, "lgtd_proto_power_on wasn't called");
    }

    return 0;
}
