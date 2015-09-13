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
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
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
                              struct sockaddr *addr,
                              int addrlen,
                              void *ctx)
{
    (void)evlistener;
    struct lgtd_listen *listener = ctx;

    char bufserver[LGTD_SOCKADDR_STRLEN];
    LGTD_SOCKADDRTOA(listener->sockaddr, bufserver);

    struct lgtd_client *client = NULL;
    if (addr->sa_family == AF_UNIX) {
        struct sockaddr_storage sockname;
        memset(&sockname, 0, sizeof(sockname));
        ev_socklen_t socklen = sizeof(sockname);
        getsockname(peer, (struct sockaddr *)&sockname, &socklen);
        client = lgtd_client_open(peer, (struct sockaddr *)&sockname, socklen);
    } else {
        client = lgtd_client_open(peer, addr, addrlen);
    }

    if (!client) {
        char bufclient[LGTD_SOCKADDR_STRLEN];
        lgtd_warn(
            "can't accept new client %s on %s",
            LGTD_SOCKADDRTOA(client->addr, bufclient),
            bufserver
        );
        return;
    }

    lgtd_info("accepted new client %s", bufserver);
}

void
lgtd_listen_close_all(void)
{
    while (!SLIST_EMPTY(&lgtd_listeners)) {
        struct lgtd_listen *listener = SLIST_FIRST(&lgtd_listeners);
        SLIST_REMOVE_HEAD(&lgtd_listeners, link);
        if (listener->sockaddr->sa_family == AF_UNIX) {
            unlink(((struct sockaddr_un *)listener->sockaddr)->sun_path);
        }
        evconnlistener_free(listener->evlistener);
        char addr[LGTD_SOCKADDR_STRLEN];
        LGTD_SOCKADDRTOA(listener->sockaddr, addr);
        lgtd_info("closed socket %s", addr);
        free(listener->sockaddr);
        free(listener);
    }

    lgtd_daemon_update_proctitle();
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
        SLIST_FOREACH(listener, &lgtd_listeners, link) {
            if (listener->addrlen == it->ai_addrlen
                && memcmp(listener->sockaddr, it->ai_addr, it->ai_addrlen)) {
                goto done;
            }
        }

        evlistener = NULL;
        listener = calloc(1, sizeof(*listener));
        if (!listener) {
            goto error;
        }
        listener->sockaddr = calloc(1, it->ai_addrlen);
        if (!listener->sockaddr) {
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
        listener->addrlen = it->ai_addrlen;
        memcpy(listener->sockaddr, it->ai_addr, it->ai_addrlen);

        SLIST_INSERT_HEAD(&lgtd_listeners, listener, link);
        lgtd_info(
            "listening on %s:%s (%s)",
            addr, port, it->ai_family == AF_INET ? "IPv4" : "IPv6"
        );
    }

    lgtd_daemon_update_proctitle();

done:
    evutil_freeaddrinfo(res);
    return true;

error:
    lgtd_warn("can't listen on %s:%s", addr, port);
    if (listener) {
        if (listener->evlistener) {
            evconnlistener_free(evlistener);
        }
        free(listener->sockaddr);
        free(listener);
    }
    evutil_freeaddrinfo(res);
    return false;
}

bool
lgtd_listen_unix_open(const char *path)
{
    assert(path);

    static const int maxpathlen =
        sizeof(struct sockaddr_un) - offsetof(struct sockaddr_un, sun_path);
    int pathlen = strlen(path);
    if (pathlen > maxpathlen) {
        lgtd_warnx(
            "%s (%d bytes) is too long, your system only supports paths up to "
            "%d bytes", path, pathlen, maxpathlen
        );
        return false;
    }

    struct lgtd_listen *listener;
    SLIST_FOREACH(listener, &lgtd_listeners, link) {
        if (listener->addrlen == sizeof(struct sockaddr_un)) {
            struct sockaddr_un *sockaddr;
            sockaddr = (struct sockaddr_un *)listener->sockaddr;
            if (!strcmp(sockaddr->sun_path, path)) {
                return true;
            }
        }
    }

    evutil_socket_t fd = -1;

    listener = calloc(1, sizeof(*listener));
    if (!listener) {
        goto error;
    }

    struct sockaddr_un *sockpath = calloc(1, sizeof(*sockpath));
    if (!sockpath) {
        goto error;
    }
    sockpath->sun_family = AF_UNIX;
    memcpy(sockpath->sun_path, path, pathlen);
    listener->sockaddr = (struct sockaddr *)sockpath;
    listener->addrlen = sizeof(*sockpath);

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        goto error;
    }

    if (evutil_make_socket_nonblocking(fd) == -1) {
        goto error;
    }

    struct stat sb;
    if (stat(path, &sb) == -1) {
        if (errno != ENOENT) {
            goto error;
        }
    } else if ((sb.st_mode & S_IFMT) == S_IFSOCK) {
        lgtd_warnx("removing existing unix socket: %s", path);
        if (unlink(path) == -1 && errno != ENOENT) {
            goto error;
        }
    } else {
        errno = EEXIST;
        goto error;
    }

    if (bind(fd, (struct sockaddr *)sockpath, sizeof(*sockpath)) == -1) {
        goto error;
    }

    mode_t mode = S_IWUSR|S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IWGRP;
    if (chmod(path, mode)) {
        goto error;
    }

    listener->evlistener = evconnlistener_new(
        lgtd_ev_base,
        lgtd_listen_accept_new_client,
        listener,
        LEV_OPT_CLOSE_ON_FREE,
        -1,
        fd
    );
    if (!listener->evlistener) {
        goto error;
    }

    SLIST_INSERT_HEAD(&lgtd_listeners, listener, link);
    lgtd_info("unix socket ready at %s", path);

    return true;

error:
    lgtd_warn("can't open unix socket at %s", path);
    if (fd != -1) {
        close(fd);
    }
    unlink(path);
    free(listener);
    return false;
}
