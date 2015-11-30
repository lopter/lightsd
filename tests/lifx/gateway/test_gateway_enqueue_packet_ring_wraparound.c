#include "gateway.c"

#include "test_gateway_utils.h"
#include "mock_log.h"
#include "mock_timer.h"
#include "mock_wire_proto.h"

int
main(void)
{
    lgtd_lifx_wire_setup();

    struct lgtd_lifx_gateway gw;
    memset(&gw, 0, sizeof(gw));
    gw.socket_ev = (void *)42;

    struct lgtd_lifx_packet_power_state pkt;
    pkt.power = LGTD_LIFX_POWER_ON;

    union lgtd_lifx_target target = { .tags = 0 };

    struct lgtd_lifx_packet_header hdr;
    const struct lgtd_lifx_packet_info *pkt_info = lgtd_lifx_wire_setup_header(
        &hdr,
        LGTD_LIFX_TARGET_ALL_DEVICES,
        target,
        gw.site.as_array,
        LGTD_LIFX_SET_POWER_STATE
    );

    int pkt_ring_last_idx = LGTD_ARRAY_SIZE(gw.pkt_ring) - 1;

    // set the head so it has to wrap-around and set the tail somewhere:
    gw.pkt_ring_head = pkt_ring_last_idx;
    gw.pkt_ring_tail = 2;

    lgtd_lifx_gateway_enqueue_packet(&gw, &hdr, pkt_info, &pkt);

    if (memcmp(gw_write_buf, &hdr, sizeof(hdr))) {
        errx(1, "header incorrectly buffered");
    }

    if (memcmp(&gw_write_buf[sizeof(hdr)], &pkt, sizeof(pkt))) {
        errx(1, "pkt incorrectly buffered");
    }

    if (gw.pkt_ring[pkt_ring_last_idx].type != LGTD_LIFX_SET_POWER_STATE) {
        errx(1, "packet type incorrectly enqueued");
    }

    if (gw.pkt_ring[pkt_ring_last_idx].size != sizeof(pkt) + sizeof(hdr)) {
        errx(1, "packet size incorrectly enqueued");
    }

    if (gw.pkt_ring_head != 0) {
        errx(1, "packet ring head should have wrapped around to index 0");
    }

    if (gw.pkt_ring_tail != 2) {
        errx(1, "packet ring tail should be on index 2");
    }

    if (gw.pkt_ring_full == true) {
        errx(1, "packet ring shouldn't be full");
    }

    if (last_event_passed_to_event_add != gw.socket_ev) {
        errx(1, "event_add should have been called with gw.socket_ev");
    }

    return 0;
}
