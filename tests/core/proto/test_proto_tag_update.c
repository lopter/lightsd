#include "proto.c"

#include "mock_client_buf.h"
#include "mock_daemon.h"
#define MOCKED_LIFX_GATEWAY_SEND_TO_SITE
#define MOCKED_LIFX_GATEWAY_ALLOCATE_TAG_ID
#include "mock_gateway.h"
#include "mock_event2.h"
#include "mock_log.h"
#include "mock_timer.h"
#define MOCKED_LGTD_LIFX_WIRE_ENCODE_TAG_LABELS
#define MOCKED_LGTD_LIFX_WIRE_ENCODE_TAGS
#include "mock_wire_proto.h"
#include "tests_utils.h"

#define MOCKED_ROUTER_TARGETS_TO_DEVICES
#define MOCKED_ROUTER_SEND_TO_DEVICE
#define MOCKED_ROUTER_DEVICE_LIST_FREE
#include "tests_proto_utils.h"

#define FAKE_TARGET_LIST (void *)0x2a

static struct lgtd_router_device_list devices =
    SLIST_HEAD_INITIALIZER(&devices);

static int lifx_wire_encode_tag_labels_call_count = 0;

void
lgtd_lifx_wire_encode_tag_labels(struct lgtd_lifx_packet_tag_labels *pkt)
{
    (void)pkt;

    lifx_wire_encode_tag_labels_call_count++;
}

static int lifx_wire_encode_tags_call_count = 0;

void
lgtd_lifx_wire_encode_tags(struct lgtd_lifx_packet_tags *pkt)
{
    (void)pkt;

    lifx_wire_encode_tags_call_count++;
}

static bool send_to_device_called = false;

void
lgtd_router_send_to_device(struct lgtd_lifx_bulb *bulb,
                           enum lgtd_lifx_packet_type pkt_type,
                           void *pkt)
{
    if (send_to_device_called) {
        errx(1, "lgtd_router_send_to_device should have been called once only");
    }

    if (!bulb) {
        errx(1, "lgtd_router_send_to_device must be called with a bulb");
    }

    uint8_t expected_addr[LGTD_LIFX_ADDR_LENGTH] = { 5, 4, 3, 2, 1 };
    char addr[LGTD_LIFX_ADDR_STRLEN], expected[LGTD_LIFX_ADDR_STRLEN];
    if (memcmp(bulb->addr, expected_addr, LGTD_LIFX_ADDR_LENGTH)) {
        errx(
            1, "got bulb with addr %s (expected %s)",
            LGTD_IEEE8023MACTOA(bulb->addr, addr),
            LGTD_IEEE8023MACTOA(expected_addr, expected)
        );
    }

    if (pkt_type != LGTD_LIFX_SET_TAGS) {
        errx(
            1, "got packet type %d (expected %d)", pkt_type, LGTD_LIFX_SET_TAGS
        );
    }

    if (!pkt) {
        errx(1, "missing SET_TAGS payload");
    }

    const struct lgtd_lifx_packet_tags *pkt_tags = pkt;
    if (lifx_wire_encode_tags_call_count != 1) {
        errx(
            1, "lifx_wire_encode_tags_call_count = %d (expected 1)",
            lifx_wire_encode_tags_call_count
        );
    }
    if (pkt_tags->tags != 0x7) {
        errx(
            1, "invalid SET_TAGS payload=%#jx (expected %#x)",
            (uintmax_t)pkt_tags->tags, 0x7
        );
    }

    send_to_device_called = true;
}

static bool gateway_send_to_site_called_for_gw_1 = false;
static bool gateway_send_to_site_called_for_gw_2 = false;

