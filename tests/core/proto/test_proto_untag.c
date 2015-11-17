#include "proto.c"

#include "mock_client_buf.h"
#include "mock_daemon.h"
#include "mock_gateway.h"
#include "mock_event2.h"
#include "mock_log.h"
#include "mock_timer.h"
#define MOCKED_LGTD_LIFX_WIRE_ENCODE_TAGS
#include "mock_wire_proto.h"
#include "tests_utils.h"

#define MOCKED_ROUTER_TARGETS_TO_DEVICES
#define MOCKED_ROUTER_SEND_TO_DEVICE
#define MOCKED_ROUTER_DEVICE_LIST_FREE
#include "tests_proto_utils.h"

static bool device_list_free_called = false;

static int lifx_wire_encode_tags_call_count = 0;

void
lgtd_lifx_wire_encode_tags(struct lgtd_lifx_packet_tags *pkt)
{
    (void)pkt;

    lifx_wire_encode_tags_call_count++;
}

void
lgtd_router_device_list_free(struct lgtd_router_device_list *devices)
{
    if (device_list_free_called) {
        errx(1, "the device list should have been freed once");
    }

    if (!devices) {
        errx(1, "the device list must be passed");
    }

    device_list_free_called = true;
}

static struct lgtd_lifx_tag *tag_vapor = NULL;

struct lgtd_router_device_list *
lgtd_router_targets_to_devices(const struct lgtd_proto_target_list *targets)
{
    if (targets != (void *)0x2a) {
        lgtd_errx(1, "unexpected targets list");
    }

    static struct lgtd_router_device_list devices =
        SLIST_HEAD_INITIALIZER(&devices);

    static struct lgtd_lifx_gateway gw_bulb_1 = {
        .bulbs = LIST_HEAD_INITIALIZER(&gw_bulb_1.bulbs)
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
            .tags = 0
        },
        .gw = &gw_bulb_1
    };
    static struct lgtd_router_device device_1 = { .device = &bulb_1 };
    SLIST_INSERT_HEAD(&devices, &device_1, link);

    struct lgtd_lifx_tag *gw_2_tag_2 = lgtd_tests_insert_mock_tag("d^-^b");
    struct lgtd_lifx_tag *gw_2_tag_3 = lgtd_tests_insert_mock_tag("wave~");
    static struct lgtd_lifx_gateway gw_bulb_2 = {
        .bulbs = LIST_HEAD_INITIALIZER(&gw_bulb_2.bulbs),
        .tag_ids = 0x7
    };
    lgtd_tests_add_tag_to_gw(tag_vapor, &gw_bulb_2, 0);
    lgtd_tests_add_tag_to_gw(gw_2_tag_2, &gw_bulb_2, 1);
    lgtd_tests_add_tag_to_gw(gw_2_tag_3, &gw_bulb_2, 2);
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

    return &devices;
}

static bool send_to_device_called = false;

void
lgtd_router_send_to_device(struct lgtd_lifx_bulb *bulb,
                           enum lgtd_lifx_packet_type pkt_type,
                           void *pkt)
{
    if (send_to_device_called) {
        errx(1, "lgtd_router_send_to_device should have been called once");
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

    struct lgtd_lifx_packet_tags *pkt_tags = pkt;
    if (lifx_wire_encode_tags_call_count != 1) {
        errx(
            1, "lifx_wire_encode_tags_call_count = %d (expected 1)",
            lifx_wire_encode_tags_call_count
        );
    }
    if (pkt_tags->tags != 0x2) {
        errx(
            1, "invalid SET_TAGS payload=%#jx (expected %#x)",
            (uintmax_t)pkt_tags->tags, 0x2
        );
    }

    send_to_device_called = true;
}

int
main(void)
{
    struct lgtd_client *client;
    client = lgtd_tests_insert_mock_client(FAKE_BUFFEREVENT);

    struct lgtd_proto_target_list *targets = (void *)0x2a;

    tag_vapor = lgtd_tests_insert_mock_tag("vapor");

    lgtd_proto_untag(client, targets, "vapor");

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

    if (!device_list_free_called) {
        lgtd_errx(1, "the list of devices hasn't been freed");
    }
    if (!send_to_device_called) {
        lgtd_errx(1, "nothing was send to any device");
    }

    return 0;
}
