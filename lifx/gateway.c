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
#include "core/time_monotonic.h"
#include "bulb.h"
#include "gateway.h"
#include "broadcast.h"
#include "timer.h"
#include "tagging.h"
#include "core/jsmn.h"
#include "core/jsonrpc.h"
#include "core/client.h"
#include "core/proto.h"
#include "core/router.h"
#include "core/lightsd.h"

struct lgtd_lifx_gateway_list lgtd_lifx_gateways =
    LIST_HEAD_INITIALIZER(&lgtd_lifx_gateways);

// Kim Walisch (2012)
// http://chessprogramming.wikispaces.com/BitScan#DeBruijnMultiplation
static inline int
lgtd_lifx_bitscan64_forward(uint64_t n)
{
    enum { DEBRUIJN_NUMBER = 0x03f79d71b4cb0a89 };
    static const int DEBRUIJN_SEQUENCE[64] = {
        0, 47,  1, 56, 48, 27,  2, 60,
       57, 49, 41, 37, 28, 16,  3, 61,
       54, 58, 35, 52, 50, 42, 21, 44,
       38, 32, 29, 23, 17, 11,  4, 62,
       46, 55, 26, 59, 40, 36, 15, 53,
       34, 51, 20, 43, 31, 22, 10, 45,
       25, 39, 14, 33, 19, 30,  9, 24,
       13, 18,  8, 12,  7,  6,  5, 63
    };

    return n ? DEBRUIJN_SEQUENCE[((n ^ (n - 1)) * DEBRUIJN_NUMBER) >> 58] : -1;
}

void
lgtd_lifx_gateway_close(struct lgtd_lifx_gateway *gw)
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
    for (int i = 0; i != LGTD_LIFX_GATEWAY_MAX_TAGS; i++) {
        if (gw->tags[i]) {
            lgtd_lifx_tagging_decref(gw->tags[i], gw);
        }
    }
    struct lgtd_lifx_bulb *bulb, *next_bulb;
    SLIST_FOREACH_SAFE(bulb, &gw->bulbs, link_by_gw, next_bulb) {
        lgtd_lifx_bulb_close(bulb);
    }

    lgtd_info(
        "connection with gateway bulb [%s]:%hu (site %s) closed",
        gw->ip_addr, gw->port, lgtd_addrtoa(gw->site.as_array)
    );
    free(gw);
}

static void
lgtd_lifx_gateway_write_callback(evutil_socket_t socket,
                                 short events, void *ctx)
{
    (void)socket;

    assert(ctx);

    struct lgtd_lifx_gateway *gw = (struct lgtd_lifx_gateway *)ctx;

    if (events & EV_TIMEOUT) {  // Not sure how that could happen in UDP but eh.
        lgtd_warn(
            "lost connection with gateway bulb [%s]:%hu", gw->ip_addr, gw->port
        );
        lgtd_lifx_gateway_close(gw);
        if (!lgtd_lifx_broadcast_discovery()) {
            lgtd_err(1, "can't start auto discovery");
        }
        return;
    }

    if (events & EV_WRITE) {
        assert(gw->pkt_ring_tail >= 0);
        assert(gw->pkt_ring_tail < (int)LGTD_ARRAY_SIZE(gw->pkt_ring));

        int to_write = gw->pkt_ring[gw->pkt_ring_tail].size;
        int nbytes = evbuffer_write_atmost(gw->write_buf, gw->socket, to_write);
        if (nbytes == -1 && errno != EAGAIN) {
            lgtd_warn("can't write to [%s]:%hu", gw->ip_addr, gw->port);
            lgtd_lifx_gateway_close(gw);
            if (!lgtd_lifx_broadcast_discovery()) {
                lgtd_err(1, "can't start auto discovery");
            }
            return;
        }

        // Callbacks are called in any order, so we keep two timers to make
        // sure we can get the latency right, otherwise we could be compute the
        // latency with last_pkt_at < last_req_at, which isn't true since the
        // pkt will be for an answer to the previous write:
        gw->last_req_at = gw->next_req_at;
        gw->next_req_at = lgtd_time_monotonic_msecs();

        gw->pkt_ring[gw->pkt_ring_tail].size -= nbytes;
        if (gw->pkt_ring[gw->pkt_ring_tail].size == 0) {
            enum lgtd_lifx_packet_type type;
            type = gw->pkt_ring[gw->pkt_ring_tail].type;
            if (type == LGTD_LIFX_GET_TAG_LABELS) {
                gw->pending_refresh_req = false;
            }
            if (lgtd_opts.verbosity <= LGTD_DEBUG) {
                const struct lgtd_lifx_packet_infos *pkt_infos =
                    lgtd_lifx_wire_get_packet_infos(type);
                lgtd_debug(
                    "%s --> [%s]:%hu", pkt_infos->name, gw->ip_addr, gw->port
                );
            }
            gw->pkt_ring[gw->pkt_ring_tail].type = 0;
            LGTD_LIFX_GATEWAY_INC_MESSAGE_RING_INDEX(gw->pkt_ring_tail);
            gw->pkt_ring_full = false;
        }

        if (!evbuffer_get_length(gw->write_buf)) {
            event_del(gw->write_ev);
        }
    }
}

