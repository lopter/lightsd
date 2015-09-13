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
#include "watchdog.h"
#include "broadcast.h"
#include "core/timer.h"
#include "tagging.h"
#include "core/jsmn.h"
#include "core/jsonrpc.h"
#include "core/client.h"
#include "core/proto.h"
#include "core/router.h"
#include "core/stats.h"
#include "core/daemon.h"
#include "core/lightsd.h"

struct lgtd_lifx_gateway_list lgtd_lifx_gateways =
    LIST_HEAD_INITIALIZER(&lgtd_lifx_gateways);

void
lgtd_lifx_gateway_close(struct lgtd_lifx_gateway *gw)
{
    assert(gw);

    LGTD_STATS_ADD_AND_UPDATE_PROCTITLE(gateways, -1);
    lgtd_timer_stop(gw->refresh_timer);
    event_del(gw->write_ev);
    if (gw->socket != -1) {
        evutil_closesocket(gw->socket);
        LIST_REMOVE(gw, link);
    }
    event_free(gw->write_ev);
    evbuffer_free(gw->write_buf);
    for (int i = 0; i != LGTD_LIFX_GATEWAY_MAX_TAGS; i++) {
        if (gw->tags[i]) {
            lgtd_lifx_tagging_decref(gw->tags[i], gw);
        }
    }
    while (!SLIST_EMPTY(&gw->bulbs)) {
        struct lgtd_lifx_bulb *bulb = SLIST_FIRST(&gw->bulbs);
        lgtd_lifx_gateway_remove_and_close_bulb(gw, bulb);
    }

    char site[LGTD_LIFX_ADDR_STRLEN];
    lgtd_info(
        "connection with gateway bulb %s (site %s) closed",
        gw->peeraddr, LGTD_IEEE8023MACTOA(gw->site.as_array, site)
    );
    free(gw->peer);
    free(gw);
}

void
lgtd_lifx_gateway_remove_and_close_bulb(struct lgtd_lifx_gateway *gw,
                                        struct lgtd_lifx_bulb *bulb)
{
    assert(gw);
    assert(bulb);

    int tag_id;
    LGTD_LIFX_WIRE_FOREACH_TAG_ID(tag_id, bulb->state.tags) {
        assert(gw->tag_refcounts[tag_id] > 0);
        gw->tag_refcounts[tag_id]--;
    }
    SLIST_REMOVE(&gw->bulbs, bulb, lgtd_lifx_bulb, link_by_gw);

    lgtd_lifx_bulb_close(bulb);
}

static void
lgtd_lifx_gateway_write_callback(evutil_socket_t socket,
                                 short events, void *ctx)
{
    (void)socket;

    assert(ctx);

    struct lgtd_lifx_gateway *gw = (struct lgtd_lifx_gateway *)ctx;

    if (events & EV_TIMEOUT) {  // Not sure how that could happen in UDP but eh.
        lgtd_warn("lost connection with gateway bulb %s", gw->peeraddr);
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
            lgtd_warn("can't write to %s", gw->peeraddr);
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
            gw->pkt_ring[gw->pkt_ring_tail].type = 0;
            LGTD_LIFX_GATEWAY_INC_MESSAGE_RING_INDEX(gw->pkt_ring_tail);
            gw->pkt_ring_full = false;
        }

        if (!evbuffer_get_length(gw->write_buf)) {
            event_del(gw->write_ev);
        }
    }
}

static bool
lgtd_lifx_gateway_send_to_site_impl(struct lgtd_lifx_gateway *gw,
                                    enum lgtd_lifx_packet_type pkt_type,
                                    void *pkt,
                                    const struct lgtd_lifx_packet_info **pkt_info)
{
    assert(gw);
    assert(pkt_info);

    struct lgtd_lifx_packet_header hdr;
    union lgtd_lifx_target target = { .addr = gw->site.as_array };
    *pkt_info = lgtd_lifx_wire_setup_header(
        &hdr,
        LGTD_LIFX_TARGET_SITE,
        target,
        gw->site.as_array,
        pkt_type
    );
    assert(*pkt_info);

    lgtd_lifx_gateway_enqueue_packet(gw, &hdr, *pkt_info, pkt);

    return true; // FIXME, have real return values on the send paths...
}

