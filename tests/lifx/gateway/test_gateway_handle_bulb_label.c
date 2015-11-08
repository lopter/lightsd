#include <string.h>

#include "gateway.c"

#include "mock_log.h"
#include "mock_timer.h"
#include "test_gateway_utils.h"
#include "tests_utils.h"

int
main(void)
{
    lgtd_lifx_wire_load_packet_info_map();

    struct lgtd_lifx_gateway gw = { .last_pkt_at = 42 };

    struct lgtd_lifx_bulb *b = lgtd_tests_insert_mock_bulb(&gw, 42);

    struct lgtd_lifx_packet_header hdr = {
        .packet_type = LGTD_LIFX_BULB_LABEL
    };
    memcpy(&hdr.target.device_addr, &b->addr, sizeof(hdr.target.device_addr));
    struct lgtd_lifx_packet_label pkt = { .label = "TEST LABEL" };

    lgtd_lifx_gateway_handle_bulb_label(&gw, &hdr, &pkt);

    if (memcmp(&b->state.label, &pkt.label, LGTD_LIFX_LABEL_SIZE)) {
        errx(
            1, "got label %2$.*1$s, (expected %3$.*1$s)",
            LGTD_LIFX_LABEL_SIZE, b->state.label, pkt.label
        );
    }

    return 0;
}
