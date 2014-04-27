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
#include <err.h>
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
#include "discovery.h"
#include "lifxd.h"

static struct lifxd_gateway_list lifxd_gateways = \
    LIST_HEAD_INITIALIZER(&lifxd_gateways);

static struct lifxd_packet_infos_map lifxd_packet_infos = \
    RB_INITIALIZER(&lifxd_packets_infos);

RB_GENERATE_STATIC(
    lifxd_packet_infos_map,
    lifxd_packet_infos,
    link,
    lifxd_packet_infos_cmp
);

void
lifxd_gateway_load_packet_infos_map(void)
{
#define DECODER(x)  ((void (*)(void *))(x))
#define ENCODER(x)  ((void (*)(void *))(x))
#define HANDLER(x)                                  \
    ((void (*)(struct lifxd_gateway *,              \
               const struct lifxd_packet_header *,  \
               const void *))(x))

    static struct lifxd_packet_infos packet_table[] = {
        {
            .name = "GET_PAN_GATEWAY",
            .type = LIFXD_GET_PAN_GATEWAY
        },
        {
            .name = "PAN_GATEWAY",
            .type = LIFXD_PAN_GATEWAY,
            .size = sizeof(struct lifxd_packet_pan_gateway),
            .decode = DECODER(lifxd_wire_decode_pan_gateway),
            .encode = ENCODER(lifxd_wire_encode_pan_gateway),
            .handle = HANDLER(lifxd_gateway_handle_pan_gateway)
        },
        {
            .name = "LIGHT_STATUS",
            .type = LIFXD_LIGHT_STATUS,
            .size = sizeof(struct lifxd_packet_light_status),
            .decode = DECODER(lifxd_wire_decode_light_status),
            .encode = ENCODER(lifxd_wire_encode_light_status),
            .handle = HANDLER(lifxd_gateway_handle_light_status)
        },
        {
            .name = "POWER_STATE",
            .type = LIFXD_POWER_STATE,
            .size = sizeof(struct lifxd_packet_power_state),
            .decode = DECODER(lifxd_wire_decode_power_state),
            .handle = HANDLER(lifxd_gateway_handle_power_state)
        }
    };

    for (int i = 0; i != LIFXD_ARRAY_SIZE(packet_table); ++i) {
        RB_INSERT(
            lifxd_packet_infos_map, &lifxd_packet_infos, &packet_table[i]
        );
    }
}

const struct lifxd_packet_infos *
lifxd_gateway_get_packet_infos(enum lifxd_packet_type packet_type)
{
    struct lifxd_packet_infos pkt_infos = { .type = packet_type };
    return RB_FIND(lifxd_packet_infos_map, &lifxd_packet_infos, &pkt_infos);
}

static void
lifxd_gateway_close(struct lifxd_gateway *gw)
{
    assert(gw);
    assert(gw->io);

    int sockfd = bufferevent_getfd(gw->io);
    if (sockfd != -1) {
        evutil_closesocket(sockfd);
        LIST_REMOVE(gw, link);
    }
    bufferevent_free(gw->io);
    for (struct lifxd_bulb *bulb = lifxd_bulb_get(gw, NULL);
         bulb;
         bulb = lifxd_bulb_get(gw, NULL)) {
        lifxd_bulb_close(bulb);
    }
    lifxd_info(
        "connection with gateway bulb [%s]:%hu closed",
        gw->hostname,
        gw->port
    );
    free(gw->hostname);
    free(gw);
}

static void
lifxd_gateway_event_callback(struct bufferevent *bev, short events, void *ctx)
{
    assert(ctx);

    struct lifxd_gateway *gw = (struct lifxd_gateway *)ctx;
    if (events & BEV_EVENT_CONNECTED) {
        lifxd_info(
            "connected to gateway bulb: [%s]:%hu", gw->hostname, gw->port
        );
        LIST_INSERT_HEAD(&lifxd_gateways, gw, link);
        bufferevent_enable(bev, EV_READ|EV_WRITE|EV_TIMEOUT);
    } else if (events & (BEV_EVENT_ERROR|BEV_EVENT_EOF)) {
        if (events & BEV_EVENT_ERROR) {
            int gai_error = bufferevent_socket_get_dns_error(gw->io);
            if (gai_error) {
                lifxd_warnx(
                    "can't connect to %s: %s",
                    gw->hostname,
                    evutil_gai_strerror(gai_error)
                );
            } else {
                lifxd_warn(
                    "lost connection with gateway bulb [%s]:%hu",
                    gw->hostname,
                    gw->port
                );
            }
        }
        lifxd_gateway_close(gw);
        if (!lifxd_discovery_start()) {
            lifxd_err(1, "can't start auto discovery");
        }
    }
}

