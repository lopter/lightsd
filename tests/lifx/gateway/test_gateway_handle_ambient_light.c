#include <string.h>

#include "gateway.c"

#include "mock_timer.h"
#include "test_gateway_utils.h"
#include "tests_utils.h"

int
main(void)
{
    lgtd_lifx_wire_load_packet_info_map();

    struct lgtd_lifx_gateway gw;
    memset(&gw, 0, sizeof(gw));

    struct lgtd_lifx_bulb *bulb = lgtd_tests_insert_mock_bulb(&gw, 42);

    struct lgtd_lifx_packet_header hdr;
    memset(&hdr, 0, sizeof(hdr));
    memcpy(
        &hdr.target.device_addr, &bulb->addr, sizeof(hdr.target.device_addr)
    );

    struct lgtd_lifx_packet_ambient_light pkt = { .illuminance = 3.14 };

    lgtd_lifx_gateway_handle_ambient_light(&gw, &hdr, &pkt);

    if (bulb->ambient_light != pkt.illuminance) {
        errx(
            1, "bulb->ambient_light = %f (expected %f)",
            bulb->ambient_light, pkt.illuminance
        );
    }

    return 0;
}
