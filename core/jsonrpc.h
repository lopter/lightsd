// Copyright (c) 2015, Louis Opter <kalessin@kalessin.fr>
//
// This file is part of lighstd.
//
// lighstd is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// lighstd is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with lighstd.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

struct lgtd_client;

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
    int             ntokens_offset;
    bool            (*type_cmp)(const jsmntok_t *, const char *);
    bool            optional;
};

#define LGTD_JSONRPC_SET_JSMNTOK(object, value_offset, value) do {          \
    *(const jsmntok_t **)(&(((char *)(object))[value_offset])) = (value);   \
} while (0)

#define LGTD_JSONRPC_GET_JSMNTOK(object, value_offset)          \
    *(const jsmntok_t **)(&((char *)(object))[value_offset]);

#define LGTD_JSONRPC_SET_NTOKENS(object, ntokens_offset, ntokens) do {  \
    *(int *)(&(((char *)(object))[ntokens_offset])) = (ntokens);          \
} while (0)

#define LGTD_JSONRPC_NODE(key_, value_offset_, ntokens_offset_, fn_type_cmp, optional_)   { \
    .key = (key_),                                                                          \
    .keylen = sizeof((key_)) - 1,                                                           \
    .value_offset = (value_offset_),                                                        \
    .ntokens_offset = (ntokens_offset_),                                                    \
    .type_cmp = (fn_type_cmp),                                                              \
    .optional = (optional_)                                                                 \
}

#define LGTD_JSONRPC_TOKEN_LEN(t) ((t)->end - (t)->start)

struct lgtd_jsonrpc_method {
    const char  *name;
    int         namelen;
    int         params_count;
    void        (*method)(struct lgtd_client *);
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

void lgtd_jsonrpc_dispatch_request(struct lgtd_client *, int);

void lgtd_jsonrpc_send_error(struct lgtd_client *,
                             enum lgtd_jsonrpc_error_code,
                             const char *);
void lgtd_jsonrpc_send_response(struct lgtd_client *,
                                const char *);
