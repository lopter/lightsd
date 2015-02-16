// Copyright (c) 2015, Louis Opter <kalessin@kalessin.fr>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

struct lgtd_jsonrpc_request {
    const jsmntok_t *method;
    const jsmntok_t *params;
    int             params_ntokens;
    const jsmntok_t *id;
};

struct lgtd_jsonrpc_node {
    const char      *key;
    int             keylen;
    int             value_offset;
    bool            (*type_cmp)(const jsmntok_t *, const char *);
    bool            optional;
};

#define LGTD_JSONRPC_SET_JSMNTOK(object, value_offset, value) do {          \
    *(const jsmntok_t **)(&(((char *)(object))[value_offset])) = (value);   \
} while (0)

#define LGTD_JSONRPC_GET_JSMNTOK(object, value_offset)          \
    *(const jsmntok_t **)(&((char *)(object))[value_offset]);   \

#define LGTD_JSONRPC_NODE(key_, value_offset_, fn_type_cmp, optional_)   { \
    .key = (key_),                                                          \
    .keylen = sizeof((key_)) - 1,                                           \
    .value_offset = (value_offset_),                                        \
    .type_cmp = (fn_type_cmp),                                              \
    .optional = (optional_)                                                 \
}

#define LGTD_JSONRPC_TOKEN_LEN(t) ((t)->end - (t)->start)

struct lgtd_jsonrpc_method {
    const char  *name;
    int         namelen;
    int         params_count;
    void        (*method)(struct lgtd_client *,
                          const struct lgtd_jsonrpc_request *,
                          const char *);
};

#define LGTD_JSONRPC_METHOD(name_, params_count_, method_) {    \
    .name = (name_),                                            \
    .namelen = sizeof((name_)) -1,                              \
    .params_count = (params_count_),                            \
    .method = (method_)                                         \
}

enum lgtd_jsonrpc_error_code {
    LGTD_JSONRPC_SUCCESS = 0,
    LGTD_JSONRPC_PARSE_ERROR = -32700,
    LGTD_JSONRPC_INVALID_REQUEST = -32600,
    LGTD_JSONRPC_METHOD_NOT_FOUND = -32601,
    LGTD_JSONRPC_INVALID_PARAMS = -32602,
    LGTD_JSONRPC_INTERNAL_ERROR = -32603,
    LGTD_JSONRPC_SERVER_ERROR = -32000 // (to -32099)
};

void lgtd_jsonrpc_dispatch_request(struct lgtd_client *, const char *, int);

void lgtd_jsonrpc_send_error(struct lgtd_client *,
                             const struct lgtd_jsonrpc_request *,
                             const char *,
                             enum lgtd_jsonrpc_error_code,
                             const char *);
void lgtd_jsonrpc_send_response(struct lgtd_client *,
                                const struct lgtd_jsonrpc_request *,
                                const char *,
                                const char *);
