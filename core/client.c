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
