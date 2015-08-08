#include "bulb.c"

#include "mock_gateway.h"

int
main(void)
{
    struct lgtd_lifx_bulb bulb = {
        .state = {
            .hue = 54321,
            .brightness = UINT16_MAX,
            .kelvin = 12345,
            .dim = 808,
            .power = LGTD_LIFX_POWER_OFF,
            .label = "lair",
            .tags = 0x2a
        },
        .gw = (void *)0xdeaf
    };
    struct lgtd_lifx_light_state new_state;
    memcpy(&new_state, &bulb.state, sizeof(new_state));
    new_state.power = LGTD_LIFX_POWER_ON;


    for (int i = 0; i != 2; i++) {
        lgtd_lifx_bulb_set_power_state(&bulb, LGTD_LIFX_POWER_ON);
        if (memcmp(&bulb.state, &new_state, sizeof(new_state))) {
            errx(1, "new light state incorrectly set");
        }
        if (LGTD_STATS_GET(bulbs_powered_on) != 1) {
            errx(
                1, "unexpected bulbs_powered_on counter value %d (expected 1)",
                LGTD_STATS_GET(bulbs_powered_on)
            );
        }
    }

    return 0;
}