static void
lgtd_lifx_gateway_send_get_all_light_state(struct lgtd_lifx_gateway *gw)
{
    assert(gw);

    struct lgtd_lifx_packet_header hdr;
    union lgtd_lifx_target target = { .addr = gw->site.as_array };

    lgtd_lifx_wire_setup_header(
        &hdr,
        LGTD_LIFX_TARGET_SITE,
        target,
        gw->site.as_array,
        LGTD_LIFX_GET_LIGHT_STATE
    );
    lgtd_lifx_gateway_enqueue_packet(
        gw, &hdr, LGTD_LIFX_GET_LIGHT_STATE, NULL, 0
    );

    struct lgtd_lifx_packet_get_tag_labels pkt = { .tags = LGTD_LIFX_ALL_TAGS };
    lgtd_lifx_wire_setup_header(
        &hdr,
        LGTD_LIFX_TARGET_SITE,
        target,
        gw->site.as_array,
        LGTD_LIFX_GET_TAG_LABELS
    );
    lgtd_lifx_gateway_enqueue_packet(
        gw, &hdr, LGTD_LIFX_GET_TAG_LABELS, &pkt, sizeof(pkt)
    );

    gw->pending_refresh_req = true;
}

static void
lgtd_lifx_gateway_refresh_callback(evutil_socket_t socket,
                                   short events,
                                   void *ctx)
{
    (void)socket;
    (void)events;
    lgtd_lifx_gateway_send_get_all_light_state((struct lgtd_lifx_gateway *)ctx);
}

void
lgtd_lifx_gateway_force_refresh(struct lgtd_lifx_gateway *gw)
{
    assert(gw);

    event_active(gw->refresh_ev, 0, 0);
}

static struct lgtd_lifx_bulb *
lgtd_lifx_gateway_get_or_open_bulb(struct lgtd_lifx_gateway *gw,
                                   const uint8_t *bulb_addr)
{
    assert(gw);
    assert(bulb_addr);

    struct lgtd_lifx_bulb *bulb = lgtd_lifx_bulb_get(bulb_addr);
    if (!bulb) {
        bulb = lgtd_lifx_bulb_open(gw, bulb_addr);
        if (bulb) {
            SLIST_INSERT_HEAD(&gw->bulbs, bulb, link_by_gw);
            lgtd_info(
                "bulb %s on [%s]:%hu",
                lgtd_addrtoa(bulb_addr), gw->ip_addr, gw->port
            );
        }
    }
    return bulb;
}

