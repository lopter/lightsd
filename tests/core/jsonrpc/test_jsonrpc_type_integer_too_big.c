#include "jsonrpc.c"

#include "mock_client_buf.h"
#include "test_jsonrpc_utils.h"

int
main(void)
{
    const char *json = "9999999999";
    jsmntok_t tokens[8];

    parse_json(tokens, LGTD_ARRAY_SIZE(tokens), json, sizeof(json));

    bool ok = lgtd_jsonrpc_type_integer(tokens, json);

    if (ok) {
        errx(1, "%s wasn't considered invalid", json);
    }

    return 0;
}
