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

    struct lgtd_lifx_gateway gw = { .last_pkt_at = 42 };

    struct lgtd_lifx_bulb *b = lgtd_tests_insert_mock_bulb(&gw, 42);

    struct lgtd_lifx_packet_header hdr = {
        .packet_type = LGTD_LIFX_VERSION_STATE
    };
    memcpy(&hdr.target.device_addr, &b->addr, sizeof(hdr.target.device_addr));
    struct lgtd_lifx_packet_product_info pkt = {
        .vendor_id = 1, .product_id = 3, .version = 42
    };

    lgtd_lifx_gateway_handle_product_info(&gw, &hdr, &pkt);

    if (memcmp(&b->product_info, &pkt, sizeof(pkt))) {
        errx(1, "the product info weren't set correctly on the bulb");
    }

    const char *expected_model = "Color 650";
    if (strcmp(b->model, expected_model)) {
        errx(1, "model %s (expected %s)", b->model, expected_model);
    }

    const char *expected_vendor = "LIFX";
    if (strcmp(b->vendor, expected_vendor)) {
        errx(1, "vendor %s (expected %s)", b->vendor, expected_vendor);
    }

    return 0;
}
