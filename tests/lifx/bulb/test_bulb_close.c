#include "bulb.c"

#include "mock_gateway.h"
#include "mock_log.h"
#include "mock_router.h"
#include "mock_timer.h"

int
main(void)
{
    struct lgtd_lifx_gateway gw;
    uint8_t bulb_addr[LGTD_LIFX_ADDR_LENGTH] = { 5, 4, 3, 2, 1, 0 };
    struct lgtd_lifx_bulb *bulb = lgtd_lifx_bulb_open(&gw, bulb_addr);

    bulb->state.power = LGTD_LIFX_POWER_ON;
    LGTD_STATS_ADD_AND_UPDATE_PROCTITLE(bulbs_powered_on, 1);

    lgtd_lifx_bulb_close(bulb);

    if (!RB_EMPTY(&lgtd_lifx_bulbs_table)) {
        errx(1, "The bulbs table should be empty!");
    }

    if (LGTD_STATS_GET(bulbs) != 0) {
        errx(1, "The bulbs counter is %d (expected 0)", LGTD_STATS_GET(bulbs));
    }

    if (LGTD_STATS_GET(bulbs_powered_on) != 0) {
        errx(
            1, "The powered on bulbs counter is %d (expected 0)",
            LGTD_STATS_GET(bulbs_powered_on)
        );
    }

    return 0;
}
