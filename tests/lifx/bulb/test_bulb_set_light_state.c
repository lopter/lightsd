#include "bulb.c"

#define MOCKED_LGTD_LIFX_GATEWAY_UPDATE_TAG_REFCOUNTS
#include "mock_gateway.h"
#include "mock_router.h"
#include "mock_timer.h"

static int update_tag_refcouts_call_counts = 0;

void
lgtd_lifx_gateway_update_tag_refcounts(struct lgtd_lifx_gateway *gw,
                                       uint64_t bulb_tags,
                                       uint64_t pkt_tags)
{
    if (gw != (void *)0xdeaf) {
        errx(1, "got wrong gw %p (expected 0xdeaf)", gw);
    }

    if (pkt_tags != 0xfeed) {
        errx(1, "got pkt_tags %#jx (expected 0xfeed)", (uintmax_t)pkt_tags);
    }

    if (!update_tag_refcouts_call_counts) {
        if (bulb_tags != 0x2a) {
            errx(1, "got bulb_tags %#jx (expected 0x2a)", (uintmax_t)bulb_tags);
        }
    } else {
        if (bulb_tags != 0xfeed) {
            errx(1, "got bulb_tags %#jx (expected 0xfeed)", (uintmax_t)bulb_tags);
        }
    }

    update_tag_refcouts_call_counts++;
}

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

    struct lgtd_lifx_light_state new_state = {
        .hue = 22222,
        .brightness = UINT16_MAX / 2,
        .kelvin = 54321,
        .dim = 303,
        .power = LGTD_LIFX_POWER_ON,
        .label = "caverne",
        .tags = 0xfeed
    };

    lgtd_lifx_bulb_set_light_state(&bulb, &new_state, 2015);
    if (memcmp(&bulb.state, &new_state, sizeof(new_state))) {
        errx(1, "new light state incorrectly set");
    }
    if (LGTD_STATS_GET(bulbs_powered_on) != 1) {
        errx(
            1, "unexpected bulbs_powered_on counter value %d (expected 1)",
            LGTD_STATS_GET(bulbs_powered_on)
        );
    }
    if (bulb.last_light_state_at != 2015) {
        errx(
            1, "got bulb.last_light_state = %jx (expected 2015)",
            (uintmax_t)bulb.last_light_state_at
        );
    }
    if (update_tag_refcouts_call_counts != 1) {
        errx(1, "lgtd_lifx_gateway_update_tag_refcounts wasn't called");
    }

    lgtd_lifx_bulb_set_light_state(&bulb, &new_state, 2015);
    if (update_tag_refcouts_call_counts != 2) {
        errx(1, "lgtd_lifx_gateway_update_tag_refcounts wasn't called");
    }
    if (LGTD_STATS_GET(bulbs_powered_on) != 1) {
        errx(
            1, "unexpected bulbs_powered_on counter value %d (expected 1)",
            LGTD_STATS_GET(bulbs_powered_on)
        );
    }

    return 0;
}
