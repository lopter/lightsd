#include "bulb.c"

#include "mock_gateway.h"
#include "mock_router.h"
#define MOCKED_LGTD_TIMER_START
#include "mock_timer.h"

static int timer_start_call_count = 0;

struct lgtd_timer *
lgtd_timer_start(int flags,
                 int ms,
                 void (*cb)(struct lgtd_timer *,
                            union lgtd_timer_ctx),
                 union lgtd_timer_ctx ctx)
{
    if (flags != (LGTD_TIMER_PERSISTENT|LGTD_TIMER_ACTIVATE_NOW)) {
        errx(
            1, "got flags %#x (expected %#x)",
            flags, LGTD_TIMER_PERSISTENT|LGTD_TIMER_ACTIVATE_NOW
        );
    }

    if (ms != LGTD_LIFX_BULB_FETCH_HARDWARE_INFO_TIMER_MSECS) {
        errx(
            1, "got timeout %d (expected %d)",
            ms, LGTD_LIFX_BULB_FETCH_HARDWARE_INFO_TIMER_MSECS
        );
    }

    if (cb != lgtd_lifx_bulb_fetch_hardware_info) {
        errx(
            1, "got callback %p (expected %p)",
            cb, lgtd_lifx_bulb_fetch_hardware_info
        );
    }

    if (!ctx.as_uint) {
        errx(1, "ctx wasn't set");
    }

    if (timer_start_call_count++) {
        errx(1, "timer_start should have been called once");
    }

    return (void *)42;
}

int
main(void)
{
    struct lgtd_lifx_gateway gw;
    uint8_t bulb_addr[LGTD_LIFX_ADDR_LENGTH] = { 5, 4, 3, 2, 1, 0 };
    lgtd_time_mono_t now = lgtd_time_monotonic_msecs();
    struct lgtd_lifx_bulb *bulb = lgtd_lifx_bulb_open(&gw, bulb_addr);

    if (!bulb) {
        errx(1, "lgtd_lifx_bulb_open didn't return any bulb");
    }

    char addr[LGTD_LIFX_ADDR_STRLEN], expected[LGTD_LIFX_ADDR_STRLEN];
    if (memcmp(bulb->addr, bulb_addr, LGTD_LIFX_ADDR_LENGTH)) {
        errx(
            1, "got bulb addr %s (expected %s)",
            LGTD_IEEE8023MACTOA(bulb->addr, addr),
            LGTD_IEEE8023MACTOA(bulb_addr, expected)
        );
    }

    if (bulb->gw != &gw) {
        errx(1, "got bulb gateway %p (expected %p)", bulb->gw, &gw);
    }

    if (lgtd_lifx_bulb_get(bulb_addr) != bulb) {
        errx(1, "the new bulb can't be found");
    }

    if (bulb->last_light_state_at < now) {
        errx(
            1, "got bulb->last_light_state_at %ju (expected >= %ju)",
            (uintmax_t)bulb->last_light_state_at, (uintmax_t)now
        );
    }

    if (LGTD_STATS_GET(bulbs) != 1) {
        errx(1, "bulbs counter is %d (expected 1)", LGTD_STATS_GET(bulbs));
    }

    if (!timer_start_call_count) {
        errx(1, "timer_start_call_count = 0 (expected 1)");
    }

    return 0;
}
