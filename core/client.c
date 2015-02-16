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

#include <sys/queue.h>
#include <assert.h>
#include <err.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>

#include "jsmn.h"
#include "client.h"
#include "jsonrpc.h"
#include "lightsd.h"

struct lgtd_client_list lgtd_clients = LIST_HEAD_INITIALIZER(&lgtd_clients);

static void
lgtd_client_close(struct lgtd_client *client)
{
    assert(client);
    assert(client->io);

    LIST_REMOVE(client, link);
    bufferevent_free(client->io);
    free(client);
}

void
lgtd_client_close_all(void)
{
    struct lgtd_client *client, *next_client;
    LIST_FOREACH_SAFE(client, &lgtd_clients, link, next_client) {
        lgtd_client_close(client);
    }
}

static void
lgtd_client_read_callback(struct bufferevent *bev, void *ctx)
{
    assert(ctx);

    struct lgtd_client *client = ctx;

    struct evbuffer *input = bufferevent_get_input(bev);
    size_t bufsz = evbuffer_get_contiguous_space(input);
    // Get the actual pointer to the beginning of the evbuf:
    const char *buf = (char *)evbuffer_pullup(input, bufsz);
    jsmnerr_t rv;

retry_after_pullup:
    rv = jsmn_parse(
        &client->jsmn_ctx,
        buf,
        bufsz,
        client->jsmn_tokens,
        LGTD_ARRAY_SIZE(client->jsmn_tokens)
    );
    switch (rv) {
    case JSMN_ERROR_NOMEM:
        lgtd_warnx(
            "dropping client [%s]:%hu: request too big, not "
            "enough parser tokens", client->ip_addr, client->port
        );
        lgtd_client_close(client);
        break;
    case JSMN_ERROR_INVAL:
        lgtd_warnx(
            "dropping client [%s]:%hu: invalid json",
            client->ip_addr, client->port
        );
        // TODO: consume remaining data and send a proper error instead of
        // closing the connection:
        lgtd_client_close(client);
        break;
    case JSMN_ERROR_PART:
        if (evbuffer_get_length(input) > LGTD_CLIENT_MAX_REQUEST_BUF_SIZE) {
            lgtd_warnx(
                "dropping client [%s]:%hu: request too big",
                client->ip_addr, client->port
            );
            lgtd_client_close(client);
            break;
        } else if (bufsz >= evbuffer_get_length(input)) {
            break; // We pulled up everything already, wait for more data
        }
        // pullup and resume parsing:
        buf = (char *)evbuffer_pullup(input, -1);
        bufsz = evbuffer_get_length(input);
        goto retry_after_pullup;
    default:
        lgtd_jsonrpc_dispatch_request(client, buf, rv);
        evbuffer_drain(input, bufsz);
        jsmn_init(&client->jsmn_ctx);
        break;
    }
}

static void
lgtd_client_event_callback(struct bufferevent *bev, short events, void *ctx)
{
    (void)bev;
    assert(ctx);

    struct lgtd_client *client = ctx;

    if (events & (BEV_EVENT_ERROR|BEV_EVENT_EOF)) {
        lgtd_info(
            "lost connection with client [%s]:%hu",
            client->ip_addr, client->port
        );
        lgtd_client_close(client);
    }
}

struct lgtd_client *
lgtd_client_open(evutil_socket_t peer, const struct sockaddr_storage *peer_addr)
{
    assert(peer != -1);
    assert(peer_addr);

    struct lgtd_client *client = calloc(1, sizeof(*client));
    if (!client) {
        return NULL;
    }
    client->io = bufferevent_socket_new(
        lgtd_ev_base, peer, BEV_OPT_CLOSE_ON_FREE
    );
    if (!client->io) {
        return NULL;
    }
    bufferevent_setcb(
        client->io,
        lgtd_client_read_callback,
        NULL,
        lgtd_client_event_callback,
        client
    );
    lgtd_sockaddrtoa(peer_addr, client->ip_addr, sizeof(client->ip_addr));
    client->port = lgtd_sockaddrport(peer_addr);
    jsmn_init(&client->jsmn_ctx);
    bufferevent_enable(client->io, EV_READ|EV_WRITE|EV_TIMEOUT);

    LIST_INSERT_HEAD(&lgtd_clients, client, link);

    return client;
}