static void
lifxd_gateway_data_read_callback(struct bufferevent *bev, void *ctx)
{
    assert(ctx);

    const struct lifxd_packet_infos *pkt_infos = NULL;
    struct lifxd_gateway *gw = (struct lifxd_gateway *)ctx;
    struct lifxd_packet_header *cur_hdr = &gw->cur_hdr;

    if (gw->cur_hdr_offset != LIFXD_PACKET_HEADER_SIZE) {
        gw->cur_hdr_offset += bufferevent_read(
            bev,
            ((void *)cur_hdr) + gw->cur_hdr_offset,
            LIFXD_PACKET_HEADER_SIZE - gw->cur_hdr_offset
        );
        if (gw->cur_hdr_offset == LIFXD_PACKET_HEADER_SIZE) {
            lifxd_wire_decode_header(cur_hdr);
            lifxd_debug(
                "received header from [%s]:%hu for packet type %#x",
                gw->hostname, gw->port, cur_hdr->packet_type
            );
            gw->cur_pkt_size = cur_hdr->size - LIFXD_PACKET_HEADER_SIZE;
            if (gw->cur_pkt_size > LIFXD_MAX_PACKET_SIZE) {
                lifxd_warnx(
                    "unsupported packet size %hu (max = %d, packet_type = %#x) "
                    "from [%s]:%hu",
                    gw->cur_pkt_size,
                    LIFXD_MAX_PACKET_SIZE,
                    cur_hdr->packet_type,
                    gw->hostname,
                    gw->port
                );
                goto drop_gateway;
            }
            if (gw->cur_pkt_size) {
                gw->cur_pkt = calloc(1, gw->cur_pkt_size);
                if (!gw->cur_pkt) {
                    lifxd_warn("can't allocate memory for a packet");
                    goto drop_gateway;
                }
            }
        }
    }

    if (gw->cur_hdr_offset == LIFXD_PACKET_HEADER_SIZE) {
        if (gw->cur_pkt_offset != gw->cur_pkt_size) {
            gw->cur_pkt_offset += bufferevent_read(
                bev,
                gw->cur_pkt + gw->cur_pkt_offset,
                gw->cur_pkt_size - gw->cur_pkt_offset
            );
        }
        if (gw->cur_pkt_offset == gw->cur_pkt_size) {
            pkt_infos = lifxd_gateway_get_packet_infos(cur_hdr->packet_type);
            if (pkt_infos) {
                pkt_infos->decode(gw->cur_pkt);
                pkt_infos->handle(gw, cur_hdr, gw->cur_pkt);
            } else {
                lifxd_warnx("discarding unknown packet %#x from [%s]:%hu",
                    cur_hdr->packet_type, gw->hostname, gw->port
                );
            }
            free(gw->cur_pkt);
            gw->cur_pkt = NULL;
            gw->cur_pkt_size = 0;
            gw->cur_pkt_offset = 0;
            gw->cur_hdr_offset = 0;
        }
    }

    return;

drop_gateway:
    lifxd_gateway_close(gw);
    if (!lifxd_discovery_start()) {
        lifxd_err(1, "can't start auto discovery");
    }
}


struct lifxd_gateway *
lifxd_gateway_open(const char *hostname, uint16_t port, const uint8_t *site)
{
    assert(hostname);
    assert(port < UINT16_MAX);
    assert(port > 0);

    if (!site) {
        lifxd_warnx("connecting directly to a bulb isn't supported yet");
        return NULL;
    }

    struct lifxd_gateway *gw = calloc(1, sizeof(*gw));
    if (!gw) {
        lifxd_warn("can't allocate a new gateway bulb");
        return false;
    }
    gw->io = bufferevent_socket_new(lifxd_ev_base, -1, 0);
    if (!gw->io) {
        lifxd_warn("can't allocate a new gateway bulb");
        goto bev_alloc_error;
    }
    gw->hostname = strdup(hostname);
    if (!gw->hostname) {
        lifxd_warn("can't allocate a new gateway bulb");
        goto hostname_alloc_error;
    }
    gw->port = port;
    memcpy(gw->addr, site, sizeof(gw->addr));
    bufferevent_setcb(
        gw->io,
        lifxd_gateway_data_read_callback,
        NULL,
        lifxd_gateway_event_callback,
        gw
    );

    int error = bufferevent_socket_connect_hostname(
        gw->io, NULL, AF_UNSPEC, hostname, port
    );
    if (!error) {
        lifxd_info("new gateway at [%s]:%hu", hostname, port);
        return gw;
    }

    free(gw->hostname);
hostname_alloc_error:
    bufferevent_free(gw->io);
bev_alloc_error:
    free(gw);
    return NULL;
}

