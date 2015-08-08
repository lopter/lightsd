#include "gateway.c"

#include "test_gateway_utils.h"

int
main(void)
{
    lgtd_lifx_wire_load_packet_infos_map();

    struct lgtd_lifx_gateway gw;
    memset(&gw, 0, sizeof(gw));

    lgtd_lifx_gateway_update_tag_refcounts(&gw, 0, 0);
    for (int i = 0; i != LGTD_LIFX_GATEWAY_MAX_TAGS; i++) {
        if (gw.tag_refcounts[i]) {
            errx(
                1, "gw.tag_refcounts[%d] was %d, (expected 0)",
                i, gw.tag_refcounts[i]
            );
        }
    }

    for (int n = 1; n != 3; n++) {
        lgtd_lifx_gateway_update_tag_refcounts(&gw, 0, 1);
        if (gw.tag_refcounts[0] != n) {
            errx(
                1, "gw.tag_refcounts[0] was %d (expected %d)",
                gw.tag_refcounts[0], n
            );
        }
        for (int i = 1; i != LGTD_LIFX_GATEWAY_MAX_TAGS; i++) {
            if (gw.tag_refcounts[i]) {
                errx(
                    1, "gw.tag_refcounts[%d] was %d (expected 0)",
                    i, gw.tag_refcounts[i]
                );
            }
        }
    }

    lgtd_lifx_gateway_update_tag_refcounts(&gw, 0, 2);
    gw.tag_ids = 0x2;

    for (int n = 1; n >= 0; n--) {
        lgtd_lifx_gateway_update_tag_refcounts(&gw, 1, 0);
        if (gw.tag_refcounts[0] != n) {
            errx(
                1, "gw.tag_refcounts[0] was %d (expected %d)",
                gw.tag_refcounts[0], n - 1
            );
        }
        if (gw.tag_refcounts[1] != 1) {
            errx(
                1, "gw.tag_refcounts[1] was %d (expected 1)",
                gw.tag_refcounts[1]
            );
        }
        for (int i = 2; i != LGTD_LIFX_GATEWAY_MAX_TAGS; i++) {
            if (gw.tag_refcounts[i]) {
                errx(
                    1, "gw.tag_refcounts[%d] was %d (expected 0)",
                    i, gw.tag_refcounts[i]
                );
            }
        }
    }
    if (gw.pkt_ring[0].type != LGTD_LIFX_SET_TAG_LABELS) {
        errx(1, "SET_TAG_LABELS should have been enqueued on the gateway");
    }

    struct lgtd_lifx_packet_tag_labels *pkt =
        (void *)&gw_write_buf[sizeof(struct lgtd_lifx_packet_header)];
    uint64_t tags = le64toh(pkt->tags);
    if (tags != ~2ULL) {
        errx(
            1, "tags on LGTD_LIFX_SET_TAG_LABELS was %#jx (expected %#jx)",
            (uintmax_t)tags, (uintmax_t)~2ULL
        );
    }
    const char blank_label[LGTD_LIFX_LABEL_SIZE] = { 0 };
    if (memcmp(pkt->label, blank_label, LGTD_LIFX_LABEL_SIZE)) {
        errx(
            1, "label on LGTD_LIFX_SET_TAG_LABELS should be "
            "all zero but got %.*s", LGTD_LIFX_LABEL_SIZE, pkt->label
        );
    }

    for (int n = 0; n != UINT8_MAX; n++) {
        lgtd_lifx_gateway_update_tag_refcounts(&gw, 0, 4);
    }
    if (gw.tag_refcounts[2] != UINT8_MAX) {
        errx(
            1, "gw.tag_refcounts[2] was %d (expected %d)",
            gw.tag_refcounts[2], UINT8_MAX
        );
    }
    lgtd_lifx_gateway_update_tag_refcounts(&gw, 0, 4);
    if (gw.tag_refcounts[2] != UINT8_MAX) {
        errx(
            1, "gw.tag_refcounts[2] was %d (expected %d)",
            gw.tag_refcounts[2], UINT8_MAX
        );
    }

    return 0;
}
