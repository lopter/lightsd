#pragma once

// we need those two because mock_timer.h is being used where it shouldn't be
// because it is a dependency for modules (e.g: bulb) that haven't be mocked
// yet:
#include <assert.h>
#include <event2/event.h>

#include "core/timer.h" // to pull the union definition

#ifndef MOCKED_LGTD_TIMER_START
struct lgtd_timer *
lgtd_timer_start(int flags,
                 int ms,
                 void (*cb)(struct lgtd_timer *,
                            union lgtd_timer_ctx),
                 union lgtd_timer_ctx ctx)
{
    (void)flags;
    (void)ms;
    (void)cb;
    (void)ctx;
    return NULL;
}
#endif

#ifndef MOCKED_LGTD_TIMER_STOP
void
lgtd_timer_stop(struct lgtd_timer *timer)
{
    (void)timer;
}
#endif
