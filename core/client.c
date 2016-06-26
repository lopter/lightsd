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
#include <sys/tree.h>
#include <assert.h>
#include <endian.h>
#include <err.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>

#include "lifx/wire_proto.h"
#include "jsmn.h"
#include "jsonrpc.h"
#include "client.h"
#include "proto.h"
#include "stats.h"
#include "daemon.h"
#include "lightsd.h"

struct lgtd_client_list lgtd_clients = LIST_HEAD_INITIALIZER(&lgtd_clients);

static void
lgtd_client_close(struct lgtd_client *client)
{
    assert(client);
    assert(client->io);

    LGTD_STATS_ADD_AND_UPDATE_PROCTITLE(clients, -1);

    LIST_REMOVE(client, link);
    if (client->io) { // XXX: see ugly hack in lgtd_jsonrpc_dispatch_one
        bufferevent_free(client->io);
    }
    free(client->addr);
    free(client->jsmn_tokens);
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

    char addr[LGTD_SOCKADDR_STRLEN];
    LGTD_SOCKADDRTOA(client->addr, addr);

    struct evbuffer *input = bufferevent_get_input(bev);
    size_t nbytes = evbuffer_get_contiguous_space(input);
    // Get the actual pointer to the beginning of the evbuf:
    const char *buf = (char *)evbuffer_pullup(input, nbytes);

    jsmntok_t *tokens = NULL;
    int ntokens = 0;
    do {
        jsmn_parser jsmn_ctx;
    parse_after_realloc:
        jsmn_init(&jsmn_ctx);
        int rv = jsmn_parse(&jsmn_ctx, buf, nbytes, tokens, ntokens);
        switch (rv) {
        case JSMN_ERROR_NOMEM:
        case JSMN_ERROR_INVAL:
            lgtd_warnx("client %s: request too big or invalid", addr);
            evbuffer_drain(input, nbytes);
            break;
        case JSMN_ERROR_PART:
        case 0:
            (void)0;
            size_t buflen = evbuffer_get_length(input);
            if (buflen > LGTD_CLIENT_MAX_REQUEST_BUF_SIZE) {
                lgtd_warnx("client %s: request too big or invalid", addr);
                evbuffer_drain(input, buflen);
            } else if (nbytes == buflen) {
                return; // We pulled up everything already, wait for more data
            }
            break;
        default:
            ntokens = rv;
            if (tokens) {
                client->json = buf;
                lgtd_jsonrpc_dispatch_request(client, ntokens);
                client->json = NULL;
                size_t request_size = tokens[0].end;
                tokens = NULL;
                evbuffer_drain(input, request_size);
                if (request_size < nbytes) {
                    buf += request_size;
                    nbytes -= request_size;
                    // FIXME: instead of calling jsmn_parse again, return the
                    // number of tokens consumed from jsonrpc and make this
                    // case a loop.
                    continue;
                }
                break;
            } else {
                client->jsmn_tokens = reallocarray(
                    client->jsmn_tokens, ntokens, sizeof(*tokens)
                );
                tokens = client->jsmn_tokens;
                goto parse_after_realloc;
            }
        }
        // pullup and resume parsing:
        buf = (char *)evbuffer_pullup(input, -1);
        nbytes = evbuffer_get_contiguous_space(input);
    } while (nbytes);
}

static void
lgtd_client_event_callback(struct bufferevent *bev, short events, void *ctx)
{
    (void)bev;
    assert(ctx);

    struct lgtd_client *client = ctx;

    if (events & (BEV_EVENT_ERROR|BEV_EVENT_EOF)) {
        char addr[LGTD_SOCKADDR_STRLEN];
        lgtd_info("lost connection with client %s", LGTD_SOCKADDRTOA(
            client->addr, addr
        ));
        lgtd_client_close(client);
    }
}

void
lgtd_client_write_string(struct lgtd_client *client, const char *msg)
{
    assert(client);
    assert(msg);

    if (client->io) {
        bufferevent_write(client->io, msg, strlen(msg));
    }
}

void
lgtd_client_write_buf(struct lgtd_client *client, const char *buf, int bufsz)
{
    assert(client);
    assert(buf);
    assert(bufsz >= 0);

    if (bufsz > 0 && client->io) {
        bufferevent_write(client->io, buf, bufsz);
    }
}

void
lgtd_client_send_response(struct lgtd_client *client, const char *msg)
{
    lgtd_jsonrpc_send_response(client, msg);
}

void
lgtd_client_start_send_response(struct lgtd_client *client)
{
    lgtd_jsonrpc_start_send_response(client);
}

void
lgtd_client_end_send_response(struct lgtd_client *client)
{
    lgtd_jsonrpc_end_send_response(client);
}

void
lgtd_client_send_error(struct lgtd_client *client,
                       enum lgtd_client_error_code error,
                       const char *msg)
{
    lgtd_jsonrpc_send_error(client, (enum lgtd_jsonrpc_error_code)error, msg);
}

struct lgtd_client *
lgtd_client_open(evutil_socket_t peer, const struct sockaddr *addr, int addrlen)
{
    assert(peer != -1);
    assert(addr);

    struct lgtd_client *client = calloc(1, sizeof(*client));
    if (!client) {
        return NULL;
    }

    client->io = bufferevent_socket_new(
        lgtd_ev_base, peer, BEV_OPT_CLOSE_ON_FREE
    );
    if (!client->io) {
        goto error;
    }

    client->addr = calloc(1, addrlen);
    if (!client->addr) {
        goto error;
    }
    memcpy(client->addr, addr, addrlen);

    bufferevent_setcb(
        client->io,
        lgtd_client_read_callback,
        NULL,
        lgtd_client_event_callback,
        client
    );
    if (bufferevent_enable(client->io, EV_READ|EV_WRITE|EV_TIMEOUT) == -1) {
        goto error;
    }

    LIST_INSERT_HEAD(&lgtd_clients, client, link);

    LGTD_STATS_ADD_AND_UPDATE_PROCTITLE(clients, 1);

    return client;

error:
    if (client->io) {
        bufferevent_free(client->io);
    }
    free(client->addr);
    free(client);
    return NULL;
}

void
lgtd_client_open_from_pipe(struct lgtd_client *pipe_client)
{
    assert(pipe_client);

    memset(pipe_client, 0, sizeof(*pipe_client));
}
