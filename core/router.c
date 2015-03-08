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
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <event2/util.h>

#include "lifx/wire_proto.h"
#include "time_monotonic.h"
#include "lifx/bulb.h"
#include "lifx/gateway.h"
#include "lightsd.h"

static void
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
    }

    if (pkt_infos) {
        lgtd_debug("broadcasting %s", pkt_infos->name);
    }
}

bool
lgtd_router_send(const char *target,
                 enum lgtd_lifx_packet_type pkt_type,
                 void *pkt)
{
    assert(target);

    if (!strcmp(target, "*")) {
        lgtd_router_broadcast(pkt_type, pkt);
        return true;
    }

    return false;
}