static bool
lgtd_lifx_gateway_send_to_site_quiet(struct lgtd_lifx_gateway *gw,
                                     enum lgtd_lifx_packet_type pkt_type,
                                     void *pkt)
{

    const struct lgtd_lifx_packet_info *pkt_info;
    bool rv = lgtd_lifx_gateway_send_to_site_impl(
        gw, pkt_type, pkt, &pkt_info
    );

    char site[LGTD_LIFX_ADDR_STRLEN];
    lgtd_debug(
        "sending %s to site %s",
        pkt_info->name, LGTD_IEEE8023MACTOA(gw->site.as_array, site)
    );

    return rv; // FIXME, have real return values on the send paths...
}

bool
lgtd_lifx_gateway_send_to_site(struct lgtd_lifx_gateway *gw,
                               enum lgtd_lifx_packet_type pkt_type,
                               void *pkt)
{
    const struct lgtd_lifx_packet_info *pkt_info;
    bool rv = lgtd_lifx_gateway_send_to_site_impl(
        gw, pkt_type, pkt, &pkt_info
    );

    char site[LGTD_LIFX_ADDR_STRLEN];
    lgtd_info(
        "sending %s to site %s",
        pkt_info->name, LGTD_IEEE8023MACTOA(gw->site.as_array, site)
    );

    return rv; // FIXME, have real return values on the send paths...
}

static void
lgtd_lifx_gateway_send_get_all_light_state(struct lgtd_lifx_gateway *gw)
{
    assert(gw);

    lgtd_lifx_gateway_send_to_site_quiet(gw, LGTD_LIFX_GET_LIGHT_STATE, NULL);

    struct lgtd_lifx_packet_tags pkt = { .tags = LGTD_LIFX_ALL_TAGS };
    lgtd_lifx_gateway_send_to_site_quiet(gw, LGTD_LIFX_GET_TAG_LABELS, &pkt);

    gw->pending_refresh_req = true;
}

static void
lgtd_lifx_gateway_refresh_callback(struct lgtd_timer *timer,
                                   union lgtd_timer_ctx ctx)
{
    (void)timer;
    struct lgtd_lifx_gateway *gw = ctx.as_ptr;
    lgtd_lifx_gateway_send_get_all_light_state(gw);
}

void
lgtd_lifx_gateway_force_refresh(struct lgtd_lifx_gateway *gw)
{
    assert(gw);

    lgtd_timer_activate(gw->refresh_timer);
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
            char addr[LGTD_LIFX_ADDR_STRLEN];
            lgtd_info(
                "bulb %s on %s",
                LGTD_IEEE8023MACTOA(bulb->addr, addr), gw->peeraddr
            );
        }
    }
    return bulb;
}

