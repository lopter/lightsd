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
#include <err.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <event2/util.h>

#include "wire_proto.h"
#include "core/time_monotonic.h"
#include "bulb.h"
#include "gateway.h"
#include "core/lightsd.h"

struct lgtd_lifx_bulb_map lgtd_lifx_bulbs_table =
    RB_INITIALIZER(&lgtd_lifx_bulbs_table);

struct lgtd_lifx_bulb *
lgtd_lifx_bulb_get(const uint8_t *addr)
{
    assert(addr);

    struct lgtd_lifx_bulb bulb;
    memcpy(bulb.addr, addr, sizeof(bulb.addr));
    return RB_FIND(lgtd_lifx_bulb_map, &lgtd_lifx_bulbs_table, &bulb);
}

struct lgtd_lifx_bulb *
lgtd_lifx_bulb_open(struct lgtd_lifx_gateway *gw, const uint8_t *addr)
{
    assert(gw);
    assert(addr);

    struct lgtd_lifx_bulb *bulb = calloc(1, sizeof(*bulb));
    if (!bulb) {
        lgtd_warn("can't allocate a new bulb");
        return NULL;
    }

    bulb->gw = gw;
    memcpy(bulb->addr, addr, sizeof(bulb->addr));
    RB_INSERT(lgtd_lifx_bulb_map, &lgtd_lifx_bulbs_table, bulb);

    bulb->last_light_state_at = lgtd_time_monotonic_msecs();

    return bulb;
}

void
lgtd_lifx_bulb_close(struct lgtd_lifx_bulb *bulb)
{
    assert(bulb);
    assert(bulb->gw);

    RB_REMOVE(lgtd_lifx_bulb_map, &lgtd_lifx_bulbs_table, bulb);
    SLIST_REMOVE(&bulb->gw->bulbs, bulb, lgtd_lifx_bulb, link_by_gw);
    lgtd_info(
        "closed bulb \"%.*s\" (%s) on [%s]:%hu",
        LGTD_LIFX_LABEL_SIZE,
        bulb->state.label,
        lgtd_addrtoa(bulb->addr),
        bulb->gw->ip_addr,
        bulb->gw->port
    );
    free(bulb);
}

void
lgtd_lifx_bulb_set_light_state(struct lgtd_lifx_bulb *bulb,
                               const struct lgtd_lifx_light_state *state,
                               lgtd_time_mono_t received_at)
{
    assert(bulb);
    assert(state);
    bulb->last_light_state_at = received_at;
    memcpy(&bulb->state, state, sizeof(bulb->state));
}

void
lgtd_lifx_bulb_set_power_state(struct lgtd_lifx_bulb *bulb, uint16_t power)
{
    assert(bulb);
    bulb->state.power = power;
}
