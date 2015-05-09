#include "gateway.c"

#include "test_gateway_utils.h"

int
main(void)
{
    lgtd_lifx_wire_load_packet_infos_map();

    struct lgtd_lifx_gateway gw;
    memset(&gw, 0, sizeof(gw));
    gw.write_ev = (void *)42;

    struct lgtd_lifx_packet_power_state pkt;
    pkt.power = LGTD_LIFX_POWER_ON;

    union lgtd_lifx_target target = { .tags = 0 };

    struct lgtd_lifx_packet_header hdr;
    lgtd_lifx_wire_setup_header(
        &hdr,
        LGTD_LIFX_TARGET_ALL_DEVICES,
        target,
        gw.site.as_array,
        LGTD_LIFX_SET_POWER_STATE
    );

    lgtd_lifx_gateway_enqueue_packet(
        &gw, &hdr, LGTD_LIFX_SET_POWER_STATE, &pkt, sizeof(pkt)
    );

    if (memcmp(gw_write_buf, &hdr, sizeof(hdr))) {
        errx(1, "header incorrectly buffered");
    }

    if (memcmp(&gw_write_buf[sizeof(hdr)], &pkt, sizeof(pkt))) {
        errx(1, "pkt incorrectly buffered");
    }

    if (gw.pkt_ring[0].type != LGTD_LIFX_SET_POWER_STATE) {
        errx(1, "packet type incorrectly enqueued");
    }

    if (gw.pkt_ring[0].size != sizeof(pkt) + sizeof(hdr)) {
        errx(1, "packet size incorrectly enqueued");
    }

    if (gw.pkt_ring_head != 1) {
        errx(1, "packet ring head should be on index 1");
    }

    if (gw.pkt_ring_tail != 0) {
        errx(1, "packet ring tail should be on index 0");
    }

    if (gw.pkt_ring_full == true) {
        errx(1, "packet ring shouldn't be full");
    }

    if (last_event_passed_to_event_add != gw.write_ev) {
        errx(1, "event_add should have been called with gw.write_ev");
    }

    return 0;
}