struct lgtd_lifx_gateway *
lgtd_lifx_gateway_open(const struct sockaddr_storage *peer,
                       ev_socklen_t addrlen,
                       const uint8_t *site,
                       lgtd_time_mono_t received_at)
{
    assert(peer);
    assert(site);

    struct lgtd_lifx_gateway *gw = calloc(1, sizeof(*gw));
    if (!gw) {
        lgtd_warn("can't allocate a new gateway bulb");
        return false;
    }
    gw->socket = socket(peer->ss_family, SOCK_DGRAM, IPPROTO_UDP);
    if (gw->socket == -1) {
        lgtd_warn("can't open a new socket");
        goto error_socket;
    }
    if (connect(gw->socket, (const struct sockaddr *)peer, addrlen) == -1
        || evutil_make_socket_nonblocking(gw->socket) == -1) {
        lgtd_warn("can't open a new socket");
        goto error_connect;
    }
    gw->write_ev = event_new(
        lgtd_ev_base,
        gw->socket,
        EV_WRITE|EV_PERSIST,
        lgtd_lifx_gateway_write_callback,
        gw
    );
    gw->write_buf = evbuffer_new();
    gw->refresh_ev = evtimer_new(
        lgtd_ev_base, lgtd_lifx_gateway_refresh_callback, gw
    );
    memcpy(&gw->peer, peer, sizeof(gw->peer));
    lgtd_sockaddrtoa(peer, gw->ip_addr, sizeof(gw->ip_addr));
    gw->port = lgtd_sockaddrport(peer);
    memcpy(gw->site.as_array, site, sizeof(gw->site.as_array));
    gw->last_req_at = received_at;
    gw->next_req_at = received_at;
    gw->last_pkt_at = received_at;

    struct timeval refresh_interval = LGTD_MSECS_TO_TIMEVAL(
        LGTD_LIFX_GATEWAY_MIN_REFRESH_INTERVAL_MSECS
    );

    if (!gw->write_ev || !gw->write_buf || !gw->refresh_ev
        || event_add(gw->refresh_ev, &refresh_interval) != 0) {
        lgtd_warn("can't allocate a new gateway bulb");
        goto error_allocate;
    }

    lgtd_info(
        "gateway for site %s at [%s]:%hu",
        lgtd_addrtoa(gw->site.as_array), gw->ip_addr, gw->port
    );
    LIST_INSERT_HEAD(&lgtd_lifx_gateways, gw, link);

    // In case this is the first bulb (re-)discovered, start the watchdog, it
    // will stop by itself:
    lgtd_lifx_timer_start_watchdog();

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

struct lgtd_lifx_gateway *
lgtd_lifx_gateway_get(const struct sockaddr_storage *peer)
{
    assert(peer);

    struct lgtd_lifx_gateway *gw, *next_gw;
    LIST_FOREACH_SAFE(gw, &lgtd_lifx_gateways, link, next_gw) {
        if (peer->ss_family == gw->peer.ss_family
            && !memcmp(&gw->peer, peer, sizeof(*peer))) {
            return gw;
        }
    }

    return NULL;
}

void
lgtd_lifx_gateway_close_all(void)
{
    struct lgtd_lifx_gateway *gw, *next_gw;
    LIST_FOREACH_SAFE(gw, &lgtd_lifx_gateways, link, next_gw) {
        lgtd_lifx_gateway_close(gw);
    }
}

void
lgtd_lifx_gateway_enqueue_packet(struct lgtd_lifx_gateway *gw,
                                 const struct lgtd_lifx_packet_header *hdr,
                                 enum lgtd_lifx_packet_type pkt_type,
                                 const void *pkt,
                                 int pkt_size)
{
    assert(gw);
    assert(hdr);
    assert(pkt_size >= 0 && pkt_size < LGTD_LIFX_MAX_PACKET_SIZE);
    assert(!memcmp(hdr->site, gw->site.as_array, LGTD_LIFX_ADDR_LENGTH));
    assert(gw->pkt_ring_head >= 0);
    assert(gw->pkt_ring_head < (int)LGTD_ARRAY_SIZE(gw->pkt_ring));

    if (gw->pkt_ring_full) {
        lgtd_warnx(
            "dropping packet type %s: packet queue on [%s]:%hu is full",
            lgtd_lifx_wire_get_packet_infos(pkt_type)->name,
            gw->ip_addr, gw->port
        );
        return;
    }

    evbuffer_add(gw->write_buf, hdr, sizeof(*hdr));
    if (pkt) {
        assert((unsigned)pkt_size == le16toh(hdr->size) - sizeof(*hdr));
        evbuffer_add(gw->write_buf, pkt, pkt_size);
    }
    gw->pkt_ring[gw->pkt_ring_head].size = sizeof(*hdr) + pkt_size;
    gw->pkt_ring[gw->pkt_ring_head].type = pkt_type;
    LGTD_LIFX_GATEWAY_INC_MESSAGE_RING_INDEX(gw->pkt_ring_head);
    if (gw->pkt_ring_head == gw->pkt_ring_tail) {
        gw->pkt_ring_full = true;
    }
    event_add(gw->write_ev, NULL);
}

void
lgtd_lifx_gateway_handle_pan_gateway(struct lgtd_lifx_gateway *gw,
                                     const struct lgtd_lifx_packet_header *hdr,
                                     const struct lgtd_lifx_packet_pan_gateway *pkt)
{
    assert(gw && hdr && pkt);

    lgtd_debug(
        "SET_PAN_GATEWAY <-- [%s]:%hu - %s site=%s",
        gw->ip_addr, gw->port,
        lgtd_addrtoa(hdr->target.device_addr),
        lgtd_addrtoa(hdr->site)
    );
}

void
lgtd_lifx_gateway_handle_light_status(struct lgtd_lifx_gateway *gw,
                                      const struct lgtd_lifx_packet_header *hdr,
                                      const struct lgtd_lifx_packet_light_status *pkt)
{
    assert(gw && hdr && pkt);

    lgtd_debug(
        "SET_LIGHT_STATE <-- [%s]:%hu - %s "
        "hue=%#hx, saturation=%#hx, brightness=%#hx, "
        "kelvin=%d, dim=%#hx, power=%#hx, label=%.*s, tags=%#jx",
        gw->ip_addr, gw->port, lgtd_addrtoa(hdr->target.device_addr),
        pkt->hue, pkt->saturation, pkt->brightness, pkt->kelvin,
        pkt->dim, pkt->power, LGTD_LIFX_LABEL_SIZE, pkt->label,
        (uintmax_t)pkt->tags
    );

    struct lgtd_lifx_bulb *b = lgtd_lifx_gateway_get_or_open_bulb(
        gw, hdr->target.device_addr
    );
    if (!b) {
        return;
    }

    assert(sizeof(*pkt) == sizeof(b->state));
    lgtd_lifx_bulb_set_light_state(
        b, (const struct lgtd_lifx_light_state *)pkt, gw->last_pkt_at
    );

    if (b->dirty_at
        && b->last_light_state_at > b->dirty_at
        && b->gw->last_pkt_at - b->dirty_at > 400) {
        if (b->expected_power_on == b->state.power) {
            lgtd_debug("clearing dirty_at on %s", b->state.label);
            b->dirty_at = 0;
        } else {
            lgtd_info(
                "retransmiting power %s to %s",
                b->expected_power_on ? "on" : "off", b->state.label
            );
            struct lgtd_lifx_packet_power_state pkt;
            pkt.power = b->expected_power_on ?
                    LGTD_LIFX_POWER_ON : LGTD_LIFX_POWER_OFF;
            lgtd_router_send_to_device(b, LGTD_LIFX_SET_POWER_STATE, &pkt);
        }
    }

    int latency = gw->last_pkt_at - gw->last_req_at;
    if (latency < LGTD_LIFX_GATEWAY_MIN_REFRESH_INTERVAL_MSECS) {
        if (!event_pending(gw->refresh_ev, EV_TIMEOUT, NULL)) {
            int timeout = LGTD_LIFX_GATEWAY_MIN_REFRESH_INTERVAL_MSECS - latency;
            struct timeval tv = LGTD_MSECS_TO_TIMEVAL(timeout);
            evtimer_add(gw->refresh_ev, &tv);
            lgtd_debug(
                "[%s]:%hu latency is %dms, scheduling next GET_LIGHT_STATE in %dms",
                gw->ip_addr, gw->port, latency, timeout
            );
        }
        return;
    }

    if (!gw->pending_refresh_req) {
        lgtd_debug(
            "[%s]:%hu latency is %dms, sending GET_LIGHT_STATE now",
            gw->ip_addr, gw->port, latency
        );
        lgtd_lifx_gateway_send_get_all_light_state(gw);
    } else {
        lgtd_debug(
            "[%s]:%hu GET_LIGHT_STATE for all bulbs on this gw has already "
            "been enqueued", gw->ip_addr, gw->port
        );
    }
}

void
lgtd_lifx_gateway_handle_power_state(struct lgtd_lifx_gateway *gw,
                                     const struct lgtd_lifx_packet_header *hdr,
                                     const struct lgtd_lifx_packet_power_state *pkt)
{
    assert(gw && hdr && pkt);

    lgtd_debug(
        "SET_POWER_STATE <-- [%s]:%hu - %s power=%#hx",
        gw->ip_addr, gw->port, lgtd_addrtoa(hdr->target.device_addr), pkt->power
    );

    struct lgtd_lifx_bulb *b = lgtd_lifx_gateway_get_or_open_bulb(
        gw, hdr->target.device_addr
    );
    if (!b) {
        return;
    }

    lgtd_lifx_bulb_set_power_state(b, pkt->power);
}

int
lgtd_lifx_gateway_allocate_tag_id(struct lgtd_lifx_gateway *gw,
                                  int tag_id,
                                  const char *tag_label)
{
    assert(gw);
    assert(tag_label);
    // allocating a new tag_id (tag_id == -1) isn't supported yet:
    assert(tag_id >= 0);
    assert(tag_id < LGTD_LIFX_GATEWAY_MAX_TAGS);

    if (!(gw->tag_ids & LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(tag_id))) {
        struct lgtd_lifx_tag *tag;
        tag = lgtd_lifx_tagging_incref(tag_label, gw, tag_id);
        if (!tag) {
            lgtd_warn(
                "couldn't allocate a new reference to tag [%s] (site %s)",
                tag_label, lgtd_addrtoa(gw->site.as_array)
            );
            return -1;
        }
        lgtd_debug(
            "tag_id %d allocated for tag [%s] on gw [%s]:%hu (site %s)",
            tag_id, tag_label, gw->ip_addr, gw->port,
            lgtd_addrtoa(gw->site.as_array)
        );
        gw->tag_ids |= LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(tag_id);
        gw->tags[tag_id] = tag;
    }

    return tag_id;
}

void
lgtd_lifx_gateway_deallocate_tag_id(struct lgtd_lifx_gateway *gw, int tag_id)
{
    assert(gw);
    assert(tag_id >= 0);
    assert(tag_id < LGTD_LIFX_GATEWAY_MAX_TAGS);

    if (gw->tag_ids & LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(tag_id)) {
        lgtd_debug(
            "tag_id %d deallocated for tag [%s] on gw [%s]:%hu (site %s)",
            tag_id, gw->tags[tag_id]->label,
            gw->ip_addr, gw->port,
            lgtd_addrtoa(gw->site.as_array)
        );
        lgtd_lifx_tagging_decref(gw->tags[tag_id], gw);
        gw->tag_ids &= ~LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(tag_id);
        gw->tags[tag_id] = NULL;
    }
}

void
lgtd_lifx_gateway_handle_tag_labels(struct lgtd_lifx_gateway *gw,
                                    const struct lgtd_lifx_packet_header *hdr,
                                    const struct lgtd_lifx_packet_tag_labels *pkt)
{
    assert(gw && hdr && pkt);

    lgtd_debug(
        "SET_TAG_LABELS <-- [%s]:%hu - %s label=%s, tags=%jx",
        gw->ip_addr, gw->port, lgtd_addrtoa(hdr->target.device_addr),
        pkt->label, (uintmax_t)pkt->tags
    );

    uint64_t tags = pkt->tags;
    while (true) {
        int tag_id = lgtd_lifx_bitscan64_forward(tags);
        if (tag_id == -1) {
            break;
        }
        if (pkt->label[0]) {
            lgtd_lifx_gateway_allocate_tag_id(gw, tag_id, pkt->label);
        } else if (gw->tags[tag_id]) {
            lgtd_lifx_gateway_deallocate_tag_id(gw, tag_id);
        }
        tags &= ~LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(tag_id);
    }
}
