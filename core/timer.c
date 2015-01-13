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
#include "time_monotonic.h"
#include "broadcast.h"
#include "bulb.h"
#include "gateway.h"
#include "timer.h"
#include "lifxd.h"

static struct {
    struct event *watchdog_interval_ev;
    struct event *discovery_timeout_ev;
} lifxd_timer_context = {
    .watchdog_interval_ev = NULL,
    .discovery_timeout_ev = NULL
};

static void
lifxd_timer_discovery_timeout_event_callback(evutil_socket_t socket,
                                             short events,
                                             void *ctx)
{
    (void)socket;
    (void)events;
    (void)ctx;

    int timeout = LIFXD_TIMER_PASSIVE_DISCOVERY_INTERVAL_MSECS;
    if (LIST_EMPTY(&lifxd_gateways)) {
        lifxd_debug(
            "discovery didn't returned anything in %dms, restarting it",
            LIFXD_TIMER_ACTIVE_DISCOVERY_INTERVAL_MSECS
        );
        timeout = LIFXD_TIMER_ACTIVE_DISCOVERY_INTERVAL_MSECS;
    } else {
        lifxd_debug("sending periodic discovery packet");
    }

    struct timeval tv = LIFXD_MSECS_TO_TIMEVAL(timeout);
    if (event_add(lifxd_timer_context.discovery_timeout_ev, &tv)
        || !lifxd_broadcast_discovery()) {
        lifxd_err(1, "can't start discovery");
    }
}

static void
lifxd_timer_watchdog_timeout_event_callback(evutil_socket_t socket,
                                            short events,
                                            void *ctx)
{
    (void)socket;
    (void)events;
    (void)ctx;

    bool start_discovery = false;
    lifxd_time_mono_t now = lifxd_time_monotonic_msecs();

    struct lifxd_bulb *bulb, *next_bulb;
    RB_FOREACH_SAFE(bulb, lifxd_bulb_map, &lifxd_bulbs_table, next_bulb) {
        int light_state_lag = now - bulb->last_light_state_at;
        if (light_state_lag >= LIFXD_TIMER_DEVICE_TIMEOUT_MSECS) {
            lifxd_info(
                "closing bulb \"%.s\" that hasn't been updated for %dms",
                sizeof(bulb->state.label),
                bulb->state.label,
                light_state_lag
            );
            lifxd_bulb_close(bulb);
            start_discovery = true;
        }
    }

    // Repeat for the gateways, we could also look if we are removing the last
    // bulb on the gateway but this will also support architectures where
    // gateways aren't bulbs themselves:
    struct lifxd_gateway *gw, *next_gw;
    LIST_FOREACH_SAFE(gw, &lifxd_gateways, link, next_gw) {
        int gw_lag = now - gw->last_pkt_at;
        if (gw_lag >= LIFXD_TIMER_DEVICE_TIMEOUT_MSECS) {
            lifxd_info(
                "closing bulb gateway [%s]:%hu that "
                "hasn't received traffic for %dms",
                gw->ip_addr, gw->port,
                gw_lag
            );
            lifxd_gateway_close(gw);
            start_discovery = true;
        }
    }

    // If anything happens restart a discovery right away, maybe something just
    // moved on the network:
    if (start_discovery) {
        lifxd_broadcast_discovery();
    }
}

bool
lifxd_timer_setup(void)
{
    assert(!lifxd_timer_context.watchdog_interval_ev);
    assert(!lifxd_timer_context.discovery_timeout_ev);

    lifxd_timer_context.discovery_timeout_ev = event_new(
        lifxd_ev_base,
        -1,
        0,
        lifxd_timer_discovery_timeout_event_callback,
        NULL
    );
    lifxd_timer_context.watchdog_interval_ev = event_new(
        lifxd_ev_base,
        -1,
        EV_PERSIST,
        lifxd_timer_watchdog_timeout_event_callback,
        NULL
    );

    if (lifxd_timer_context.discovery_timeout_ev
        && lifxd_timer_context.watchdog_interval_ev) {
        return true;
    }

    int errsave = errno;
    lifxd_timer_close();
    errno = errsave;
    return false;
}

void
lifxd_timer_close(void)
{
    if (lifxd_timer_context.discovery_timeout_ev) {
        event_del(lifxd_timer_context.discovery_timeout_ev);
        event_free(lifxd_timer_context.discovery_timeout_ev);
        lifxd_timer_context.discovery_timeout_ev = NULL;
    }
    if (lifxd_timer_context.watchdog_interval_ev) {
        event_del(lifxd_timer_context.watchdog_interval_ev);
        event_free(lifxd_timer_context.watchdog_interval_ev);
        lifxd_timer_context.watchdog_interval_ev = NULL;
    }
}

void
lifxd_timer_start_watchdog(void)
{
    assert(!RB_EMPTY(&lifxd_bulbs_table) || !LIST_EMPTY(&lifxd_gateways));

    if (!evtimer_pending(lifxd_timer_context.watchdog_interval_ev, NULL)) {
        struct timeval tv = LIFXD_MSECS_TO_TIMEVAL(
            LIFXD_TIMER_WATCHDOG_INTERVAL_MSECS
        );
        if (event_add(lifxd_timer_context.watchdog_interval_ev, &tv)) {
            lifxd_err(1, "can't start watchdog");
        }
        lifxd_debug("starting watchdog timer");
    }
}

void
lifxd_timer_start_discovery(void)
{
    assert(!evtimer_pending(lifxd_timer_context.discovery_timeout_ev, NULL));

    struct timeval tv = LIFXD_MSECS_TO_TIMEVAL(
        LIFXD_TIMER_ACTIVE_DISCOVERY_INTERVAL_MSECS
    );
    if (event_add(lifxd_timer_context.discovery_timeout_ev, &tv)) {
        lifxd_err(1, "can't start discovery timer");
    }
    lifxd_debug("starting discovery timer");
}