struct lifxd_gateway *
lifxd_gateway_get(const uint8_t *site)
{
    assert(site);

    struct lifxd_gateway *gw, *next_gw;
    LIST_FOREACH_SAFE(gw, &lifxd_gateways, link, next_gw) {
        if (!memcmp(gw->addr, site, sizeof(gw->addr))) {
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
lifxd_gateway_get_pan_gateway(struct lifxd_gateway *gw)
{
    assert(gw);

    struct lifxd_packet_header hdr = {
        .size = LIFXD_PACKET_HEADER_SIZE,
        .protocol = LIFXD_PROTOCOL_VERSION,
        .packet_type = LIFXD_GET_PAN_GATEWAY
    };
    lifxd_wire_encode_header(&hdr);
    lifxd_debug("GET_PAN_GATEWAY → [%s]:%hu", gw->hostname, gw->port);
    bufferevent_write(gw->io, &hdr, sizeof(hdr));
}

void
lifxd_gateway_handle_pan_gateway(struct lifxd_gateway *gw,
                                 const struct lifxd_packet_header *hdr,
                                 const struct lifxd_packet_pan_gateway *pkt)
{
    assert(gw && hdr && pkt);

    lifxd_debug(
        "SET_PAN_GATEWAY ← [%s]:%hu - %s gw_addr=%s",
        gw->hostname, gw->port,
        lifxd_addrtoa(hdr->bulb_addr),
        lifxd_addrtoa(hdr->gw_addr)
    );
    memcpy(gw->addr, &gw->cur_hdr.gw_addr, sizeof(gw->addr));
}

void
lifxd_gateway_get_light_status(struct lifxd_gateway *gw)
{
    assert(gw);

    struct lifxd_packet_header hdr = {
        .size = LIFXD_PACKET_HEADER_SIZE,
        .protocol = LIFXD_PROTOCOL_VERSION,
        .packet_type = LIFXD_GET_LIGHT_STATE
    };
    lifxd_wire_encode_header(&hdr);
    lifxd_debug("GET_LIGHT_STATE → [%s]:%hu", gw->hostname, gw->port);
    bufferevent_write(gw->io, &hdr, sizeof(hdr));
}

void
lifxd_gateway_handle_light_status(struct lifxd_gateway *gw,
                                  const struct lifxd_packet_header *hdr,
                                  const struct lifxd_packet_light_status *pkt)
{
    assert(gw && hdr && pkt);

    lifxd_debug(
        "SET_LIGHT_STATUS ← [%s]:%hu - %s "
        "hue=%#hx, saturation=%#hx, brightness=%#hx, "
        "kelvin=%#hx, dim=%#hx, power=%#hx, label=%.*s, tags=%lx",
        gw->hostname, gw->port, lifxd_addrtoa(hdr->bulb_addr),
        pkt->hue, pkt->saturation, pkt->brightness, pkt->kelvin,
        pkt->dim, pkt->power, sizeof(pkt->label), pkt->label, pkt->tags
    );

    struct lifxd_bulb *bulb = lifxd_bulb_get_or_open(gw, hdr->bulb_addr);
    if (bulb) {
        assert(sizeof(*pkt) == sizeof(bulb->status));
        memcpy(&bulb->status, pkt, sizeof(*pkt));
    }
}

void
lifxd_gateway_handle_power_state(struct lifxd_gateway *gw,
                                 const struct lifxd_packet_header *hdr,
                                 const struct lifxd_packet_power_state *pkt)
{
    assert(gw && hdr && pkt);

    lifxd_debug(
        "SET_POWER_STATE ← [%s]:%hu - %s power=%#hx",
        gw->hostname, gw->port, lifxd_addrtoa(hdr->bulb_addr), pkt->power
    );

    struct lifxd_bulb *bulb = lifxd_bulb_get_or_open(gw, hdr->bulb_addr);
    if (bulb) {
        bulb->status.power = pkt->power;
    }
}
