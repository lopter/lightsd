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

enum { LGTD_CLIENT_JSMN_TOKENS_NUM = 48 };
enum { LGTD_CLIENT_MAX_REQUEST_BUF_SIZE = 2048 };

struct lgtd_client {
    LIST_ENTRY(lgtd_client)  link;
    struct bufferevent      *io;
    char                    ip_addr[INET6_ADDRSTRLEN];
    uint16_t                port;
    jsmn_parser             jsmn_ctx;
    jsmntok_t               jsmn_tokens[LGTD_CLIENT_JSMN_TOKENS_NUM];
};
LIST_HEAD(lgtd_client_list, lgtd_client);

#define LGTD_CLIENT_WRITE_STRING(client, s) do {        \
    bufferevent_write((client)->io, s, strlen((s)));    \
} while(0)

struct lgtd_client *lgtd_client_open(evutil_socket_t, const struct sockaddr_storage *);
void lgtd_client_close_all(void);
