#include "bulb.c"

#include "mock_gateway.h"
#include "mock_router.h"
#include "mock_timer.h"

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

    return 0;
}