bool
lgtd_lifx_gateway_send_to_site(struct lgtd_lifx_gateway *gw,
                               enum lgtd_lifx_packet_type pkt_type,
                               void *pkt)
{
    if (!gw) {
        errx(1, "missing gateway");
    }

    if (pkt_type != LGTD_LIFX_SET_TAG_LABELS) {
        errx(
            1, "got packet type %#x (expected %#x)",
            pkt_type, LGTD_LIFX_SET_TAG_LABELS
        );
    }

    const struct lgtd_lifx_packet_tag_labels *pkt_tag_labels = pkt;

    if (strcmp(pkt_tag_labels->label, "dub")) {
        errx(1, "got label %s (expected dub)", pkt_tag_labels->label);
    }

    if (gw->site.as_integer == 42) {
        if (pkt_tag_labels->tags != 0x1) {
            errx(
                1, "got tags %#jx (expected %#x)",
                (uintmax_t)pkt_tag_labels->tags, 0x1
            );
        }
        if (gateway_send_to_site_called_for_gw_1) {
            errx(1, "LGTD_LIFX_SET_TAG_LABELS already called for gw 1");
        }
        gateway_send_to_site_called_for_gw_1 = true;
    } else if (gw->site.as_integer == 44) {
        if (pkt_tag_labels->tags != 0x4) {
            errx(
                1, "got tags %#jx (expected %#x)",
                (uintmax_t)pkt_tag_labels->tags, 0x4
            );
        }
        if (gateway_send_to_site_called_for_gw_2) {
            errx(1, "LGTD_LIFX_SET_TAG_LABELS already called for gw 2");
        }
        gateway_send_to_site_called_for_gw_2 = true;
    } else {
        errx(1, "LGTD_LIFX_SET_TAG_LABELS received an invalid gateway");
    }

    int expected_tag_encode_tag_labels_call_count = (
        (int)(gateway_send_to_site_called_for_gw_1 == true)
        + (int)(gateway_send_to_site_called_for_gw_2 == true)
    );
    if (lifx_wire_encode_tag_labels_call_count
        != expected_tag_encode_tag_labels_call_count) {
        errx(
            1, "lifx_wire_encode_tag_labels_call_count = %d (expected %d)",
            lifx_wire_encode_tag_labels_call_count,
            expected_tag_encode_tag_labels_call_count
        );
    }

    return true;
}

static bool gateway_allocate_tag_id_called = false;

int
lgtd_lifx_gateway_allocate_tag_id(struct lgtd_lifx_gateway *gw,
                                  int tag_id,
                                  const char *tag_label)
{
    if (gateway_allocate_tag_id_called) {
        errx(
            1, "lgtd_lifx_gateway_allocate_tag_id "
            "should have been called once only"
        );
    }

    if (tag_id != -1) {
        errx(
            1, "lgtd_lifx_gateway_allocate_tag_id "
            "tag_id %d (expected -1)", tag_id
        );
    }

    if (!gw) {
        errx(
            1, "lgtd_lifx_gateway_allocate_tag_id "
            "must be called with gateway"
        );
    }

    if (!tag_label) {
        errx(
            1, "lgtd_lifx_gateway_allocate_tag_id "
            "must be called with a tag_label"
        );
    }

    if (gw->site.as_integer != 44) {
        errx(
            1, "lgtd_lifx_gateway_allocate_tag_id got the wrong gateway "
            "%#jx (expected %d)", (uintmax_t)gw->site.as_integer, 44
        );
    }

    tag_id = 2;

    struct lgtd_lifx_tag *tag = lgtd_lifx_tagging_find_tag(tag_label);
    if (!tag) {
        errx(1, "tag %s wasn't found", tag_label);
    }
    lgtd_tests_add_tag_to_gw(tag, gw, tag_id);

    gateway_allocate_tag_id_called = true;

    return tag_id;
}

static bool device_list_free_called = false;

void
lgtd_router_device_list_free(struct lgtd_router_device_list *devices)
{
    if (!devices) {
        lgtd_errx(1, "the device list must be passed");
    }

    device_list_free_called = true;
}

