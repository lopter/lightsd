#include <string.h>

#include "gateway.c"

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
    if (tag_id != 4 && tag_id != 63) {
        errx(1, "got tag_id %d but expected 4 or 63", tag_id);
    }

    static struct lgtd_lifx_tag *tag = NULL;

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
    lgtd_lifx_wire_load_packet_infos_map();

    struct lgtd_lifx_gateway gw;
    memset(&gw, 0, sizeof(gw));

    struct lgtd_lifx_packet_header hdr;
    memset(&hdr, 0, sizeof(hdr));

    lgtd_lifx_gateway_allocate_tag_id(&gw, 4, "test");
    if (!gw.tags[4]) {
        errx(1, "gw.tag_ids[4] shouldn't be NULL");
    }
    if (strcmp(gw.tags[4]->label, "test")) {
        errx(
            1, "unexpected tag %.*s (expected test)",
            (int)sizeof(gw.tags[0]->label), gw.tags[4]->label
        );
    }
    if (gw.tag_ids != LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(4)) {
        errx(
            1, "tag_ids = %jx (expected %jx)",
            (uintmax_t)gw.tag_ids, (uintmax_t)LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(4)
        );
    }

    lgtd_lifx_gateway_allocate_tag_id(&gw, 63, "test");
    if (!gw.tags[4]) {
        errx(1, "gw.tag_ids[4] shouldn't be NULL");
    }
    if (strcmp(gw.tags[4]->label, "test")) {
        errx(
            1, "unexpected tag %.*s (expected test)",
            (int)sizeof(gw.tags[0]->label), gw.tags[4]->label
        );
    }
    if (!gw.tags[63]) {
        errx(1, "gw.tag_ids[63] shouldn't be NULL");
    }
    if (strcmp(gw.tags[63]->label, "test")) {
        errx(
            1, "unexpected tag %.*s (expected test)",
            (int)sizeof(gw.tags[0]->label), gw.tags[4]->label
        );
    }
    uint64_t expected_tags = LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(4)
                             | LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(63);
    if (gw.tag_ids != expected_tags) {
        errx(
            1, "tag_ids = %jx (expected %jx)",
            (uintmax_t)gw.tag_ids, (uintmax_t)expected_tags
        );
    }

    tagging_incref_called = false;
    lgtd_lifx_gateway_allocate_tag_id(&gw, 4, "test");
    if (!gw.tags[4]) {
        errx(1, "gw.tag_ids[4] shouldn't be NULL");
    }
    if (strcmp(gw.tags[4]->label, "test")) {
        errx(
            1, "unexpected tag %.*s (expected test)",
            (int)sizeof(gw.tags[0]->label), gw.tags[4]->label
        );
    }
    if (!gw.tags[63]) {
        errx(1, "gw.tag_ids[63] shouldn't be NULL");
    }
    if (strcmp(gw.tags[63]->label, "test")) {
        errx(
            1, "unexpected tag %.*s (expected test)",
            (int)sizeof(gw.tags[0]->label), gw.tags[4]->label
        );
    }
    if (gw.tag_ids != expected_tags) {
        errx(
            1, "tag_ids = %jx (expected %jx)",
            (uintmax_t)gw.tag_ids, (uintmax_t)expected_tags
        );
    }
    if (tagging_incref_called) {
        errx(1, "lgtd_lifx_tagging_incref shouldn't have been called");
    }

    return 0;
}
