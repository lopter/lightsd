#include "jsonrpc.c"

#include "mock_client_buf.h"
#include "mock_log.h"
#include "mock_proto.h"
#include "test_jsonrpc_utils.h"

int
main(void)
{
    const char *json = "\"42\"";
    jsmntok_t token = { .start = 1, .end = 3, .type = JSMN_STRING };
    struct lgtd_jsonrpc_request req = { .id = &token };
    struct lgtd_client client = {
        .io = NULL, .current_request = &req, .json = json
    };
    const char *expected = (
        "{"
        "\"jsonrpc\": \"2.0\", "
        "\"id\": \"42\", "
        "\"error\": {\"code\": -32600, \"message\": \"Invalid Request\"}"
        "}"
    );
    lgtd_jsonrpc_send_error(
        &client, LGTD_JSONRPC_INVALID_REQUEST, "Invalid Request"
    );
    int diff = memcmp(client_write_buf, expected, strlen(expected));
    if (diff) {
        printf("expected: %s\n", expected);
        printf("received: %s\n", client_write_buf);
        return 1;
    }

    reset_client_write_buf();

    req.id = NULL;
    expected = (
        "{"
        "\"jsonrpc\": \"2.0\", "
        "\"id\": null, "
        "\"error\": {\"code\": -32600, \"message\": \"Invalid Request\"}"
        "}"
    );
    lgtd_jsonrpc_send_error(
        &client, LGTD_JSONRPC_INVALID_REQUEST, "Invalid Request"
    );
    diff = memcmp(client_write_buf, expected, strlen(expected));
    if (diff) {
        printf("expected: %s\n", expected);
        printf("received: %s\n", client_write_buf);
        return 1;
    }

    return 0;
}
