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

#pragma once

struct timeval;

union lgtd_timer_ctx {
    uint64_t    as_uint;
    void        *as_ptr;
};

struct lgtd_timer {
    LIST_ENTRY(lgtd_timer) link;
    void                   (*callback)(struct lgtd_timer *,
                                       union lgtd_timer_ctx);
    union lgtd_timer_ctx   ctx;
    struct event           *event;
};
LIST_HEAD(lgtd_timer_list, lgtd_timer);

enum lgtd_timer_flags {
    LGTD_TIMER_DEFAULT_FLAGS = 0,
    LGTD_TIMER_ACTIVATE_NOW  = 1,
    LGTD_TIMER_PERSISTENT    = 1 << 1,
};

// Activate the timer now, in other words make the callback pending:
static inline void
lgtd_timer_activate(struct lgtd_timer *timer)
{
    assert(timer);

    event_active(timer->event, 0, 0);
}

// Re-schedule a non-persistent timer with the given timeout:
static inline bool
lgtd_timer_reschedule(struct lgtd_timer *timer, const struct timeval *tv)
{
    assert(timer);
    assert(tv);

    return !evtimer_add(timer->event, tv);
}

static inline bool
lgtd_timer_ispending(const struct lgtd_timer *timer)
{
    assert(timer);

    return evtimer_pending(timer->event, NULL);
}

void lgtd_timer_stop(struct lgtd_timer *);
void lgtd_timer_stop_all(void);
// NOTE: if you start a persistent timer and don't keep track of it, make sure
//       you don't end up in a callback using a context that has been freed.
struct lgtd_timer *lgtd_timer_start(int,
                                    int, // ms
                                    void (*)(struct lgtd_timer *,
                                             union lgtd_timer_ctx),
                                    union lgtd_timer_ctx);
