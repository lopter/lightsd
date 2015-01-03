// Copyright (c) 2014, Louis Opter <kalessin@kalessin.fr>
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
#include <sys/tree.h>
#include <arpa/inet.h>
#include <assert.h>
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
#include "bulb.h"
#include "gateway.h"
#include "broadcast.h"
#include "lifxd.h"

static struct {
    evutil_socket_t socket;
    struct event    *read_ev;
    struct event    *write_ev;
    struct event    *discovery_timeout_ev;
} lifxd_broadcast_endpoint = {
    .socket = -1,
    .read_ev = NULL,
    .write_ev = NULL,
    .discovery_timeout_ev = NULL
};

static bool
lifxd_broadcast_handle_read(void)
{
    assert(lifxd_broadcast_endpoint.socket != -1);

    while (true) {
        struct sockaddr_storage peer;
        ev_socklen_t addrlen = sizeof(peer);
        union {
            char buf[LIFXD_MAX_PACKET_SIZE];
            struct lifxd_packet_header hdr;
        } read;
        int nbytes = recvfrom(
            lifxd_broadcast_endpoint.socket,
            read.buf,
            sizeof(read.buf),
            0,
            (struct sockaddr *)&peer,
            &addrlen
        );
        if (nbytes == -1) {
            int error = EVUTIL_SOCKET_ERROR();
            if (error == EINTR) {
                continue;
            }
            if (error == EAGAIN) {
                return true;
            }
            lifxd_warn("can't receive broadcast packet");
            return false;
        }

        char peer_addr[INET6_ADDRSTRLEN];
        lifxd_sockaddrtoa(&peer, peer_addr, sizeof(peer_addr));
        short peer_port = lifxd_sockaddrport(&peer);

        if (nbytes < LIFXD_PACKET_HEADER_SIZE) {
            lifxd_warnx(
                "broadcast packet too short from [%s]:%hu", peer_addr, peer_port
            );
            return false;
        }

        lifxd_wire_decode_header(&read.hdr);
        if (read.hdr.size != nbytes) {
            lifxd_warnx(
                "incomplete broadcast packet from [%s]:%hu",
                peer_addr, peer_port
            );
            return false;
        }
        if (read.hdr.protocol.version != LIFXD_LIFX_PROTOCOL_V1) {
            lifxd_warnx(
                "unsupported protocol %d from [%s]:%hu",
                read.hdr.protocol.version, peer_addr, peer_port
            );
        }
        if (read.hdr.packet_type == LIFXD_GET_PAN_GATEWAY) {
            lifxd_debug(
                "discarding GET_PAN_GATEWAY packet from [%s]:%hu",
                peer_addr, peer_port
            );
            continue;
        }

        const struct lifxd_packet_infos *pkt_infos =
            lifxd_wire_get_packet_infos(read.hdr.packet_type);
        if (!pkt_infos) {
            lifxd_warnx(
                "received unknown packet %#x from [%s]:%hu",
                read.hdr.packet_type, peer_addr, peer_port
            )
            continue;
        }
        if (read.hdr.protocol.tagged || !read.hdr.protocol.addressable) {
            lifxd_warnx(
                "received non-addressable packet %s from [%s]:%hu",
                pkt_infos->name, peer_addr, peer_port
            );
            continue;
        }
        struct lifxd_gateway *gw = lifxd_gateway_get(&peer);
        if (!gw && read.hdr.packet_type == LIFXD_PAN_GATEWAY) {
            gw = lifxd_gateway_open(&peer, read.hdr.site);
            if (!gw) {
                lifxd_err(1, "can't allocate gateway");
            }
            if (event_del(lifxd_broadcast_endpoint.discovery_timeout_ev)) {
                lifxd_err(1, "can't setup events");
            }
        }
        if (gw) {
            void *pkt = &read.buf[LIFXD_PACKET_HEADER_SIZE];
            pkt_infos->decode(pkt);
            pkt_infos->handle(gw, &read.hdr, pkt);
        } else {
            lifxd_warnx(
                "got packet from unknown gateway [%s]:%hu", peer_addr, peer_port
            );
        }
    }
}

static bool
lifxd_broadcast_handle_write(void)
{
    assert(lifxd_broadcast_endpoint.socket != -1);

    struct sockaddr_in lifx_addr = {
        .sin_len = sizeof(lifx_addr),
        .sin_family = AF_INET,
        .sin_addr = { INADDR_BROADCAST },
        .sin_port = htons(lifxd_opts.master_port)
    };
    struct lifxd_packet_header get_pan_gateway;
    lifxd_wire_setup_header(
        &get_pan_gateway,
        LIFXD_TARGET_ALL_DEVICES,
        LIFXD_UNSPEC_TARGET,
        NULL,
        LIFXD_GET_PAN_GATEWAY
    );

    int nbytes;
retry:
    nbytes = sendto(
        lifxd_broadcast_endpoint.socket,
        (void *)&get_pan_gateway,
        sizeof(get_pan_gateway),
        0,
        (const struct sockaddr *)&lifx_addr,
        sizeof(lifx_addr)
    );
    if (nbytes == sizeof(get_pan_gateway)) {
        struct timeval tv = LIFXD_MSECS_TO_TV(
            LIFXD_BROADCAST_DISCOVERY_TIMEOUT_MSEC
        );
        if (event_del(lifxd_broadcast_endpoint.write_ev)
            || event_add(lifxd_broadcast_endpoint.discovery_timeout_ev, &tv)) {
            lifxd_err(1, "can't setup events");
        }
        return true;
    }
    if (nbytes == -1) {
        if (EVUTIL_SOCKET_ERROR() == EINTR) {
            goto retry;
        }
        lifxd_warn("can't broadcast discovery packet");
    } else {
        lifxd_warnx("can't broadcast discovery packet");
    }
    return false;
}

