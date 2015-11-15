#include "jsonrpc.c"

#include "mock_client_buf.h"
#include "mock_log.h"
#include "mock_proto.h"
#include "mock_wire_proto.h"
#include "test_jsonrpc_utils.h"

int
main(void)
{
    const char json[] = "[1234]";
    jsmntok_t tokens[8];

    int rv = parse_json(tokens, LGTD_ARRAY_SIZE(tokens), json, sizeof(json));

    printf("rv = %d\n", rv);

    bool ok = lgtd_jsonrpc_type_integer(&tokens[1], json);

    if (!ok) {
        errx(1, "%s wasn't considered as a valid integer", json);
    }

    return 0;
}
