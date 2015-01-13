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
#include <assert.h>
#include <endian.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/util.h>

#include "wire_proto.h"
#include "time_monotonic.h"
#include "bulb.h"
#include "gateway.h"
#include "broadcast.h"
#include "timer.h"
#include "lifxd.h"

struct lifxd_gateway_list lifxd_gateways =
    LIST_HEAD_INITIALIZER(&lifxd_gateways);

void
lifxd_gateway_close(struct lifxd_gateway *gw)
{
    assert(gw);

    event_del(gw->refresh_ev);
    event_del(gw->write_ev);
    if (gw->socket != -1) {
        evutil_closesocket(gw->socket);
        LIST_REMOVE(gw, link);
    }
    event_free(gw->refresh_ev);
    event_free(gw->write_ev);
    evbuffer_free(gw->write_buf);
    struct lifxd_bulb *bulb, *next_bulb;
    SLIST_FOREACH_SAFE(bulb, &gw->bulbs, link_by_gw, next_bulb) {
        lifxd_bulb_close(bulb);
    }

    lifxd_info(
        "connection with gateway bulb [%s]:%hu closed", gw->ip_addr, gw->port
    );
    free(gw);
}

static void
lifxd_gateway_write_callback(evutil_socket_t socket, short events, void *ctx)
{
    (void)socket;

    assert(ctx);

    struct lifxd_gateway *gw = (struct lifxd_gateway *)ctx;
    if (events & EV_TIMEOUT) {  // Not sure how that could happen in UDP but eh.
        lifxd_warn(
            "lost connection with gateway bulb [%s]:%hu", gw->ip_addr, gw->port
        );
        lifxd_gateway_close(gw);
        if (!lifxd_broadcast_discovery()) {
            lifxd_err(1, "can't start auto discovery");
        }
        return;
    }
    if (events & EV_WRITE) {
        if (evbuffer_write(gw->write_buf, gw->socket) == -1 && errno != EAGAIN) {
            lifxd_warn("can't write to [%s]:%hu", gw->ip_addr, gw->port);
            lifxd_gateway_close(gw);
            if (!lifxd_broadcast_discovery()) {
                lifxd_err(1, "can't start auto discovery");
            }
            return;
        }
        // Callbacks are called in any order, so we keep two timers to make
        // sure we can get the latency right, otherwise we could be compute the
        // latency with last_pkt_at < last_req_at, which isn't true since the
        // pkt will be for an answer the previous write:
        gw->last_req_at = gw->next_req_at;
        gw->next_req_at = lifxd_time_monotonic_msecs();
        if (!evbuffer_get_length(gw->write_buf)) {
            event_del(gw->write_ev);
        }
    }
}

static void
lifxd_gateway_send_get_all_light_state(struct lifxd_gateway *gw)
{
    assert(gw);

    struct lifxd_packet_header hdr;
    union lifxd_target target = { .addr = gw->site };
    lifxd_wire_setup_header(
        &hdr, LIFXD_TARGET_SITE, target, gw->site, LIFXD_GET_LIGHT_STATE
    );
    lifxd_debug("GET_LIGHT_STATE --> [%s]:%hu", gw->ip_addr, gw->port);
    lifxd_gateway_send_packet(gw, &hdr, NULL, 0);
}

static void
lifxd_gateway_refresh_callback(evutil_socket_t socket, short events, void *ctx)
{
    (void)socket;
    (void)events;
    lifxd_gateway_send_get_all_light_state((struct lifxd_gateway *)ctx);
}

static struct lifxd_bulb *
lifxd_gateway_get_or_open_bulb(struct lifxd_gateway *gw, const uint8_t *bulb_addr)
{
    assert(gw);
    assert(bulb_addr);

    struct lifxd_bulb *bulb = lifxd_bulb_get(gw, bulb_addr);
    if (!bulb) {
        bulb = lifxd_bulb_open(gw, bulb_addr);
        if (bulb) {
            SLIST_INSERT_HEAD(&gw->bulbs, bulb, link_by_gw);
            lifxd_info(
                "bulb %s on [%s]:%hu",
                lifxd_addrtoa(bulb_addr), gw->ip_addr, gw->port
            );
        }
    }
    return bulb;
}