struct lgtd_lifx_gateway *
lgtd_lifx_gateway_open(const struct sockaddr *peer,
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
    gw->socket = socket(peer->sa_family, SOCK_DGRAM, IPPROTO_UDP);
    if (gw->socket == -1) {
        lgtd_warn("can't open a new socket");
        goto error_socket;
    }
    if (connect(gw->socket, peer, addrlen) == -1
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
    if (!gw->write_ev || !gw->write_buf) {
        goto error_allocate;
    }
    gw->peer = malloc(addrlen);
    if (!gw->peer) {
        goto error_allocate;
    }

    memcpy(gw->peer, peer, addrlen);
    gw->peerlen = addrlen;
    LGTD_SOCKADDRTOA(gw->peer, gw->peeraddr);
    memcpy(gw->site.as_array, site, sizeof(gw->site.as_array));
    gw->last_req_at = received_at;
    gw->next_req_at = received_at;
    gw->last_pkt_at = received_at;

    union lgtd_timer_ctx ctx = { .as_ptr = gw };
    gw->refresh_timer = lgtd_timer_start(
        LGTD_TIMER_ACTIVATE_NOW,
        LGTD_LIFX_GATEWAY_MIN_REFRESH_INTERVAL_MSECS,
        lgtd_lifx_gateway_refresh_callback,
        ctx
    );
    if (!gw->refresh_timer) {
        lgtd_warn("can't allocate a new timer");
        goto error_allocate;
    }

    char site_addr[LGTD_LIFX_ADDR_STRLEN];
    lgtd_info(
        "gateway for site %s at %s",
        LGTD_IEEE8023MACTOA(gw->site.as_array, site_addr), gw->peeraddr
    );
    LIST_INSERT_HEAD(&lgtd_lifx_gateways, gw, link);

    // In case this is the first bulb (re-)discovered, start the watchdog, it
    // will stop by itself:
    lgtd_lifx_watchdog_start();

    LGTD_STATS_ADD_AND_UPDATE_PROCTITLE(gateways, 1);

    return gw;

error_allocate:
    lgtd_warn("can't allocate a new gateway bulb");
    if (gw->write_ev) {
        event_free(gw->write_ev);
    }
    if (gw->write_buf) {
        evbuffer_free(gw->write_buf);
    }
error_connect:
    evutil_closesocket(gw->socket);
error_socket:
    free(gw->peer);
    free(gw);
    return NULL;
}

struct lgtd_lifx_gateway *
lgtd_lifx_gateway_get(const struct sockaddr *peer, ev_socklen_t peerlen)
{
    assert(peer);

    struct lgtd_lifx_gateway *gw, *next_gw;
    LIST_FOREACH_SAFE(gw, &lgtd_lifx_gateways, link, next_gw) {
        if (peer->sa_family == gw->peer->sa_family
            && peerlen == gw->peerlen
            && !memcmp(gw->peer, peer, peerlen)) {
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
                                 const struct lgtd_lifx_packet_info *pkt_info,
                                 void *pkt)
{
    assert(gw);
    assert(hdr);
    assert(pkt_info);
    assert(!memcmp(hdr->site, gw->site.as_array, LGTD_LIFX_ADDR_LENGTH));
    assert(gw->pkt_ring_head >= 0);
    assert(gw->pkt_ring_head < (int)LGTD_ARRAY_SIZE(gw->pkt_ring));

    if (gw->pkt_ring_full) {
        lgtd_warnx(
            "dropping packet type %s: packet queue on %s is full",
            pkt_info->name, gw->peeraddr
        );
        return;
    }

    evbuffer_add(gw->write_buf, hdr, sizeof(*hdr));
    if (pkt) {
        assert(pkt_info->size == le16toh(hdr->size) - sizeof(*hdr));
        evbuffer_add(gw->write_buf, pkt, pkt_info->size);
    }
    gw->pkt_ring[gw->pkt_ring_head].size = sizeof(*hdr) + pkt_info->size;
    gw->pkt_ring[gw->pkt_ring_head].type = pkt_info->type;
    LGTD_LIFX_GATEWAY_INC_MESSAGE_RING_INDEX(gw->pkt_ring_head);
    if (gw->pkt_ring_head == gw->pkt_ring_tail) {
        gw->pkt_ring_full = true;
    }
    event_add(gw->write_ev, NULL);
}

void
lgtd_lifx_gateway_update_tag_refcounts(struct lgtd_lifx_gateway *gw,
                                       uint64_t bulb_tags,
                                       uint64_t pkt_tags)
{
    uint64_t changes = bulb_tags ^ pkt_tags;
    uint64_t added_tags = changes & pkt_tags;
    uint64_t removed_tags = changes & bulb_tags;
    int tag_id;

    LGTD_LIFX_WIRE_FOREACH_TAG_ID(tag_id, added_tags) {
        if (gw->tag_refcounts[tag_id] != UINT8_MAX) {
            gw->tag_refcounts[tag_id]++;
        } else {
            lgtd_warnx(
                "reached refcount limit (%u) for tag [%s] (%d) on gw %s",
                UINT8_MAX, gw->tags[tag_id] ? gw->tags[tag_id]->label : NULL,
                tag_id, gw->peeraddr
            );
        }
    }

    LGTD_LIFX_WIRE_FOREACH_TAG_ID(tag_id, removed_tags) {
        assert(gw->tag_refcounts[tag_id] > 0);
        if (--gw->tag_refcounts[tag_id] == 0) {
            char site[LGTD_LIFX_ADDR_STRLEN];
            lgtd_info(
                "deleting unused tag [%s] (%d) from gw %s (site %s)",
                gw->tags[tag_id] ? gw->tags[tag_id]->label : NULL,
                tag_id, gw->peeraddr,
                LGTD_IEEE8023MACTOA(gw->site.as_array, site)
            );
            struct lgtd_lifx_packet_tag_labels pkt = {
                .tags = ~(gw->tag_ids & ~LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(tag_id))
            };
            lgtd_lifx_wire_encode_tag_labels(&pkt);
            lgtd_lifx_gateway_send_to_site(gw, LGTD_LIFX_SET_TAG_LABELS, &pkt);
        }
    }
}

lgtd_time_mono_t
lgtd_lifx_gateway_latency(const struct lgtd_lifx_gateway *gw)
{
    assert(gw);

    if (gw->last_req_at < gw->last_pkt_at) { // this doesn't look right
        return gw->last_pkt_at - gw->last_req_at;
    }
    return 0;
}

void
lgtd_lifx_gateway_handle_pan_gateway(struct lgtd_lifx_gateway *gw,
                                     const struct lgtd_lifx_packet_header *hdr,
                                     const struct lgtd_lifx_packet_pan_gateway *pkt)
{
    assert(gw && hdr && pkt);

    char addr[LGTD_LIFX_ADDR_STRLEN], site[LGTD_LIFX_ADDR_STRLEN];
    lgtd_debug(
        "SET_PAN_GATEWAY <-- %s - %s site=%s, service_type=%d",
        gw->peeraddr,
        LGTD_IEEE8023MACTOA(hdr->target.device_addr, addr),
        LGTD_IEEE8023MACTOA(hdr->site, site), pkt->service_type
    );
}

void
lgtd_lifx_gateway_handle_light_status(struct lgtd_lifx_gateway *gw,
                                      const struct lgtd_lifx_packet_header *hdr,
                                      const struct lgtd_lifx_packet_light_status *pkt)
{
    assert(gw && hdr && pkt);

    char addr[LGTD_LIFX_ADDR_STRLEN];
    lgtd_debug(
        "SET_LIGHT_STATE <-- %s - %s "
        "hue=%#hx, saturation=%#hx, brightness=%#hx, "
        "kelvin=%d, dim=%#hx, power=%#hx, label=%.*s, tags=%#jx",
        gw->peeraddr, LGTD_IEEE8023MACTOA(hdr->target.device_addr, addr),
        pkt->hue, pkt->saturation, pkt->brightness, pkt->kelvin,
        pkt->dim, pkt->power, LGTD_LIFX_LABEL_SIZE, pkt->label,
        (uintmax_t)pkt->tags
    );

    struct lgtd_lifx_bulb *b;
    LGTD_LIFX_GATEWAY_GET_BULB_OR_RETURN(b, gw, hdr->target.device_addr);

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

    lgtd_time_mono_t latency = lgtd_lifx_gateway_latency(gw);
    if (latency < LGTD_LIFX_GATEWAY_MIN_REFRESH_INTERVAL_MSECS) {
        if (!lgtd_timer_ispending(gw->refresh_timer)) {
            int timeout = LGTD_LIFX_GATEWAY_MIN_REFRESH_INTERVAL_MSECS - latency;
            struct timeval tv = LGTD_MSECS_TO_TIMEVAL(timeout);
            lgtd_timer_reschedule(gw->refresh_timer, &tv);
            lgtd_debug(
                "%s latency is %jums, scheduling next GET_LIGHT_STATE in %dms",
                gw->peeraddr, (uintmax_t)latency, timeout
            );
        }
        return;
    }

    if (!gw->pending_refresh_req) {
        lgtd_debug(
            "%s latency is %jums, sending GET_LIGHT_STATE now",
            gw->peeraddr, (uintmax_t)latency
        );
        lgtd_lifx_gateway_send_get_all_light_state(gw);
    } else {
        lgtd_debug(
            "%s GET_LIGHT_STATE for all bulbs on this gw has already "
            "been enqueued", gw->peeraddr
        );
    }
}

void
lgtd_lifx_gateway_handle_power_state(struct lgtd_lifx_gateway *gw,
                                     const struct lgtd_lifx_packet_header *hdr,
                                     const struct lgtd_lifx_packet_power_state *pkt)
{
    assert(gw && hdr && pkt);

    char addr[LGTD_LIFX_ADDR_STRLEN];
    lgtd_debug(
        "SET_POWER_STATE <-- %s - %s power=%#hx", gw->peeraddr,
        LGTD_IEEE8023MACTOA(hdr->target.device_addr, addr), pkt->power
    );

    LGTD_LIFX_GATEWAY_SET_BULB_ATTR(
        gw, hdr->target.device_addr, lgtd_lifx_bulb_set_power_state, pkt->power
    );
}

int
lgtd_lifx_gateway_get_tag_id(const struct lgtd_lifx_gateway *gw,
                             const struct lgtd_lifx_tag *tag)
{
    assert(gw);
    assert(tag);

    int tag_id;
    LGTD_LIFX_WIRE_FOREACH_TAG_ID(tag_id, gw->tag_ids) {
        if (gw->tags[tag_id] == tag) {
            return tag_id;
        }
    }

    return -1;
}

int
lgtd_lifx_gateway_allocate_tag_id(struct lgtd_lifx_gateway *gw,
                                  int tag_id,
                                  const char *tag_label)
{
    assert(gw);
    assert(tag_label);
    assert(tag_id >= -1);
    assert(tag_id < LGTD_LIFX_GATEWAY_MAX_TAGS);

    char site[LGTD_LIFX_ADDR_STRLEN];
    LGTD_IEEE8023MACTOA(gw->site.as_array, site);

    if (tag_id == -1) {
        tag_id = lgtd_lifx_wire_bitscan64_forward(~gw->tag_ids);
        if (tag_id == -1) {
            lgtd_warnx(
                "no tag_id left for new tag [%s] on gw %s (site %s)",
                tag_label, gw->peeraddr, site
            );
            return -1;
        }
    }

    if (!(gw->tag_ids & LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(tag_id))) {
        struct lgtd_lifx_tag *tag;
        tag = lgtd_lifx_tagging_incref(tag_label, gw, tag_id);
        if (!tag) {
            lgtd_warn(
                "couldn't allocate a new reference to tag [%s] (site %s)",
                tag_label, site
            );
            return -1;
        }
        lgtd_debug(
            "tag_id %d allocated for tag [%s] on gw %s (site %s)",
            tag_id, tag_label, gw->peeraddr, site
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
        char site[LGTD_LIFX_ADDR_STRLEN];
        lgtd_debug(
            "tag_id %d deallocated for tag [%s] on gw %s (site %s)",
            tag_id, gw->tags[tag_id]->label,
            gw->peeraddr, LGTD_IEEE8023MACTOA(gw->site.as_array, site)
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

    char addr[LGTD_LIFX_ADDR_STRLEN];
    lgtd_debug(
        "SET_TAG_LABELS <-- %s - %s label=%.*s, tags=%jx",
        gw->peeraddr, LGTD_IEEE8023MACTOA(hdr->target.device_addr, addr),
        LGTD_LIFX_LABEL_SIZE, pkt->label, (uintmax_t)pkt->tags
    );

    int tag_id;
    LGTD_LIFX_WIRE_FOREACH_TAG_ID(tag_id, pkt->tags) {
        if (pkt->label[0]) {
            lgtd_lifx_gateway_allocate_tag_id(gw, tag_id, pkt->label);
        } else if (gw->tags[tag_id]) {
            lgtd_lifx_gateway_deallocate_tag_id(gw, tag_id);
        }
    }
}

void
lgtd_lifx_gateway_handle_tags(struct lgtd_lifx_gateway *gw,
                              const struct lgtd_lifx_packet_header *hdr,
                              const struct lgtd_lifx_packet_tags *pkt)
{
    assert(gw && hdr && pkt);

    char addr[LGTD_LIFX_ADDR_STRLEN];
    lgtd_debug(
        "SET_TAGS <-- %s - %s tags=%#jx",
        gw->peeraddr, LGTD_IEEE8023MACTOA(hdr->target.device_addr, addr),
        (uintmax_t)pkt->tags
    );

    struct lgtd_lifx_bulb *b;
    LGTD_LIFX_GATEWAY_GET_BULB_OR_RETURN(b, gw, hdr->target.device_addr);

    char bulb_addr[LGTD_LIFX_ADDR_STRLEN], site_addr[LGTD_LIFX_ADDR_STRLEN];
    int tag_id;
    LGTD_LIFX_WIRE_FOREACH_TAG_ID(tag_id, pkt->tags) {
        if (!(gw->tag_ids & LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(tag_id))) {
            lgtd_warnx(
                "trying to set unknown tag_id %d (%#jx) "
                "on bulb %s (%.*s), gw %s (site %s)",
                tag_id, LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(tag_id),
                LGTD_IEEE8023MACTOA(b->addr, bulb_addr),
                LGTD_LIFX_LABEL_SIZE, b->state.label, gw->peeraddr,
                LGTD_IEEE8023MACTOA(gw->site.as_array, site_addr)
            );
        }
    }

    lgtd_lifx_bulb_set_tags(b, pkt->tags);
}

void
lgtd_lifx_gateway_handle_ip_state(struct lgtd_lifx_gateway *gw,
                                  const struct lgtd_lifx_packet_header *hdr,
                                  const struct lgtd_lifx_packet_ip_state *pkt)
{
    assert(gw && hdr && pkt);

    const char  *type;
    enum lgtd_lifx_bulb_ips ip_id;
    switch (hdr->packet_type) {
    case LGTD_LIFX_MESH_INFO:
        type = "MCU_STATE";
        ip_id = LGTD_LIFX_BULB_MCU_IP;
        break;
    case LGTD_LIFX_WIFI_INFO:
        type = "WIFI_STATE";
        ip_id = LGTD_LIFX_BULB_WIFI_IP;
        break;
    default:
        lgtd_info("invalid ip state packet_type %#hx", hdr->packet_type);
#ifndef NDEBUG
        abort();
#endif
        return;
    }

    char addr[LGTD_LIFX_ADDR_STRLEN];
    lgtd_debug(
        "%s <-- %s - %s "
        "signal_strength=%f, rx_bytes=%u, tx_bytes=%u, temperature=%hu",
        type, gw->peeraddr, LGTD_IEEE8023MACTOA(hdr->target.device_addr, addr),
        pkt->signal_strength, pkt->rx_bytes, pkt->tx_bytes, pkt->temperature
    );

    LGTD_LIFX_GATEWAY_SET_BULB_ATTR(
        gw, hdr->target.device_addr, lgtd_lifx_bulb_set_ip_state,
        ip_id, (const struct lgtd_lifx_ip_state *)pkt, gw->last_pkt_at
    );
}

void
lgtd_lifx_gateway_handle_ip_firmware_info(struct lgtd_lifx_gateway *gw,
                                          const struct lgtd_lifx_packet_header *hdr,
                                          const struct lgtd_lifx_packet_ip_firmware_info *pkt)
{
    assert(gw && hdr && pkt);

    const char  *type;
    enum lgtd_lifx_bulb_ips ip_id;
    switch (hdr->packet_type) {
    case LGTD_LIFX_MESH_FIRMWARE:
        type = "MCU_FIRMWARE_INFO";
        ip_id = LGTD_LIFX_BULB_MCU_IP;
        break;
    case LGTD_LIFX_WIFI_FIRMWARE_STATE:
        type = "WIFI_FIRMWARE_INFO";
        ip_id = LGTD_LIFX_BULB_WIFI_IP;
        break;
    default:
        lgtd_info("invalid ip firmware packet_type %#hx", hdr->packet_type);
#ifndef NDEBUG
        abort();
#endif
        return;
    }

    char built_at[64], installed_at[64], addr[LGTD_LIFX_ADDR_STRLEN];
    lgtd_debug(
        "%s <-- %s - %s "
        "built_at=%s, installed_at=%s, version=%u",
        type, gw->peeraddr, LGTD_IEEE8023MACTOA(hdr->target.device_addr, addr),
        LGTD_LIFX_WIRE_PRINT_NSEC_TIMESTAMP(pkt->built_at, built_at),
        LGTD_LIFX_WIRE_PRINT_NSEC_TIMESTAMP(pkt->installed_at, installed_at),
        pkt->version
    );

    LGTD_LIFX_GATEWAY_SET_BULB_ATTR(
        gw, hdr->target.device_addr, lgtd_lifx_bulb_set_ip_firmware_info,
        ip_id, (const struct lgtd_lifx_ip_firmware_info *)pkt, gw->last_pkt_at
    );
}

void
lgtd_lifx_gateway_handle_product_info(struct lgtd_lifx_gateway *gw,
                                      const struct lgtd_lifx_packet_header *hdr,
                                      const struct lgtd_lifx_packet_product_info *pkt)
{
    assert(gw && hdr && pkt);

    char addr[LGTD_LIFX_ADDR_STRLEN];
    lgtd_debug(
        "PRODUCT_INFO <-- %s - %s "
        "vendor_id=%#x, product_id=%#x, version=%u",
        gw->peeraddr, LGTD_IEEE8023MACTOA(hdr->target.device_addr, addr),
        pkt->vendor_id, pkt->product_id, pkt->version
    );

    LGTD_LIFX_GATEWAY_SET_BULB_ATTR(
        gw, hdr->target.device_addr, lgtd_lifx_bulb_set_product_info,
        (const struct lgtd_lifx_product_info *)pkt
    );
}

void
lgtd_lifx_gateway_handle_runtime_info(struct lgtd_lifx_gateway *gw,
                                      const struct lgtd_lifx_packet_header *hdr,
                                      const struct lgtd_lifx_packet_runtime_info *pkt)
{
    assert(gw && hdr && pkt);

    char device_time[64], uptime[64], downtime[64], addr[LGTD_LIFX_ADDR_STRLEN];
    lgtd_debug(
        "PRODUCT_INFO <-- %s - %s time=%s, uptime=%s, downtime=%s",
        gw->peeraddr, LGTD_IEEE8023MACTOA(hdr->target.device_addr, addr),
        LGTD_LIFX_WIRE_PRINT_NSEC_TIMESTAMP(pkt->time, device_time),
        LGTD_PRINT_DURATION(LGTD_NSECS_TO_SECS(pkt->uptime), uptime),
        LGTD_PRINT_DURATION(LGTD_NSECS_TO_SECS(pkt->downtime), downtime)
    );

    LGTD_LIFX_GATEWAY_SET_BULB_ATTR(
        gw, hdr->target.device_addr, lgtd_lifx_bulb_set_runtime_info,
        (const struct lgtd_lifx_runtime_info *)pkt, gw->last_pkt_at
    );
}

void
lgtd_lifx_gateway_handle_bulb_label(struct lgtd_lifx_gateway *gw,
                                    const struct lgtd_lifx_packet_header *hdr,
                                    const struct lgtd_lifx_packet_label *pkt)
{
    assert(gw && hdr && pkt);

    char addr[LGTD_LIFX_ADDR_STRLEN];
    lgtd_debug(
        "BULB_LABEL <-- %s - %s label=%.*s",
        gw->peeraddr, LGTD_IEEE8023MACTOA(hdr->target.device_addr, addr),
        (int)sizeof(pkt->label), pkt->label
    );

    LGTD_LIFX_GATEWAY_SET_BULB_ATTR(
        gw, hdr->target.device_addr, lgtd_lifx_bulb_set_label, pkt->label
    );
}

void
lgtd_lifx_gateway_handle_ambient_light(struct lgtd_lifx_gateway *gw,
                                       const struct lgtd_lifx_packet_header *hdr,
                                       const struct lgtd_lifx_packet_ambient_light *pkt)
{
    assert(gw && hdr && pkt);

    char addr[LGTD_LIFX_ADDR_STRLEN];
    lgtd_debug(
        "AMBIENT_LIGHT <-- %s - %s ambient_light=%flx",
        gw->peeraddr, LGTD_IEEE8023MACTOA(hdr->target.device_addr, addr),
        pkt->illuminance
    );

    LGTD_LIFX_GATEWAY_SET_BULB_ATTR(
        gw, hdr->target.device_addr,
        lgtd_lifx_bulb_set_ambient_light, pkt->illuminance
    );
}
