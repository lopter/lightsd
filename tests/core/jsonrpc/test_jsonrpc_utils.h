#pragma once

#include "mock_gateway.h"

#define TEST_REQUEST_INITIALIZER { NULL, NULL, 0, NULL, 0 }

static inline int
parse_json(jsmntok_t *tokens, size_t capacity, const char *json , size_t len)
{
    jsmn_parser ctx;
    jsmn_init(&ctx);
    return jsmn_parse(&ctx, json, len, tokens, capacity);
}
