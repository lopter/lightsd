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
        .packet_type = LGTD_LIFX_INFO_STATE
    };
    memcpy(&hdr.target.device_addr, &b->addr, sizeof(hdr.target.device_addr));
    struct lgtd_lifx_packet_runtime_info pkt = {
        .time = 1, .downtime = 2, .uptime = 3
    };

    lgtd_lifx_gateway_handle_runtime_info(&gw, &hdr, &pkt);

    if (memcmp(&b->runtime_info, &pkt, sizeof(pkt))) {
        errx(1, "the product info weren't set correctly on the bulb");
    }

    if (b->runtime_info_updated_at != gw.last_pkt_at) {
        errx(
            1, "runtime_info_updated_at = %ju (expected 42)",
            (uintmax_t)b->runtime_info_updated_at
        );
    }

    return 0;
}
