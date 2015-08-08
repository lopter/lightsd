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

enum { LGTD_CLIENT_JSMN_TOKENS_NUM = 48 };
enum { LGTD_CLIENT_MAX_REQUEST_BUF_SIZE = 2048 };

enum lgtd_client_error_code {
    LGTD_CLIENT_SUCCESS = LGTD_JSONRPC_SUCCESS,
    LGTD_CLIENT_PARSE_ERROR = LGTD_JSONRPC_PARSE_ERROR,
    LGTD_CLIENT_INVALID_REQUEST = LGTD_JSONRPC_INVALID_REQUEST,
    LGTD_CLIENT_METHOD_NOT_FOUND = LGTD_JSONRPC_METHOD_NOT_FOUND,
    LGTD_CLIENT_INVALID_PARAMS = LGTD_JSONRPC_INVALID_PARAMS,
    LGTD_CLIENT_INTERNAL_ERROR = LGTD_JSONRPC_INTERNAL_ERROR,
    LGTD_CLIENT_SERVER_ERROR = LGTD_JSONRPC_SERVER_ERROR
};

struct lgtd_client {
    LIST_ENTRY(lgtd_client)     link;
    struct bufferevent          *io;
    char                        ip_addr[INET6_ADDRSTRLEN];
    uint16_t                    port;
    jsmn_parser                 jsmn_ctx;
    jsmntok_t                   jsmn_tokens[LGTD_CLIENT_JSMN_TOKENS_NUM];
    const char                  *json;
    struct lgtd_jsonrpc_request *current_request;
};
LIST_HEAD(lgtd_client_list, lgtd_client);

struct lgtd_client *lgtd_client_open(evutil_socket_t, const struct sockaddr_storage *);
void lgtd_client_open_from_pipe(struct lgtd_client *);
void lgtd_client_close_all(void);

void lgtd_client_write_string(struct lgtd_client *, const char *);
void lgtd_client_write_buf(struct lgtd_client *, const char *, int);
void lgtd_client_send_response(struct lgtd_client *, const char *);
void lgtd_client_start_send_response(struct lgtd_client *);
void lgtd_client_end_send_response(struct lgtd_client *);
void lgtd_client_send_error(struct lgtd_client *, enum lgtd_client_error_code, const char *);
