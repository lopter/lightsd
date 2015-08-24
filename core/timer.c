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
#include <assert.h>
#include <err.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <event2/event.h>
#include <event2/util.h>

#include "timer.h"
#include "lightsd.h"

static struct lgtd_timer_list lgtd_timers = LIST_HEAD_INITIALIZER(&lgtd_timers);

static void
lgtd_timer_callback(evutil_socket_t socket, short events, void *ctx)
{
    assert(ctx);

    (void)socket;
    (void)events;

    struct lgtd_timer *timer = ctx;
    timer->callback(timer, timer->ctx);
}

struct lgtd_timer *
lgtd_timer_start(int flags,
                 int ms,
                 void (*cb)(struct lgtd_timer *,
                            union lgtd_timer_ctx),
                 union lgtd_timer_ctx ctx)
{
    assert(ms > 0);
    assert(cb);

    struct lgtd_timer *timer = calloc(1, sizeof(*timer));
    if (!timer) {
        return false;
    }
    timer->callback = cb;
    timer->ctx = ctx;
    LIST_INSERT_HEAD(&lgtd_timers, timer, link);

    struct timeval tv = LGTD_MSECS_TO_TIMEVAL(ms);
    timer->event = event_new(
        lgtd_ev_base,
        -1,
        flags & LGTD_TIMER_PERSISTENT ? EV_PERSIST : 0,
        lgtd_timer_callback,
        timer
    );
    if (!timer->event || evtimer_add(timer->event, &tv)) {
        LIST_REMOVE(timer, link);
        if (timer->event) {
            event_free(timer->event);
        }
        free(timer);
        return NULL;
    }

    if (flags & LGTD_TIMER_ACTIVATE_NOW) {
        lgtd_timer_activate(timer);
    }
    return timer;
}

void
lgtd_timer_stop(struct lgtd_timer *timer)
{
    assert(timer);

    LIST_REMOVE(timer, link);
    event_del(timer->event);
    event_free(timer->event);
    free(timer);
}

void
lgtd_timer_stop_all(void)
{
    struct lgtd_timer *timer, *next_timer;
    LIST_FOREACH_SAFE(timer, &lgtd_timers, link, next_timer) {
        lgtd_timer_stop(timer);
    }
}
