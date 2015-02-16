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
