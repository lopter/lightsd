#include "jsonrpc.c"

#include "mock_client_buf.h"
#include "mock_proto.h"
#include "test_jsonrpc_utils.h"

static void
test_float(const char *json)
{
    jsmntok_t tokens[8];
    parse_json(tokens, LGTD_ARRAY_SIZE(tokens), json, strlen(json));
    if (!lgtd_jsonrpc_type_float_between_0_and_360(tokens, json)) {
        errx(1, "%s wasn't considered as a valid float >= 0 and <= 360", json);
    }
}

int
main(void)
{
    test_float("1.1234");
    test_float("1.00000001");
    test_float("2.0000");
    test_float("10");
    test_float("0.1");
    test_float("0");
    test_float("231.");
    test_float("359.1");
    test_float("360");

    return 0;
}
