#include <string.h>

#include "gateway.c"

#include "mock_log.h"
#include "mock_timer.h"
#include "test_gateway_utils.h"
#include "tests_utils.h"
#include "mock_wire_proto.h"

int
main(void)
{
    lgtd_lifx_wire_setup();

    struct lgtd_lifx_gateway gw;
    memset(&gw, 0, sizeof(gw));

    struct lgtd_lifx_bulb *bulb = lgtd_tests_insert_mock_bulb(&gw, 42);

    struct lgtd_lifx_packet_header hdr;
    memset(&hdr, 0, sizeof(hdr));
    memcpy(
        &hdr.target.device_addr, &bulb->addr, sizeof(hdr.target.device_addr)
    );

    struct lgtd_lifx_packet_tags pkt = { .tags = 0x7 };

    lgtd_lifx_gateway_handle_tags(&gw, &hdr, &pkt);

    if (bulb->state.tags != 0x7) {
        errx(
            1, "bulb->tags = %#jx (expected 0x7)", (uintmax_t)bulb->state.tags
        );
    }

    return 0;
}
