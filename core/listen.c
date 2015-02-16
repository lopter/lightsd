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
#include <unistd.h>

#include <event2/listener.h>
#include <event2/util.h>

#include "jsmn.h"
#include "client.h"
#include "listen.h"
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
}

bool
lgtd_listen_open(const char *addr, const char *port)
{
    assert(addr);
    assert(port);

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

    struct lgtd_listen *listener;
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
