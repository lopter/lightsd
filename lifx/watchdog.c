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
#include <assert.h>
#include <endian.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <event2/event.h>
#include <event2/util.h>

#include "wire_proto.h"
#include "core/time_monotonic.h"
#include "broadcast.h"
#include "bulb.h"
#include "gateway.h"
#include "watchdog.h"
#include "core/lightsd.h"

static struct {
    struct event *watchdog_interval_ev;
    struct event *discovery_timeout_ev;
} lgtd_lifx_watchdog_context = {
    .watchdog_interval_ev = NULL,
    .discovery_timeout_ev = NULL
};

static void
lgtd_lifx_watchdog_discovery_timeout_event_callback(evutil_socket_t socket,
                                                    short events,
                                                    void *ctx)
{
    (void)socket;
    (void)events;
    (void)ctx;

    int timeout = LGTD_LIFX_WATCHDOG_PASSIVE_DISCOVERY_INTERVAL_MSECS;
    if (LIST_EMPTY(&lgtd_lifx_gateways)) {
        lgtd_debug(
            "discovery didn't returned anything in %dms, restarting it",
            LGTD_LIFX_WATCHDOG_ACTIVE_DISCOVERY_INTERVAL_MSECS
        );
        timeout = LGTD_LIFX_WATCHDOG_ACTIVE_DISCOVERY_INTERVAL_MSECS;
    } else {
        lgtd_debug("sending periodic discovery packet");
    }

    struct timeval tv = LGTD_MSECS_TO_TIMEVAL(timeout);
    if (event_add(lgtd_lifx_watchdog_context.discovery_timeout_ev, &tv)
        || !lgtd_lifx_broadcast_discovery()) {
        lgtd_err(1, "can't start discovery");
    }
}

static void
lgtd_lifx_watchdog_timeout_event_callback(evutil_socket_t socket,
                                          short events,
                                          void *ctx)
{
    (void)socket;
    (void)events;
    (void)ctx;

    bool start_discovery = false;
    lgtd_time_mono_t now = lgtd_time_monotonic_msecs();

    struct lgtd_lifx_bulb *bulb, *next_bulb;
    RB_FOREACH_SAFE(
        bulb,
        lgtd_lifx_bulb_map,
        &lgtd_lifx_bulbs_table,
        next_bulb
    ) {
        int light_state_lag = now - bulb->last_light_state_at;
        if (light_state_lag >= LGTD_LIFX_WATCHDOG_DEVICE_TIMEOUT_MSECS) {
            lgtd_info(
                "closing bulb \"%.*s\" that hasn't been updated for %dms",
                LGTD_LIFX_LABEL_SIZE, bulb->state.label, light_state_lag
            );
            lgtd_lifx_gateway_remove_and_close_bulb(bulb->gw, bulb);
            start_discovery = true;
            continue;
        }
    }

    // Repeat for the gateways, we could also look if we are removing the last
    // bulb on the gateway but this will also support architectures where
    // gateways aren't bulbs themselves:
    struct lgtd_lifx_gateway *gw, *next_gw;
    LIST_FOREACH_SAFE(gw, &lgtd_lifx_gateways, link, next_gw) {
        int gw_lag = lgtd_lifx_gateway_latency(gw);
        if (gw_lag >= LGTD_LIFX_WATCHDOG_DEVICE_TIMEOUT_MSECS) {
            lgtd_info(
                "closing bulb gateway %s that hasn't received traffic for %dms",
                gw->peeraddr, gw_lag
            );
            lgtd_lifx_gateway_close(gw);
            start_discovery = true;
        } else if (gw_lag >= LGTD_LIFX_WATCHDOG_DEVICE_FORCE_REFRESH_MSECS) {
            lgtd_info(
                "no update on bulb gateway %s for %dms, forcing refresh",
                gw->peeraddr, gw_lag
            );
            lgtd_lifx_gateway_force_refresh(gw);
        }
    }

    // If anything happens restart a discovery right away, maybe something just
    // moved on the network:
    if (start_discovery) {
        lgtd_lifx_broadcast_discovery();
    }
}

bool
lgtd_lifx_watchdog_setup(void)
{
    assert(!lgtd_lifx_watchdog_context.watchdog_interval_ev);
    assert(!lgtd_lifx_watchdog_context.discovery_timeout_ev);

    lgtd_lifx_watchdog_context.discovery_timeout_ev = event_new(
        lgtd_ev_base,
        -1,
        0,
        lgtd_lifx_watchdog_discovery_timeout_event_callback,
        NULL
    );
    lgtd_lifx_watchdog_context.watchdog_interval_ev = event_new(
        lgtd_ev_base,
        -1,
        EV_PERSIST,
        lgtd_lifx_watchdog_timeout_event_callback,
        NULL
    );

    if (lgtd_lifx_watchdog_context.discovery_timeout_ev
        && lgtd_lifx_watchdog_context.watchdog_interval_ev) {
        return true;
    }

    int errsave = errno;
    lgtd_lifx_watchdog_close();
    errno = errsave;
    return false;
}

void
lgtd_lifx_watchdog_close(void)
{
    if (lgtd_lifx_watchdog_context.discovery_timeout_ev) {
        event_del(lgtd_lifx_watchdog_context.discovery_timeout_ev);
        event_free(lgtd_lifx_watchdog_context.discovery_timeout_ev);
        lgtd_lifx_watchdog_context.discovery_timeout_ev = NULL;
    }
    if (lgtd_lifx_watchdog_context.watchdog_interval_ev) {
        event_del(lgtd_lifx_watchdog_context.watchdog_interval_ev);
        event_free(lgtd_lifx_watchdog_context.watchdog_interval_ev);
        lgtd_lifx_watchdog_context.watchdog_interval_ev = NULL;
    }
}

void
lgtd_lifx_watchdog_start(void)
{
    assert(
        !RB_EMPTY(&lgtd_lifx_bulbs_table) || !LIST_EMPTY(&lgtd_lifx_gateways)
    );

    bool pending = evtimer_pending(
        lgtd_lifx_watchdog_context.watchdog_interval_ev, NULL
    );
    if (!pending) {
        struct timeval tv = LGTD_MSECS_TO_TIMEVAL(
            LGTD_LIFX_WATCHDOG_INTERVAL_MSECS
        );
        if (event_add(lgtd_lifx_watchdog_context.watchdog_interval_ev, &tv)) {
            lgtd_err(1, "can't start watchdog");
        }
        lgtd_debug("starting watchdog timer");
    }
}

void
lgtd_lifx_watchdog_start_discovery(void)
{
    assert(!evtimer_pending(
        lgtd_lifx_watchdog_context.discovery_timeout_ev, NULL
    ));

    lgtd_lifx_watchdog_discovery_timeout_event_callback(-1, 0, NULL);
    lgtd_debug("starting discovery timer");
}
