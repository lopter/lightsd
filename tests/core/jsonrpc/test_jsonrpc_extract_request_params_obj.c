#include "jsonrpc.c"

#include "test_jsonrpc_utils.h"

int
main(void)
{
    jsmntok_t tokens[32];
    const char json[] = ("{"
        "\"jsonrpc\": \"2.0\","
        "\"method\": \"hello\","
        "\"params\": {\"on\": true},"
        "\"id\": \"42\""
    "}");
    int parsed = parse_json(
        tokens, LGTD_ARRAY_SIZE(tokens), json, sizeof(json)
    );

    struct lgtd_jsonrpc_request req = TEST_REQUEST_INITIALIZER;
    bool ok = lgtd_jsonrpc_check_and_extract_request(
        &req, tokens, parsed, json
    );

    if (!ok) {
        errx(1, "return value should be true");
    }

    if (!req.method) {
        errx(1, "missing method");
    }
    if (req.method->end - req.method->start != sizeof("hello") - 1
        || memcmp(&json[req.method->start], "hello", sizeof("hello") - 1)) {
        errx(1, "wrong method name");
    }

    if (!req.params) {
        errx(1, "missing params");
    }
    const char params[] = "{\"on\": true}";
    if (req.params->end - req.params->start != sizeof(params) - 1
        || memcmp(&json[req.params->start], params, req.params->size)) {
        errx(1, "wrong params");
    }

    if (!req.id) {
        errx(1, "missing id");
    }
    if (req.id->end - req.id->start != 2
        || memcmp(&json[req.id->start], "42", 2)) {
        errx(1, "wrong id");
    }

    ok = lgtd_jsonrpc_check_and_extract_request(
        &req, tokens, 0, json
    );
    if (ok) {
        errx(1, "empty request should return false");
    }

    return 0;
}
