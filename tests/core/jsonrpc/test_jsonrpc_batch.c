#include "jsonrpc.c"

#include "mock_client_buf.h"
#define MOCKED_LGTD_PROTO_GET_LIGHT_STATE
#define MOCKED_LGTD_PROTO_POWER_ON
#include "mock_proto.h"
#include "test_jsonrpc_utils.h"

static int power_on_call_count = 0;

void
lgtd_proto_power_on(struct lgtd_client *client,
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

    if (power_on_call_count++) {
        errx(1, "proto_power_on should have been called once");
    }
}

static int get_light_state_call_count = 0;

void
lgtd_proto_get_light_state(struct lgtd_client *client,
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

    if (get_light_state_call_count++) {
        errx(1, "proto_power_on should have been called once");
    }
}

int
main(void)
{
    jsmntok_t tokens[32];
    const char json[] = ("["
        "{"
            "\"method\": \"power_on\","
            "\"id\": \"004daf12-0561-4fbc-bfdb-bfe69cfbf4b5\","
            "\"params\": [\"*\"],"
            "\"jsonrpc\": \"2.0\""
        "},"
        "{"
            "\"method\": \"get_light_state\","
            "\"id\": \"1f7a32c8-6741-4ee7-bec1-8431c7d514dc\","
            "\"params\": [\"*\"],"
            "\"jsonrpc\": \"2.0\""
        "}"
    "]");
    struct lgtd_client client = { .json = json, .jsmn_tokens = tokens };
    int parsed = parse_json(
        tokens, LGTD_ARRAY_SIZE(tokens), json, sizeof(json)
    );

    lgtd_jsonrpc_dispatch_request(&client, parsed);

    if (!power_on_call_count) {
        errx(1, "power_on was never called");
    }

    if (!get_light_state_call_count) {
        errx(1, "get_light_state was never called");
    }

    const char expected[] = "[,]";  // we mocked the functions
    if (strcmp(expected, client_write_buf)) {
        errx(1, "got client buf %s (expected %s)", client_write_buf, expected);
    }

    return 0;
}
