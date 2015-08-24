#include <string.h>

#include "core/timer.c"

#define MOCKED_EVENT_ADD
#include "mock_event2.h"
#include "tests_shims.h"

static int event_add_call_count = 0;

int
event_add(struct event *ev, const struct timeval *tv)
{
    if (ev != MOCK_EVENT_NEW_EVENT_PTR) {
        errx(1, "got event %p (expected %p)", ev, MOCK_EVENT_NEW_EVENT_PTR);
    }

    struct timeval expected_tv = LGTD_MSECS_TO_TIMEVAL(5);
    if (memcmp(tv, &expected_tv, sizeof(*tv))) {
        errx(1, "got unexpected timeout");
    }

    if (event_add_call_count++) {
        errx(1, "event_add should have been called once");
    }

    return 0;
}

int
main(void)
{
    struct lgtd_timer timer = { .event = MOCK_EVENT_NEW_EVENT_PTR };
    struct timeval tv = LGTD_MSECS_TO_TIMEVAL(5);

    if (!lgtd_timer_reschedule(&timer, &tv)) {
        errx(1, "wrong return value");
    }

    if (!event_add_call_count) {
        errx(1, "event_add wasn't called");
    }

    return 0;
}
