#include <string.h>

#include "core/timer.c"

#define MOCKED_EVENT_PENDING
#include "mock_event2.h"
#include "tests_shims.h"

static int event_pending_call_count = 0;

int
event_pending(const struct event *ev, short events, struct timeval *tv)
{
    (void)events;

    if (ev != MOCK_EVENT_NEW_EVENT_PTR) {
        errx(1, "got event %p (expected %p)", ev, MOCK_EVENT_NEW_EVENT_PTR);
    }

    if (tv) {
        errx(1, "got unexpected parameter tv");
    }

    if (event_pending_call_count++) {
        errx(1, "event_pending should have been called once");
    }

    return true;
}

int
main(void)
{
    struct lgtd_timer timer = { .event = MOCK_EVENT_NEW_EVENT_PTR };

    if (!lgtd_timer_ispending(&timer)) {
        errx(1, "lgtd_timer_ispending returned false (expected true)");
    }

    if (!event_pending_call_count) {
        errx(1, "event_pending wasn't called");
    }

    return 0;
}