static void
lifxd_broadcast_event_callback(evutil_socket_t socket, short events, void *ctx)
{
    (void)socket;
    (void)ctx;

    if (events & EV_TIMEOUT) {
        // not sure how that could happen but eh.
        lifxd_warnx("timeout on the udp broadcast socket");
        goto error_reset;
    }
    if (events & EV_READ) {
        if (!lifxd_broadcast_handle_read()) {
            goto error_reset;
        }
    }
    if (events & EV_WRITE) {
        if (!lifxd_broadcast_handle_write()) {
            goto error_reset;
        }
    }

    return;

error_reset:
    lifxd_broadcast_close();
    lifxd_broadcast_setup();
    lifxd_broadcast_discovery();
}

static void
lifxd_broadcast_discovery_timeout_event_callback(evutil_socket_t socket,
                                                 short events,
                                                 void *ctx)
{
    (void)socket;
    (void)events;
    (void)ctx;

    lifxd_info(
        "discovery didn't returned anything in %dms, restarting it",
        LIFXD_BROADCAST_DISCOVERY_TIMEOUT_MSEC
    );

    if (!lifxd_broadcast_discovery()) {
        lifxd_err(1, "can't start discovery");
    }
}

void
lifxd_broadcast_close(void)
{
    if (lifxd_broadcast_endpoint.read_ev) {
        event_del(lifxd_broadcast_endpoint.read_ev);
        event_free(lifxd_broadcast_endpoint.read_ev);
        lifxd_broadcast_endpoint.read_ev = NULL;
    }
    if (lifxd_broadcast_endpoint.write_ev) {
        event_del(lifxd_broadcast_endpoint.write_ev);
        event_free(lifxd_broadcast_endpoint.write_ev);
        lifxd_broadcast_endpoint.write_ev = NULL;
    }
    if (lifxd_broadcast_endpoint.discovery_timeout_ev) {
        event_del(lifxd_broadcast_endpoint.discovery_timeout_ev);
        event_free(lifxd_broadcast_endpoint.discovery_timeout_ev);
        lifxd_broadcast_endpoint.discovery_timeout_ev = NULL;
    }
    if (lifxd_broadcast_endpoint.socket != -1) {
        evutil_closesocket(lifxd_broadcast_endpoint.socket);
        lifxd_broadcast_endpoint.socket = -1;
    }
}

bool
lifxd_broadcast_setup(void)
{
    assert(lifxd_broadcast_endpoint.socket == -1);
    assert(lifxd_broadcast_endpoint.read_ev == NULL);
    assert(lifxd_broadcast_endpoint.write_ev == NULL);

    lifxd_broadcast_endpoint.socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (lifxd_broadcast_endpoint.socket == -1) {
        return false;
    }

    int val = 1;
    int err = setsockopt(
        lifxd_broadcast_endpoint.socket,
        SOL_SOCKET,
        SO_BROADCAST,
        &val,
        sizeof(val)
    );
    if (err) {
        goto error;
    }

    if (evutil_make_socket_nonblocking(lifxd_broadcast_endpoint.socket) == -1) {
        goto error;
    }

    struct sockaddr_in lifx_addr = {
        .sin_len = sizeof(lifx_addr),
        .sin_family = AF_INET,
        .sin_addr = { INADDR_ANY },
        .sin_port = htons(lifxd_opts.master_port)
    };

    err = bind(
        lifxd_broadcast_endpoint.socket,
        (const struct sockaddr *)&lifx_addr,
        sizeof(lifx_addr)
    );
    if (err) {
        goto error;
    }

    lifxd_broadcast_endpoint.read_ev = event_new(
        lifxd_ev_base,
        lifxd_broadcast_endpoint.socket,
        EV_READ|EV_PERSIST,
        lifxd_broadcast_event_callback,
        NULL
    );
    lifxd_broadcast_endpoint.write_ev = event_new(
        lifxd_ev_base,
        lifxd_broadcast_endpoint.socket,
        EV_WRITE|EV_PERSIST,
        lifxd_broadcast_event_callback,
        NULL
    );
    lifxd_broadcast_endpoint.discovery_timeout_ev = event_new(
        lifxd_ev_base,
        -1,
        EV_PERSIST,
        lifxd_broadcast_discovery_timeout_event_callback,
        NULL
    );
    if (!lifxd_broadcast_endpoint.read_ev
        || !lifxd_broadcast_endpoint.write_ev
        || !lifxd_broadcast_endpoint.discovery_timeout_ev) {
        goto error;
    }

    if (!event_add(lifxd_broadcast_endpoint.read_ev, NULL)) {
        return true;
    }

    int errsave;
error:
    errsave = errno;
    lifxd_broadcast_close();
    errno = errsave;
    return false;
}

bool
lifxd_broadcast_discovery(void)
{
    assert(lifxd_broadcast_endpoint.write_ev);
    return event_add(lifxd_broadcast_endpoint.write_ev, NULL) == 0;
}
