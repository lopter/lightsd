#include <limits.h>

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
    const char json[] = "[[\"*\"],[1,2,3,4]]";
    int parsed = parse_json(
        tokens, LGTD_ARRAY_SIZE(tokens), json, sizeof(json)
    );

    struct lgtd_jsonrpc_target_args {
        const jsmntok_t *target;
        int             target_ntokens;
        const jsmntok_t *label;
    } params = { NULL, 0, NULL };
    static const struct lgtd_jsonrpc_node schema[] = {
        LGTD_JSONRPC_NODE(
            "target",
            offsetof(struct lgtd_jsonrpc_target_args, target),
            offsetof(struct lgtd_jsonrpc_target_args, target_ntokens),
            lgtd_jsonrpc_type_string_number_or_array,
            false
        ),
        LGTD_JSONRPC_NODE(
            "label",
            offsetof(struct lgtd_jsonrpc_target_args, label),
            -1,
            // this must dereference json from the what's in the token (see
            // next comment):
            lgtd_jsonrpc_type_number,
            false
        )
    };

    // invalidate all the tokens so that the test will crash if we go beyond
    // the first list:
    for (int i = 3; i != LGTD_ARRAY_SIZE(tokens); i++) {
        tokens[i].start = INT_MIN;
        tokens[i].end = INT_MIN;
        tokens[i].size = INT_MAX;
        tokens[i].type = JSMN_PRIMITIVE;
    }

    bool ok = lgtd_jsonrpc_extract_and_validate_params_against_schema(
        &params, schema, LGTD_ARRAY_SIZE(schema), &tokens[1], parsed - 1, json
    );

    if (ok) {
        errx(1, "the schema shouldn't have been validated");
    }

    return 0;
}
