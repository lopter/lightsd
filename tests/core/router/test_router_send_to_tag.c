#include "router.c"

#include "mock_daemon.h"
#include "mock_timer.h"
#include "tests_utils.h"
#include "tests_router_utils.h"

int
main(void)
{
    lgtd_lifx_wire_load_packet_info_map();

    struct lgtd_lifx_gateway *gw_1 = lgtd_tests_insert_mock_gateway(1);
    struct lgtd_lifx_gateway *gw_2 = lgtd_tests_insert_mock_gateway(2);

    struct lgtd_lifx_tag *tag_foo = lgtd_tests_insert_mock_tag("foo");
    lgtd_tests_add_tag_to_gw(tag_foo, gw_1, 42);

    struct lgtd_lifx_packet_power_state payload = {
        .power = LGTD_LIFX_POWER_ON
    };
    struct lgtd_proto_target_list *targets;
    targets = lgtd_tests_build_target_list("#foo", NULL);
    lgtd_router_send(targets, LGTD_LIFX_SET_POWER_STATE, &payload);

    if (lgtd_tests_gw_pkt_queue_size != 1) {
        lgtd_errx(1, "1 packet should have been sent");
    }

    struct lgtd_lifx_gateway *recpt_gw = lgtd_tests_gw_pkt_queue[0].gw;
    struct lgtd_lifx_packet_header *hdr_queued = lgtd_tests_gw_pkt_queue[0].hdr;
    const void *pkt_queued = lgtd_tests_gw_pkt_queue[0].pkt;
    int pkt_size = lgtd_tests_gw_pkt_queue[0].pkt_size;

    lgtd_lifx_wire_decode_header(hdr_queued);

    if (recpt_gw != gw_1) {
        lgtd_errx(1, "the packet has been sent to the wrong gateway");
    }

    int expected_flags = LGTD_LIFX_ADDRESSABLE|LGTD_LIFX_TAGGED;
    if (!lgtd_tests_lifx_header_has_flags(hdr_queued, expected_flags)) {
        lgtd_errx(1, "the packet header doesn't have the right protocol flags");
    }

    if (hdr_queued->target.tags != LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(42)) {
        lgtd_errx(1, "the packet header doesn't have the right tags set");
    }

    if (memcmp(gw_1->site.as_array, hdr_queued->site, sizeof(hdr_queued->site))) {
        lgtd_errx(1, "incorrect site in the headers");
    }

    if (pkt_queued != &payload) {
        lgtd_errx(1, "invalid payload");
    }

    if (pkt_size != sizeof(payload)) {
        lgtd_errx(
            1, "unexpected pkt size %d (expected %ju)",
            pkt_size, (uintmax_t)sizeof(payload)
        );
    }

    lgtd_tests_router_reset_pkt_queue();

    struct lgtd_lifx_tag *tag_bar = lgtd_tests_insert_mock_tag("bar");
    lgtd_tests_add_tag_to_gw(tag_bar, gw_1, 26);
    lgtd_tests_add_tag_to_gw(tag_foo, gw_2, 21);

    targets = lgtd_tests_build_target_list("#foo", "#bar", NULL);
    lgtd_router_send(targets, LGTD_LIFX_SET_POWER_STATE, &payload);

    if (lgtd_tests_gw_pkt_queue_size != 3) {
        lgtd_errx(1, "3 packet should have been sent");
    }

    int count_tag_foo_gw_1 = 0;
    int count_tag_bar_gw_1 = 0;
    int count_tag_foo_gw_2 = 0;
    int count_other = 0;
    for (int i = 0; lgtd_tests_gw_pkt_queue[i].gw; i++) {
        recpt_gw = lgtd_tests_gw_pkt_queue[i].gw;
        hdr_queued = lgtd_tests_gw_pkt_queue[i].hdr;
        pkt_queued = lgtd_tests_gw_pkt_queue[i].pkt;
        pkt_size = lgtd_tests_gw_pkt_queue[i].pkt_size;

        lgtd_lifx_wire_decode_header(hdr_queued);

        if (!lgtd_tests_lifx_header_has_flags(hdr_queued, expected_flags)) {
            lgtd_errx(1, "the packet header doesn't have the right protocol flags");
        }

        if (memcmp(recpt_gw->site.as_array, hdr_queued->site, sizeof(hdr_queued->site))) {
            lgtd_errx(1, "incorrect site in the headers");
        }

        if (pkt_queued != &payload) {
            lgtd_errx(1, "invalid payload");
        }

        if (pkt_size != sizeof(payload)) {
            lgtd_errx(
                1, "unexpected pkt size %d (expected %ju)",
                pkt_size, (uintmax_t)sizeof(payload)
            );
        }

        if (recpt_gw == gw_1) {
            if (hdr_queued->target.tags == LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(42)) {
                count_tag_foo_gw_1++;
            } else if (hdr_queued->target.tags == LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(26)) {
                count_tag_bar_gw_1++;
            } else {
                count_other++;
            }
        } else if (recpt_gw == gw_2) {
            if (hdr_queued->target.tags == LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(21)) {
                count_tag_foo_gw_2++;
            } else {
                count_other++;
            }
        } else {
            lgtd_errx(1, "unexpected gateway %p", recpt_gw);
        }
    }

    if (count_tag_foo_gw_1 != 1) {
        lgtd_errx(1, "The packet for #foo should have been enqueued on gw_1");
    }
    if (count_tag_bar_gw_1 != 1) {
        lgtd_errx(1, "The packet for #bar should have been enqueued on gw_1");
    }
    if (count_tag_foo_gw_2 != 1) {
        lgtd_errx(1, "The packet for #foo should have been enqueued on gw_2");
    }
    if (count_other) {
        lgtd_errx(1, "Unexpected packets have been enqueued");
    }

    return 0;
}