struct lifxd_gateway *
lifxd_gateway_open(const struct sockaddr_storage *peer,
                   const uint8_t *site,
                   lifxd_time_mono_t received_at)
{
    assert(peer);
    assert(site);

    struct lifxd_gateway *gw = calloc(1, sizeof(*gw));
    if (!gw) {
        lifxd_warn("can't allocate a new gateway bulb");
        return false;
    }
    gw->socket = socket(peer->ss_family, SOCK_DGRAM, IPPROTO_UDP);
    if (gw->socket == -1) {
        lifxd_warn("can't open a new socket");
        goto error_socket;
    }
    if (connect(gw->socket, (const struct sockaddr *)peer, sizeof(*peer)) == -1
        || evutil_make_socket_nonblocking(gw->socket) == -1) {
        lifxd_warn("can't open a new socket");
        goto error_connect;
    }
    gw->write_ev = event_new(
        lifxd_ev_base,
        gw->socket,
        EV_WRITE|EV_PERSIST,
        lifxd_gateway_write_callback,
        gw
    );
    gw->write_buf = evbuffer_new();
    gw->refresh_ev = evtimer_new(
        lifxd_ev_base, lifxd_gateway_refresh_callback, gw
    );
    memcpy(&gw->peer, peer, sizeof(gw->peer));
    lifxd_sockaddrtoa(peer, gw->ip_addr, sizeof(gw->ip_addr));
    gw->port = lifxd_sockaddrport(peer);
    memcpy(gw->site, site, sizeof(gw->site));
    gw->last_req_at = received_at;
    gw->next_req_at = received_at;
    gw->last_pkt_at = received_at;

    struct timeval refresh_interval = LIFXD_MSECS_TO_TIMEVAL(
        LIFXD_GATEWAY_MIN_REFRESH_INTERVAL_MSECS
    );

    if (!gw->write_ev || !gw->write_buf || !gw->refresh_ev
        || event_add(gw->refresh_ev, &refresh_interval) != 0) {
        lifxd_warn("can't allocate a new gateway bulb");
        goto error_allocate;
    }

    lifxd_info(
        "gateway for site %s at [%s]:%hu",
        lifxd_addrtoa(gw->site), gw->ip_addr, gw->port
    );
    LIST_INSERT_HEAD(&lifxd_gateways, gw, link);

    // In case this is the first bulb (re-)discovered, start the watchdog, it
    // will stop by itself:
    lifxd_timer_start_watchdog();

    return gw;

error_allocate:
    if (gw->write_ev) {
        event_free(gw->write_ev);
    }
    if (gw->write_buf) {
        evbuffer_free(gw->write_buf);
    }
    if (gw->refresh_ev) {
        event_free(gw->refresh_ev);
    }
error_connect:
    evutil_closesocket(gw->socket);
error_socket:
    free(gw);
    return NULL;
}

struct lifxd_gateway *
lifxd_gateway_get(const struct sockaddr_storage *peer)
{
    assert(peer);

    struct lifxd_gateway *gw, *next_gw;
    LIST_FOREACH_SAFE(gw, &lifxd_gateways, link, next_gw) {
        if (peer->ss_family == gw->peer.ss_family
            && !memcmp(&gw->peer, peer, sizeof(*peer))) {
            return gw;
        }
    }

    return NULL;
}

void
lifxd_gateway_close_all(void)
{
    struct lifxd_gateway *gw, *next_gw;
    LIST_FOREACH_SAFE(gw, &lifxd_gateways, link, next_gw) {
        lifxd_gateway_close(gw);
    }
}

