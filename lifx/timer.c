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
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <event2/event.h>
#include <event2/util.h>

#include "wire_proto.h"
#include "core/time_monotonic.h"
#include "broadcast.h"
#include "bulb.h"
#include "gateway.h"
#include "timer.h"
#include "core/lightsd.h"

static struct {
    struct event *watchdog_interval_ev;
    struct event *discovery_timeout_ev;
} lgtd_lifx_timer_context = {
    .watchdog_interval_ev = NULL,
    .discovery_timeout_ev = NULL
};

static void
lgtd_lifx_timer_discovery_timeout_event_callback(evutil_socket_t socket,
                                                 short events,
                                                 void *ctx)
{
    (void)socket;
    (void)events;
    (void)ctx;

    int timeout = LGTD_LIFX_TIMER_PASSIVE_DISCOVERY_INTERVAL_MSECS;
    if (LIST_EMPTY(&lgtd_lifx_gateways)) {
        lgtd_debug(
            "discovery didn't returned anything in %dms, restarting it",
            LGTD_LIFX_TIMER_ACTIVE_DISCOVERY_INTERVAL_MSECS
        );
        timeout = LGTD_LIFX_TIMER_ACTIVE_DISCOVERY_INTERVAL_MSECS;
    } else {
        lgtd_debug("sending periodic discovery packet");
    }

    struct timeval tv = LGTD_MSECS_TO_TIMEVAL(timeout);
    if (event_add(lgtd_lifx_timer_context.discovery_timeout_ev, &tv)
        || !lgtd_lifx_broadcast_discovery()) {
        lgtd_err(1, "can't start discovery");
    }
}

static void
lgtd_lifx_timer_watchdog_timeout_event_callback(evutil_socket_t socket,
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
        if (light_state_lag >= LGTD_LIFX_TIMER_DEVICE_TIMEOUT_MSECS) {
            lgtd_info(
                "closing bulb \"%.*s\" that hasn't been updated for %dms",
                LGTD_LIFX_LABEL_SIZE, bulb->state.label, light_state_lag
            );
            lgtd_lifx_bulb_close(bulb);
            start_discovery = true;
        }
    }

    // Repeat for the gateways, we could also look if we are removing the last
    // bulb on the gateway but this will also support architectures where
    // gateways aren't bulbs themselves:
    struct lgtd_lifx_gateway *gw, *next_gw;
    LIST_FOREACH_SAFE(gw, &lgtd_lifx_gateways, link, next_gw) {
        int gw_lag = now - gw->last_pkt_at;
        if (gw_lag >= LGTD_LIFX_TIMER_DEVICE_TIMEOUT_MSECS) {
            lgtd_info(
                "closing bulb gateway [%s]:%hu that "
                "hasn't received traffic for %dms",
                gw->ip_addr, gw->port,
                gw_lag
            );
            lgtd_lifx_gateway_close(gw);
            start_discovery = true;
        } else if (gw_lag >= LGTD_LIFX_TIMER_DEVICE_FORCE_REFRESH_MSECS) {
            lgtd_info(
                "no update on bulb gateway [%s]:%hu for %dms, forcing refresh",
                gw->ip_addr, gw->port, gw_lag
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
lgtd_lifx_timer_setup(void)
{
    assert(!lgtd_lifx_timer_context.watchdog_interval_ev);
    assert(!lgtd_lifx_timer_context.discovery_timeout_ev);

    lgtd_lifx_timer_context.discovery_timeout_ev = event_new(
        lgtd_ev_base,
        -1,
        0,
        lgtd_lifx_timer_discovery_timeout_event_callback,
        NULL
    );
    lgtd_lifx_timer_context.watchdog_interval_ev = event_new(
        lgtd_ev_base,
        -1,
        EV_PERSIST,
        lgtd_lifx_timer_watchdog_timeout_event_callback,
        NULL
    );

    if (lgtd_lifx_timer_context.discovery_timeout_ev
        && lgtd_lifx_timer_context.watchdog_interval_ev) {
        return true;
    }

    int errsave = errno;
    lgtd_lifx_timer_close();
    errno = errsave;
    return false;
}

void
lgtd_lifx_timer_close(void)
{
    if (lgtd_lifx_timer_context.discovery_timeout_ev) {
        event_del(lgtd_lifx_timer_context.discovery_timeout_ev);
        event_free(lgtd_lifx_timer_context.discovery_timeout_ev);
        lgtd_lifx_timer_context.discovery_timeout_ev = NULL;
    }
    if (lgtd_lifx_timer_context.watchdog_interval_ev) {
        event_del(lgtd_lifx_timer_context.watchdog_interval_ev);
        event_free(lgtd_lifx_timer_context.watchdog_interval_ev);
        lgtd_lifx_timer_context.watchdog_interval_ev = NULL;
    }
}

void
lgtd_lifx_timer_start_watchdog(void)
{
    assert(
        !RB_EMPTY(&lgtd_lifx_bulbs_table) || !LIST_EMPTY(&lgtd_lifx_gateways)
    );

    if (!evtimer_pending(lgtd_lifx_timer_context.watchdog_interval_ev, NULL)) {
        struct timeval tv = LGTD_MSECS_TO_TIMEVAL(
            LGTD_LIFX_TIMER_WATCHDOG_INTERVAL_MSECS
        );
        if (event_add(lgtd_lifx_timer_context.watchdog_interval_ev, &tv)) {
            lgtd_err(1, "can't start watchdog");
        }
        lgtd_debug("starting watchdog timer");
    }
}

void
lgtd_lifx_timer_start_discovery(void)
{
    assert(!evtimer_pending(
        lgtd_lifx_timer_context.discovery_timeout_ev, NULL
    ));

    lgtd_lifx_timer_discovery_timeout_event_callback(-1, 0, NULL);
    lgtd_debug("starting discovery timer");
}
