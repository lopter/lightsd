#include "gateway.c"

#include <string.h>

#define MOCKED_LIFX_TAGGING_INCREF
#include "test_gateway_utils.h"

static bool tagging_incref_called = false;

struct lgtd_lifx_tag *
lgtd_lifx_tagging_incref(const char *label,
                         struct lgtd_lifx_gateway *gw,
                         int tag_id)
{
    if (!label) {
        errx(1, "missing tag label");
    }
    if (!gw) {
        errx(1, "missing gateway");
    }
    if (tag_id > 2) {
        errx(1, "got tag_id %d but expected < 3", tag_id);
    }

    struct lgtd_lifx_tag *tag = lgtd_lifx_tagging_find_tag(label);
    if (!tag) {
        tag = calloc(1, sizeof(*tag));
        strcpy(tag->label, label);
        struct lgtd_lifx_site *site = calloc(1, sizeof(*site));
        site->gw = gw;
        site->tag_id = tag_id;
        LIST_INSERT_HEAD(&tag->sites, site, link);
    }

    tagging_incref_called = true;

    return tag;
}

int
main(void)
{
    lgtd_lifx_wire_load_packet_info_map();

    struct lgtd_lifx_gateway gw;
    memset(&gw, 0, sizeof(gw));

    struct lgtd_lifx_packet_header hdr;
    memset(&hdr, 0, sizeof(hdr));

    uint64_t expected_tag_ids = LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(0);

    lgtd_lifx_gateway_allocate_tag_id(&gw, 0, "test");
    if (!gw.tags[0]) {
        errx(1, "gw.tag_ids[0] shouldn't be NULL");
    }
    if (strcmp(gw.tags[0]->label, "test")) {
        errx(
            1, "unexpected tag %.*s (expected test)",
            (int)sizeof(gw.tags[0]->label), gw.tags[0]->label
        );
    }
    if (gw.tag_ids != LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(0)) {
        errx(
            1, "tag_ids = %jx (expected %jx)",
            (uintmax_t)gw.tag_ids, (uintmax_t)expected_tag_ids
        );
    }
    if (!tagging_incref_called) {
        errx(1, "lgtd_lifx_tagging_incref should have been called");
    }
    tagging_incref_called = false;

    for (int i = 1; i != 3; i++) {
        int tag_id = lgtd_lifx_gateway_allocate_tag_id(&gw, -1, "lounge");
        if (tag_id < 1) {
            errx(1, "no tag_id was allocated (received tag_id %d)", tag_id);
        }
        if (!gw.tags[tag_id]) {
            errx(1, "gw.tag_ids[%d] shouldn't be NULL", i);
        }
        if (strcmp(gw.tags[tag_id]->label, "lounge")) {
            errx(
                1, "unexpected tag %.*s (expected lounge)",
                (int)sizeof(gw.tags[tag_id]->label), gw.tags[tag_id]->label
            );
        }
        expected_tag_ids |= LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(tag_id);
        if (gw.tag_ids != expected_tag_ids) {
            errx(
                1, "tag_ids = %jx (expected %jx)",
                (uintmax_t)gw.tag_ids, (uintmax_t)expected_tag_ids
            );
        }
        if (!tagging_incref_called) {
            errx(1, "lgtd_lifx_tagging_incref should have been called");
        }
        tagging_incref_called = false;
    }

    return 0;
}
