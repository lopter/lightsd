#include "jsonrpc.c"

#include "test_jsonrpc_utils.h"

int
main(void)
{
    struct lgtd_client client = { .io = NULL };
    jsmntok_t token = { .start = 1, .end = 3, .type = JSMN_STRING };
    struct lgtd_jsonrpc_request req = { .id = &token };

    lgtd_jsonrpc_send_error(
        &client, &req, "\"42\"",LGTD_JSONRPC_INVALID_REQUEST, "Invalid Request"
    );

    const char *expected = (
        "{"
        "\"jsonrpc\": \"2.0\", "
        "\"id\": \"42\", "
        "\"error\": {\"code\": -32600, \"message\": \"Invalid Request\"}"
        "}"
    );

    int diff = memcmp(client_write_buf, expected, strlen(expected));
    if (diff) {
        printf("expected: %s\n", expected);
        printf("received: %s\n", client_write_buf);
        return 1;
    }

    return 0;
}
