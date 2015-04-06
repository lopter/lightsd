#include "router.c"

#include "tests_utils.h"
#include "tests_router_utils.h"

int
main(void)
{
    lgtd_lifx_wire_load_packet_infos_map();

    struct lgtd_lifx_gateway *gw_1 = lgtd_tests_insert_mock_gateway(1);
    struct lgtd_lifx_bulb *bulb_1 = lgtd_tests_insert_mock_bulb(gw_1, 1);
    struct lgtd_lifx_gateway *gw_2 = lgtd_tests_insert_mock_gateway(2);
    lgtd_tests_insert_mock_bulb(gw_2, 2);

    struct lgtd_lifx_packet_power_state payload = {
        .power = LGTD_LIFX_POWER_ON
    };
    struct lgtd_proto_target_list *targets;
    targets = lgtd_tests_build_target_list("1", NULL);
    lgtd_router_send(targets, LGTD_LIFX_SET_POWER_STATE, &payload);

    if (lgtd_tests_gw_pkt_queue_size != 1) {
        lgtd_errx(1, "1 packet should have been sent");
    }

    struct lgtd_lifx_gateway *recpt_gw = lgtd_tests_gw_pkt_queue[0].gw;
    struct lgtd_lifx_packet_header *hdr_queued = lgtd_tests_gw_pkt_queue[0].hdr;
    const void *pkt_queued = lgtd_tests_gw_pkt_queue[0].pkt;
    int pkt_size = lgtd_tests_gw_pkt_queue[0].pkt_size;

    if (recpt_gw != gw_1) {
        lgtd_errx(1, "the packet has been sent to the wrong gateway");
    }

    if (!hdr_queued->protocol.addressable || hdr_queued->protocol.tagged) {
        lgtd_errx(1, "the packet header doesn't have the right protocol flags");
    }

    if (memcmp(hdr_queued->target.device_addr, bulb_1->addr, sizeof(bulb_1->addr))) {
        lgtd_errx(1, "the packet header doesn't have the right target address");
    }

    if (memcmp(gw_1->site, hdr_queued->site, sizeof(hdr_queued->site))) {
        lgtd_errx(1, "incorrect site in the headers");
    }

    if (pkt_queued != &payload) {
        lgtd_errx(1, "invalid payload");
    }

    if (pkt_size != sizeof(payload)) {
        lgtd_errx(
            1, "unexpected pkt size %d (expected %ld)",
            pkt_size, sizeof(payload)
        );
    }

    return 0;
}
