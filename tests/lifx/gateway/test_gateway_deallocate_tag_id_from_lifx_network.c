#include <string.h>

#include "gateway.c"

#define MOCKED_LIFX_TAGGING_DECREF
#include "test_gateway_utils.h"

static bool tagging_decref_called = false;

void
lgtd_lifx_tagging_decref(struct lgtd_lifx_tag *tag, struct lgtd_lifx_gateway *gw)
{
    if (!tag) {
        errx(1, "missing tag");
    }
    if (!gw) {
        errx(1, "missing gateway");
    }

    tagging_decref_called = true;
}

int
main(void)
{
    lgtd_lifx_wire_load_packet_infos_map();

    struct lgtd_lifx_gateway gw;
    memset(&gw, 0, sizeof(gw));

    struct lgtd_lifx_packet_header hdr;
    memset(&hdr, 0, sizeof(hdr));

    struct lgtd_lifx_tag tag = {
        .label = "test",
        .sites = LIST_HEAD_INITIALIZER(&tag.sites)
    };

    gw.tags[0] = &tag;
    gw.tag_ids = LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(0)
                | LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(42);

    lgtd_lifx_gateway_deallocate_tag_id(&gw, 0);
    if (gw.tags[0]) {
        errx(1, "gw.tags[0] should have been set to NULL");
    }
    if (gw.tag_ids != LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(42)) {
        errx(
            1, "unexpected gw.tag_ids value = %jx (expected %jx)",
            (uintmax_t)gw.tag_ids, (uintmax_t)LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(42)
        );
    }
    if (!tagging_decref_called) {
        errx(1, "lgtd_lifx_tagging_decref should have been called");
    }

    tagging_decref_called = false;
    lgtd_lifx_gateway_deallocate_tag_id(&gw, 0);
    if (gw.tags[0]) {
        errx(1, "gw.tags[0] should be NULL");
    }
    if (gw.tag_ids != LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(42)) {
        errx(
            1, "unexpected gw.tag_ids value = %jx (expected %jx)",
            (uintmax_t)gw.tag_ids, (uintmax_t)LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(42)
        );
    }
    if (tagging_decref_called) {
        errx(1, "lgtd_lifx_tagging_decref shouldn't have been called");
    }

    return 0;
}
