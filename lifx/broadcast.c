// Copyright (c) 2014, Louis Opter <kalessin@kalessin.fr>
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
#include <arpa/inet.h>
#include <assert.h>
#include <endian.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/util.h>

#include "wire_proto.h"
#include "core/time_monotonic.h"
#include "bulb.h"
#include "gateway.h"
#include "broadcast.h"
#include "core/lightsd.h"

static struct {
    evutil_socket_t socket;
    struct event    *read_ev;
    struct event    *write_ev;
} lgtd_lifx_broadcast_endpoint = {
    .socket = -1,
    .read_ev = NULL,
    .write_ev = NULL,
};

static bool
lgtd_lifx_broadcast_handle_write(void)
{
    assert(lgtd_lifx_broadcast_endpoint.socket != -1);

    struct sockaddr_in lifx_addr = {
        .sin_family = AF_INET,
        .sin_addr = { INADDR_BROADCAST },
        .sin_port = htons(LGTD_LIFX_PROTOCOL_PORT),
        .sin_zero = { 0 }
    };
    struct lgtd_lifx_packet_header get_pan_gateway;
    lgtd_lifx_wire_setup_header(
        &get_pan_gateway,
        LGTD_LIFX_TARGET_ALL_DEVICES,
        LGTD_LIFX_UNSPEC_TARGET,
        NULL,
        LGTD_LIFX_GET_PAN_GATEWAY
    );

    int nbytes;
retry:
    nbytes = sendto(
        lgtd_lifx_broadcast_endpoint.socket,
        (void *)&get_pan_gateway,
        sizeof(get_pan_gateway),
        0,
        (const struct sockaddr *)&lifx_addr,
        sizeof(lifx_addr)
    );
    if (nbytes == sizeof(get_pan_gateway)) {
        if (event_del(lgtd_lifx_broadcast_endpoint.write_ev)) {
            lgtd_err(1, "can't setup events");
        }
        return true;
    }
    if (nbytes == -1) {
        if (EVUTIL_SOCKET_ERROR() == EINTR) {
            goto retry;
        }
        lgtd_warn("can't broadcast discovery packet");
    } else {
        lgtd_warnx("can't broadcast discovery packet");
    }
    return false;
}

static void
lgtd_lifx_broadcast_event_callback(evutil_socket_t socket,
                                   short events,
                                   void *ctx)
{
    (void)socket;
    (void)ctx;

    if (events & EV_TIMEOUT) {
        // not sure how that could happen but eh.
        lgtd_warnx("timeout on the udp broadcast socket");
        goto error_reset;
    }
    if ((events & EV_READ) && !lgtd_lifx_wire_handle_receive(socket, NULL)) {
        goto error_reset;
    }
    if ((events & EV_WRITE) && !lgtd_lifx_broadcast_handle_write()) {
        goto error_reset;
    }

    return;

error_reset:
    lgtd_lifx_broadcast_close();
    lgtd_lifx_broadcast_setup();
}

void
lgtd_lifx_broadcast_close(void)
{
    if (lgtd_lifx_broadcast_endpoint.read_ev) {
        event_del(lgtd_lifx_broadcast_endpoint.read_ev);
        event_free(lgtd_lifx_broadcast_endpoint.read_ev);
        lgtd_lifx_broadcast_endpoint.read_ev = NULL;
    }
    if (lgtd_lifx_broadcast_endpoint.write_ev) {
        event_del(lgtd_lifx_broadcast_endpoint.write_ev);
        event_free(lgtd_lifx_broadcast_endpoint.write_ev);
        lgtd_lifx_broadcast_endpoint.write_ev = NULL;
    }
    if (lgtd_lifx_broadcast_endpoint.socket != -1) {
        evutil_closesocket(lgtd_lifx_broadcast_endpoint.socket);
        lgtd_lifx_broadcast_endpoint.socket = -1;
    }
}

bool
lgtd_lifx_broadcast_setup(void)
{
    assert(lgtd_lifx_broadcast_endpoint.socket == -1);
    assert(lgtd_lifx_broadcast_endpoint.read_ev == NULL);
    assert(lgtd_lifx_broadcast_endpoint.write_ev == NULL);

    lgtd_lifx_broadcast_endpoint.socket = socket(
        AF_INET, SOCK_DGRAM, IPPROTO_UDP
    );
    if (lgtd_lifx_broadcast_endpoint.socket == -1) {
        return false;
    }

    int val = 1;
    int err = setsockopt(
        lgtd_lifx_broadcast_endpoint.socket,
        SOL_SOCKET,
        SO_BROADCAST,
        &val,
        sizeof(val)
    );
    if (err) {
        goto error;
    }
    err = evutil_make_listen_socket_reuseable(
        lgtd_lifx_broadcast_endpoint.socket
    );
    if (err) {
        goto error;
    }

    err = evutil_make_socket_nonblocking(lgtd_lifx_broadcast_endpoint.socket);
    if (err == -1) {
        goto error;
    }

    struct sockaddr_in lifx_addr = {
        .sin_family = AF_INET,
        .sin_addr = { INADDR_ANY },
        .sin_port = htons(LGTD_LIFX_PROTOCOL_PORT),
        .sin_zero = { 0 }
    };

    err = bind(
        lgtd_lifx_broadcast_endpoint.socket,
        (const struct sockaddr *)&lifx_addr,
        sizeof(lifx_addr)
    );
    if (err) {
        goto error;
    }

    lgtd_lifx_broadcast_endpoint.read_ev = event_new(
        lgtd_ev_base,
        lgtd_lifx_broadcast_endpoint.socket,
        EV_READ|EV_PERSIST,
        lgtd_lifx_broadcast_event_callback,
        NULL
    );
    lgtd_lifx_broadcast_endpoint.write_ev = event_new(
        lgtd_ev_base,
        lgtd_lifx_broadcast_endpoint.socket,
        EV_WRITE|EV_PERSIST,
        lgtd_lifx_broadcast_event_callback,
        NULL
    );
    if (!lgtd_lifx_broadcast_endpoint.read_ev
        || !lgtd_lifx_broadcast_endpoint.write_ev) {
        goto error;
    }

    if (!event_add(lgtd_lifx_broadcast_endpoint.read_ev, NULL)) {
        return true;
    }

    int errsave;
error:
    errsave = errno;
    lgtd_lifx_broadcast_close();
    errno = errsave;
    return false;
}

bool
lgtd_lifx_broadcast_discovery(void)
{
    assert(lgtd_lifx_broadcast_endpoint.write_ev);
    return event_add(lgtd_lifx_broadcast_endpoint.write_ev, NULL) == 0;
}
