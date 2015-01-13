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

#include <event2/util.h>

#include "wire_proto.h"
#include "time_monotonic.h"
#include "bulb.h"
#include "gateway.h"
#include "lifxd.h"

struct lifxd_bulb_map lifxd_bulbs_table = RB_INITIALIZER(&lifxd_bulbs_table);

struct lifxd_bulb *
lifxd_bulb_get(struct lifxd_gateway *gw, const uint8_t *addr)
{
    assert(gw);
    assert(addr);

    struct lifxd_bulb bulb;
    memcpy(bulb.addr, addr, sizeof(bulb.addr));
    return RB_FIND(lifxd_bulb_map, &lifxd_bulbs_table, &bulb);
}

struct lifxd_bulb *
lifxd_bulb_open(struct lifxd_gateway *gw, const uint8_t *addr)
{
    assert(gw);
    assert(addr);

    struct lifxd_bulb *bulb = calloc(1, sizeof(*bulb));
    if (!bulb) {
        lifxd_warn("can't allocate a new bulb");
        return NULL;
    }

    bulb->gw = gw;
    memcpy(bulb->addr, addr, sizeof(bulb->addr));
    RB_INSERT(lifxd_bulb_map, &lifxd_bulbs_table, bulb);

    bulb->last_light_state_at = lifxd_time_monotonic_msecs();

    return bulb;
}

void
lifxd_bulb_close(struct lifxd_bulb *bulb)
{
    assert(bulb);
    assert(bulb->gw);

    RB_REMOVE(lifxd_bulb_map, &lifxd_bulbs_table, bulb);
    SLIST_REMOVE(&bulb->gw->bulbs, bulb, lifxd_bulb, link_by_gw);
    lifxd_info(
        "closed bulb \"%.*s\" on [%s]:%hu",
        sizeof(bulb->state.label),
        bulb->state.label,
        bulb->gw->ip_addr,
        bulb->gw->port
    );
    free(bulb);
}

void
lifxd_bulb_set_light_state(struct lifxd_bulb *bulb,
                           const struct lifxd_light_state *state,
                           lifxd_time_mono_t received_at)
{
    assert(bulb);
    assert(state);
    bulb->last_light_state_at = received_at;
    memcpy(&bulb->state, state, sizeof(bulb->state));
}

void
lifxd_bulb_set_power_state(struct lifxd_bulb *bulb, uint16_t power)
{
    assert(bulb);
    bulb->state.power = power;
}
