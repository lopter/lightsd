#include "bulb.c"

#define MOCKED_LGTD_LIFX_GATEWAY_UPDATE_TAG_REFCOUNTS
#include "mock_gateway.h"
#include "mock_log.h"
#include "mock_router.h"
#include "mock_timer.h"

static bool update_tag_refcouts_called = false;

void
lgtd_lifx_gateway_update_tag_refcounts(struct lgtd_lifx_gateway *gw,
                                       uint64_t bulb_tags,
                                       uint64_t pkt_tags)
{
    if (gw != (void *)0xdeaf) {
        errx(1, "got wrong gw %p (expected 0xdeaf)", gw);
    }

    if (bulb_tags != 0x2a) {
        errx(1, "got bulb_tags %#jx (expected 0x2a)", (uintmax_t)bulb_tags);
    }

    if (pkt_tags != 0xfeed) {
        errx(1, "got pkt_tags %#jx (expected 0xfeed)", (uintmax_t)pkt_tags);
    }

    update_tag_refcouts_called = true;
}

int
main(void)
{
    struct lgtd_lifx_bulb bulb = {
        .state = { .tags = 0x2a },
        .gw = (void *)0xdeaf
    };

    lgtd_lifx_bulb_set_tags(&bulb, 0xfeed);

    if (bulb.state.tags != 0xfeed) {
        errx(
            1, "got bulb.state.tags = %#jx (expected 0xfeed)",
            (uintmax_t)bulb.state.tags
        );
    }

    if (!update_tag_refcouts_called) {
        errx(1, "lgtd_lifx_gateway_update_tag_refcounts wasn't called");
    }

    return 0;
}
