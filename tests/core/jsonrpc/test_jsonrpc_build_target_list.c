#include "jsonrpc.c"

#include "mock_client_buf.h"
#include "mock_proto.h"
#include "test_jsonrpc_utils.h"

static void
test_params(const char *json, const char **expected_targets)
{
    struct lgtd_jsonrpc_request request = { .id = NULL };
    struct lgtd_client client = {
        .io = NULL, .json = json, .current_request = &request
    };

    jsmntok_t tokens[32];
    int parsed = parse_json(
        tokens, LGTD_ARRAY_SIZE(tokens), json, strlen(json)
    );

    struct lgtd_proto_target_list targets = SLIST_HEAD_INITIALIZER(&targets);

    reset_client_write_buf();

    bool ok = lgtd_jsonrpc_build_target_list(&targets, &client, tokens, parsed);

    if (!expected_targets && !SLIST_EMPTY(&targets)) {
        if (ok) {
            errx(1, "lgtd_jsonrpc_build_target_list returned true on an error");
        }
        return;
    }

    struct lgtd_proto_target *target;
    int i = 0;
    SLIST_FOREACH(target, &targets, link) {
        if (!expected_targets[i]) {
            errx(1, "unexpected target %s", target->target);
        }
        if (strcmp(target->target, expected_targets[i])) {
            errx(
                1, "target mismatch got %s but expected %s",
                target->target, expected_targets[i]
            );
        }
        i++;
    }
}

int
main(void)
{
    const char *expected_1[] = {"on", "12345", "6789", NULL};
    test_params("[\"on\", 12345, \"6789\"]", expected_1);

    const char *expected_2[] = {"#tower", NULL};
    test_params("\"#tower\"", expected_2);

    test_params("{\"key\": 42}", NULL);

    const char *expected_3[] = {NULL};
    test_params("[]", expected_3);

    test_params("[\"on\", {\"lol\": \"wut\"}, \"6789\"]", NULL);

    // and make sure nothing blows up on plain invalid json/parameters
    // (jsmn_parse will return a negative value):
    test_params("null", NULL);

    return 0;
}
