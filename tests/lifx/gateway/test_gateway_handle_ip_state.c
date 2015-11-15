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

    struct lgtd_lifx_packet_header hdr;
    memset(&hdr, 0, sizeof(hdr));
    memcpy(&hdr.target.device_addr, &b->addr, sizeof(hdr.target.device_addr));

    struct lgtd_lifx_packet_ip_state pkt;
    struct lgtd_lifx_bulb_ip *ip;

    memset(&pkt, 'A', sizeof(pkt));
    hdr.packet_type = LGTD_LIFX_MESH_INFO;
    lgtd_lifx_gateway_handle_ip_state(&gw, &hdr, &pkt);
    ip = &b->ips[LGTD_LIFX_BULB_MCU_IP];
    if (memcmp(&ip->state, &pkt, sizeof(pkt))) {
        errx(1, "The MCU ip state wasn't set properly");
    }
    if (ip->state_updated_at != 42) {
        errx(
            1, "state_updated_at %ju (expected 42)",
            (uintmax_t)ip->state_updated_at
        );
    }

    memset(&pkt, 'B', sizeof(pkt));
    hdr.packet_type = LGTD_LIFX_WIFI_INFO;
    lgtd_lifx_gateway_handle_ip_state(&gw, &hdr, &pkt);
    ip = &b->ips[LGTD_LIFX_BULB_WIFI_IP];
    if (memcmp(&ip->state, &pkt, sizeof(pkt))) {
        errx(1, "The WIFI ip state wasn't set properly");
    }
    if (ip->state_updated_at != 42) {
        errx(
            1, "state_updated_at %ju (expected 42)",
            (uintmax_t)ip->state_updated_at
        );
    }

    return 0;
}
