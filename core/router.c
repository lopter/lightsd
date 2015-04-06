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
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <endian.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <event2/util.h>

#include "lifx/wire_proto.h"
#include "time_monotonic.h"
#include "lifx/bulb.h"
#include "proto.h"
#include "router.h"
#include "lifx/gateway.h"
#include "lightsd.h"

void
lgtd_router_broadcast(enum lgtd_lifx_packet_type pkt_type, void *pkt)
{
    struct lgtd_lifx_packet_header hdr;
    union lgtd_lifx_target target = { .tags = 0 };

    const struct lgtd_lifx_packet_infos *pkt_infos = NULL;
    struct lgtd_lifx_gateway *gw;
    LIST_FOREACH(gw, &lgtd_lifx_gateways, link) {
        pkt_infos = lgtd_lifx_wire_setup_header(
            &hdr, LGTD_LIFX_TARGET_ALL_DEVICES, target, gw->site, pkt_type
        );
        assert(pkt_infos);
        lgtd_lifx_gateway_send_packet(gw, &hdr, pkt, pkt_infos->size);
        struct lgtd_lifx_bulb *bulb;
        lgtd_time_mono_t now = lgtd_time_monotonic_msecs();
        SLIST_FOREACH(bulb, &gw->bulbs, link_by_gw) {
            if (pkt_type == LGTD_LIFX_SET_POWER_STATE) {
                bulb->dirty_at = now;
                struct lgtd_lifx_packet_power_state *payload = pkt;
                bulb->expected_power_on = payload->power;
            }
        }
    }

    if (pkt_infos) {
        lgtd_info("broadcasting %s", pkt_infos->name);
    }
}

void
lgtd_router_send_to_device(struct lgtd_lifx_bulb *bulb,
                           enum lgtd_lifx_packet_type pkt_type,
                           void *pkt)
{
    assert(bulb);

    struct lgtd_lifx_packet_header hdr;
    union lgtd_lifx_target target = { .addr = bulb->addr };

    const struct lgtd_lifx_packet_infos *pkt_infos;
    pkt_infos = lgtd_lifx_wire_setup_header(
        &hdr, LGTD_LIFX_TARGET_DEVICE, target, bulb->gw->site, pkt_type
    );
    assert(pkt_infos);

    lgtd_lifx_gateway_send_packet(bulb->gw, &hdr, pkt, pkt_infos->size);

    if (pkt_type == LGTD_LIFX_SET_POWER_STATE) {
        bulb->dirty_at = lgtd_time_monotonic_msecs();
        struct lgtd_lifx_packet_power_state *payload = pkt;
        bulb->expected_power_on = payload->power;
    }

    lgtd_info("sending %s to %s", pkt_infos->name, lgtd_addrtoa(bulb->addr));
}

bool
lgtd_router_send(const struct lgtd_proto_target_list *targets,
                 enum lgtd_lifx_packet_type pkt_type,
                 void *pkt)
{
    assert(targets);

    bool rv = true;

    const struct lgtd_proto_target *target;
    SLIST_FOREACH(target, targets, link) {
        if (!strcmp(target->target, "*")) {
            lgtd_router_broadcast(pkt_type, pkt);
            continue;
        } else if (isxdigit(target->target[0])) {
            const char *endptr = NULL;
            errno = 0;
            long long device = strtoll(target->target, (char **)&endptr, 16);
            if (*endptr || errno == ERANGE) {
                lgtd_debug("invalid target device %s", target->target);
                rv = false;
            }
            device = htobe64(device);
            struct lgtd_lifx_bulb *bulb = lgtd_lifx_bulb_get(
                (uint8_t *)&device + sizeof(device) - LGTD_LIFX_ADDR_LENGTH
            );
            if (bulb) {
                lgtd_router_send_to_device(bulb, pkt_type, pkt);
                continue;
            }
            lgtd_debug("target device %#llx not found", device);
        }
        rv = false;
    }

    return rv;
}
