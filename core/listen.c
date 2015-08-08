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
#include <string.h>
#include <unistd.h>

#include <event2/listener.h>
#include <event2/util.h>

#include "jsmn.h"
#include "jsonrpc.h"
#include "client.h"
#include "listen.h"
#include "daemon.h"
#include "lightsd.h"

struct lgtd_listen_list lgtd_listeners =
    SLIST_HEAD_INITIALIZER(&lgtd_listeners);

static void
lgtd_listen_accept_new_client(struct evconnlistener *evlistener,
                              evutil_socket_t peer,
                              struct sockaddr *peer_addr,
                              int addrlen,
                              void *ctx)
{
    (void)evlistener;
    (void)addrlen;

    struct lgtd_listen *listener = ctx;
    struct lgtd_client *client = lgtd_client_open(
        peer, (struct sockaddr_storage *)peer_addr
    );
    if (client) {
        lgtd_info(
            "accepted new client [%s]:%hu", client->ip_addr, client->port
        );
        return;
    }
    lgtd_warn(
        "can't accept new client on %s:%s", listener->addr, listener->port
    );
}

void
lgtd_listen_close_all(void)
{
    while (!SLIST_EMPTY(&lgtd_listeners)) {
        struct lgtd_listen *listener = SLIST_FIRST(&lgtd_listeners);
        SLIST_REMOVE_HEAD(&lgtd_listeners, link);
        evconnlistener_free(listener->evlistener);
        free(listener);
    }

    lgtd_daemon_update_proctitle();
}

bool
lgtd_listen_open(const char *addr, const char *port)
{
    assert(addr);
    assert(port);

    struct lgtd_listen *listener;
    SLIST_FOREACH(listener, &lgtd_listeners, link) {
        if (!strcmp(listener->addr, addr) && listener->port == port) {
            return true;
        }
    }

    struct evutil_addrinfo *res = NULL, hints = {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,
        .ai_flags = EVUTIL_AI_NUMERICSERV|EVUTIL_AI_PASSIVE
    };

    int err = evutil_getaddrinfo(addr, port, &hints, &res);
    if (err) {
        lgtd_warnx(
            "can't listen on %s:%s: %s", addr, port, evutil_gai_strerror(err)
        );
        return false;
    }

    struct evconnlistener *evlistener;
    for (struct evutil_addrinfo *it = res; it; it = it->ai_next) {
        evlistener = NULL;
        listener = calloc(1, sizeof(*listener));
        if (!listener) {
            goto error;
        }
        evlistener = evconnlistener_new_bind(
            lgtd_ev_base,
            lgtd_listen_accept_new_client,
            listener,
            LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE,
            -1,
            it->ai_addr,
            it->ai_addrlen
        );
        if (!evlistener) {
            goto error;
        }
        listener->evlistener = evlistener;
        listener->addr = addr;
        listener->port = port;
        SLIST_INSERT_HEAD(&lgtd_listeners, listener, link);
        lgtd_info(
            "listening on %s:%s (%s)",
            addr, port, it->ai_family == AF_INET ? "IPv4" : "IPv6"
        );
    }

    evutil_freeaddrinfo(res);

    lgtd_daemon_update_proctitle();

    return true;

error:
    lgtd_warn("can't listen on %s:%s", addr, port);
    if (evlistener) {
        evconnlistener_free(evlistener);
    }
    free(listener);
    evutil_freeaddrinfo(res);
    return false;
}
