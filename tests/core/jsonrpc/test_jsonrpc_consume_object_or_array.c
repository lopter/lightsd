#include "jsonrpc.c"

#include "mock_client_buf.h"
#include "mock_log.h"
#include "mock_proto.h"
#include "test_jsonrpc_utils.h"

int
main(void)
{
    jsmntok_t tokens[32];
    memset(tokens, 0, sizeof(tokens));
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
    int parsed = parse_json(
        tokens, LGTD_ARRAY_SIZE(tokens), json, sizeof(json)
    );

    int ti = lgtd_jsonrpc_consume_object_or_array(tokens, 0, parsed, json);
    if (ti != parsed) {
        errx(1, "ti %d (expected %d)", ti, parsed);
    }

    return 0;
}
