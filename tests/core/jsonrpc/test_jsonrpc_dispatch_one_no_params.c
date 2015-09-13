#include "jsonrpc.c"

#include "mock_client_buf.h"
#define MOCKED_LGTD_PROTO_POWER_ON
#include "mock_proto.h"

#include "test_jsonrpc_utils.h"

void
lgtd_proto_power_on(struct lgtd_client *client,
                    const struct lgtd_proto_target_list *targets)
{
    (void)client;
    (void)targets;

    errx(1, "lgtd_proto_power_on shouldn't have been called");
}

int
main(void)
{
    jsmntok_t tokens[32];
    const char json[] = ("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"power_on\","
        "\"id\": \"42\""
    "}");
    struct lgtd_client client = { .json = json };
    int parsed = parse_json(
        tokens, LGTD_ARRAY_SIZE(tokens), json, sizeof(json)
    );

    lgtd_jsonrpc_dispatch_one(&client, tokens, parsed, NULL);

    const char expected[] = ("{"
        "\"jsonrpc\": \"2.0\", "
        "\"id\": \"42\", "
        "\"error\": {"
            "\"code\": -32602, "
            "\"message\": "
            "\"Invalid number of parameters\""
        "}"
    "}");

    if (memcmp(expected, client_write_buf, sizeof(expected))) {
        errx(
            1, "got %.*s back (expected %s)",
            client_write_buf_idx, client_write_buf, expected
        );
    }

    return 0;
}