void
lifxd_gateway_send_packet(struct lifxd_gateway *gw,
                          const struct lifxd_packet_header *hdr,
                          const void *pkt,
                          int pkt_size)
{
    assert(gw);
    assert(hdr);
    assert(pkt_size >= 0 && pkt_size < LIFXD_MAX_PACKET_SIZE);
    assert(!memcmp(hdr->site, gw->site, LIFXD_ADDR_LENGTH));

    evbuffer_add(gw->write_buf, hdr, sizeof(*hdr));
    if (pkt) {
        assert((unsigned)pkt_size == le16toh(hdr->size) - sizeof(*hdr));
        evbuffer_add(gw->write_buf, pkt, pkt_size);
    }
    event_add(gw->write_ev, NULL);
}

void
lifxd_gateway_handle_pan_gateway(struct lifxd_gateway *gw,
                                 const struct lifxd_packet_header *hdr,
                                 const struct lifxd_packet_pan_gateway *pkt)
{
    assert(gw && hdr && pkt);

    lifxd_debug(
        "SET_PAN_GATEWAY <-- [%s]:%hu - %s site=%s",
        gw->ip_addr, gw->port,
        lifxd_addrtoa(hdr->target.device_addr),
        lifxd_addrtoa(hdr->site)
    );
}

void
lifxd_gateway_handle_light_status(struct lifxd_gateway *gw,
                                  const struct lifxd_packet_header *hdr,
                                  const struct lifxd_packet_light_status *pkt)
{
    assert(gw && hdr && pkt);

    lifxd_debug(
        "SET_LIGHT_STATE <-- [%s]:%hu - %s "
        "hue=%#hx, saturation=%#hx, brightness=%#hx, "
        "kelvin=%d, dim=%#hx, power=%#hx, label=%.*s, tags=%#lx",
        gw->ip_addr, gw->port, lifxd_addrtoa(hdr->target.device_addr),
        pkt->hue, pkt->saturation, pkt->brightness, pkt->kelvin,
        pkt->dim, pkt->power, sizeof(pkt->label), pkt->label, pkt->tags
    );

    struct lifxd_bulb *b = lifxd_gateway_get_or_open_bulb(
        gw, hdr->target.device_addr
    );
    if (!b) {
        return;
    }

    assert(sizeof(*pkt) == sizeof(b->state));
    lifxd_bulb_set_light_state(
        b, (const struct lifxd_light_state *)pkt, gw->last_pkt_at
    );

    int latency = gw->last_pkt_at - gw->last_req_at;
    if (latency < LIFXD_GATEWAY_MIN_REFRESH_INTERVAL_MSECS) {
        int timeout = LIFXD_GATEWAY_MIN_REFRESH_INTERVAL_MSECS - latency;
        struct timeval tv = LIFXD_MSECS_TO_TIMEVAL(timeout);
        evtimer_add(gw->refresh_ev, &tv);
        lifxd_debug(
            "[%s]:%hu latency is %dms, scheduling next GET_LIGHT_STATE in %dms",
            gw->ip_addr, gw->port, latency, timeout
        );
        return;
    }

    lifxd_debug(
        "[%s]:%hu latency is %dms, sending GET_LIGHT_STATE now",
        gw->ip_addr, gw->port, latency
    );
    lifxd_gateway_send_get_all_light_state(gw);
}

void
lifxd_gateway_handle_power_state(struct lifxd_gateway *gw,
                                 const struct lifxd_packet_header *hdr,
                                 const struct lifxd_packet_power_state *pkt)
{
    assert(gw && hdr && pkt);

    lifxd_debug(
        "SET_POWER_STATE <-- [%s]:%hu - %s power=%#hx",
        gw->ip_addr, gw->port, lifxd_addrtoa(hdr->target.device_addr), pkt->power
    );

    struct lifxd_bulb *b = lifxd_gateway_get_or_open_bulb(
        gw, hdr->target.device_addr
    );
    if (!b) {
        return;
    }

    lifxd_bulb_set_power_state(b, pkt->power);
}
