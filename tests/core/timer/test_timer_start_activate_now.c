#include <string.h>

#include "core/timer.c"

#define MOCKED_EVENT_NEW
#define MOCKED_EVENT_ADD
#define MOCKED_EVENT_ACTIVE
#include "mock_event2.h"
#include "tests_shims.h"

static void
my_test_callback(struct lgtd_timer *timer, union lgtd_timer_ctx ctx)
{
    (void)timer;
    (void)ctx;
}

static int event_active_call_count = 0;

void
event_active(struct event *ev, int res, short ncalls)
{
    if (ev != MOCK_EVENT_NEW_EVENT_PTR) {
        errx(1, "got event %p (expected %p)", ev, MOCK_EVENT_NEW_EVENT_PTR);
    }

    if (res) {
        errx(1, "got res = %d (expected 0)", res);
    }

    if (ncalls) {
        errx(1, "got ncalls = %d (expected 0)", ncalls);
    }

    if (event_active_call_count++) {
        errx(1, "event_active should be called once");
    }
}

static int event_new_call_count = 0;

struct event *
event_new(struct event_base *base,
          evutil_socket_t fd,
          short events,
          event_callback_fn cb,
          void *ctx)
{
    if (base != MOCK_LGTD_EV_BASE) {
        errx(1, "got base %p (expected %p)", base, MOCK_LGTD_EV_BASE);
    }

    if (fd != -1) {
        errx(1, "got fd %d (expected -1)", fd);
    }

    if (events) {
        errx(1, "got events %#x (expected 0)", events);
    }

    if (cb != lgtd_timer_callback) {
        errx(1, "got cb %p (expected %p)", cb, lgtd_timer_callback);
    }

    if (!ctx) {
        errx(1, "didn't get any context");
    }

    if (event_new_call_count++) {
        errx(1, "event_new should be called once");
    }

    return MOCK_EVENT_NEW_EVENT_PTR;
}

static int event_add_call_count = 0;

int
event_add(struct event *ev, const struct timeval *timeout)
{
    if (ev != MOCK_EVENT_NEW_EVENT_PTR) {
        errx(1, "got ev %p (expected %p)", ev, MOCK_EVENT_NEW_EVENT_PTR);
    }

    if (!timeout) {
        errx(1, "a timeout should have been passed in");
    }
    // so i don't know wth clang and mac os x are doing but memcmp
    // gets whacked in -O2 and returns a difference when there isn't,
    // so we have to do it the painful way:
    struct timeval expected_tv = LGTD_MSECS_TO_TIMEVAL(5);
    if (timeout->tv_sec != expected_tv.tv_sec
        || timeout->tv_usec != expected_tv.tv_usec) {
        errx(1, "got invalid timeout");
    }

    if (event_add_call_count++) {
        errx(1, "event_add should be called once");
    }

    return 0;
}

int
main(void)
{
    union lgtd_timer_ctx ctx = { .as_uint = 7614 };
    struct lgtd_timer *timer = lgtd_timer_start(
        LGTD_TIMER_ACTIVATE_NOW, 5, my_test_callback, ctx
    );

    if (timer->event != MOCK_EVENT_NEW_EVENT_PTR) {
        errx(
            1, "timer has event %p (expected %p)",
            timer->event, MOCK_EVENT_NEW_EVENT_PTR
        );
    }
    if (timer->ctx.as_uint != ctx.as_uint) {
        errx(
            1, "timer ctx is %ju (expected %ju)",
            (uintmax_t)timer->ctx.as_uint, (uintmax_t)ctx.as_uint
        );
    }
    if (timer->callback != my_test_callback) {
        errx(
            1, "timer callback is %p (expected %p)",
            timer->callback, my_test_callback
        );
    }
    if (LIST_FIRST(&lgtd_timers) != timer) {
        errx(1, "the timer wasn't inserted in the timers list");
    }

    if (!event_new_call_count) {
        errx(1, "event_new wasn't called");
    }
    if (!event_add_call_count) {
        errx(1, "event_add wasn't called");
    }
    if (!event_active_call_count) {
        errx(1, "the timer wasn't activated");
    }

    return 0;
}