struct lgtd_router_device_list *
lgtd_router_targets_to_devices(const struct lgtd_proto_target_list *targets)
{
    if (targets != FAKE_TARGET_LIST) {
        lgtd_errx(1, "unexpected targets list");
    }

    return &devices;
}

static void
setup_devices(void)
{
    static struct lgtd_lifx_gateway gw_bulb_1 = {
        .bulbs = LIST_HEAD_INITIALIZER(&gw_bulb_1.bulbs),
        .site = { .as_integer = 42 }
    };
    static struct lgtd_lifx_bulb bulb_1 = {
        .addr = { 1, 2, 3, 4, 5 },
        .state = {
            .hue = 0xaaaa,
            .saturation = 0xffff,
            .brightness = 0xbbbb,
            .kelvin = 3600,
            .label = "wave",
            .power = LGTD_LIFX_POWER_ON,
            .tags = 1
        },
        .gw = &gw_bulb_1
    };
    static struct lgtd_router_device device_1 = { .device = &bulb_1 };
    SLIST_INSERT_HEAD(&devices, &device_1, link);
    struct lgtd_lifx_tag *gw_1_tag_1 = lgtd_tests_insert_mock_tag("dub");
    lgtd_tests_add_tag_to_gw(gw_1_tag_1, &gw_bulb_1, 0);

    struct lgtd_lifx_tag *gw_2_tag_1 = lgtd_tests_insert_mock_tag("vapor");
    struct lgtd_lifx_tag *gw_2_tag_2 = lgtd_tests_insert_mock_tag("d^-^b");
    static struct lgtd_lifx_gateway gw_bulb_2 = {
        .bulbs = LIST_HEAD_INITIALIZER(&gw_bulb_2.bulbs),
        .site = { .as_integer = 44 },
        .tag_ids = 0x3
    };
    lgtd_tests_add_tag_to_gw(gw_2_tag_1, &gw_bulb_2, 0);
    lgtd_tests_add_tag_to_gw(gw_2_tag_2, &gw_bulb_2, 1);
    static struct lgtd_lifx_bulb bulb_2 = {
        .addr = { 5, 4, 3, 2, 1 },
        .state = {
            .hue = 0x0000,
            .saturation = 0x0000,
            .brightness = 0xffff,
            .kelvin = 4000,
            .label = "",
            .power = LGTD_LIFX_POWER_OFF,
            .tags = 0x3
        },
        .gw = &gw_bulb_2
    };
    static struct lgtd_router_device device_2 = { .device = &bulb_2 };
    SLIST_INSERT_HEAD(&devices, &device_2, link);
}

int
main(void)
{
    struct lgtd_client *client;
    client = lgtd_tests_insert_mock_client(FAKE_BUFFEREVENT);

    setup_devices();

    lgtd_proto_tag(client, FAKE_TARGET_LIST, "dub");

    const char expected[] = "true";
    if (client_write_buf_idx != sizeof(expected) - 1) {
        lgtd_errx(
            1,
            "%d bytes written, expected %lu "
            "(got %.*s instead of %s)",
            client_write_buf_idx, sizeof(expected) - 1UL,
            client_write_buf_idx, client_write_buf, expected
        );
    }
    if (memcmp(expected, client_write_buf, sizeof(expected) - 1)) {
        lgtd_errx(
            1, "got %.*s instead of %s",
            client_write_buf_idx, client_write_buf, expected
        );
    }

    if (!gateway_send_to_site_called_for_gw_1) {
        lgtd_errx(1, "SET_TAG_LABELS wasn't sent to gw 1");
    }
    if (!gateway_send_to_site_called_for_gw_2) {
        lgtd_errx(1, "SET_TAG_LABELS wasn't sent to gw 2");
    }
    if (!device_list_free_called) {
        lgtd_errx(1, "the list of devices hasn't been freed");
    }
    if (!send_to_device_called) {
        lgtd_errx(1, "SET_TAGS wasn't send to any device");
    }

    return 0;
}
