#include "proto.c"

#include "mock_client_buf.h"
#include "mock_daemon.h"
#include "mock_gateway.h"
#include "mock_event2.h"
#include "mock_log.h"
#include "mock_timer.h"
#include "tests_utils.h"

#define MOCKED_ROUTER_SEND_TO_DEVICE
#define MOCKED_ROUTER_TARGETS_TO_DEVICES
#define MOCKED_ROUTER_DEVICE_LIST_FREE
#include "tests_proto_utils.h"

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

    struct lgtd_lifx_tag *gw_2_tag_1 = lgtd_tests_insert_mock_tag("vapor");
    struct lgtd_lifx_tag *gw_2_tag_2 = lgtd_tests_insert_mock_tag("d^-^b");
    struct lgtd_lifx_tag *gw_2_tag_3 = lgtd_tests_insert_mock_tag("wave~");
    static struct lgtd_lifx_gateway gw_bulb_2 = {
        .bulbs = LIST_HEAD_INITIALIZER(&gw_bulb_2.bulbs),
        .tag_ids = 0x7
    };
    lgtd_tests_add_tag_to_gw(gw_2_tag_1, &gw_bulb_2, 0);
    lgtd_tests_add_tag_to_gw(gw_2_tag_2, &gw_bulb_2, 1);
    lgtd_tests_add_tag_to_gw(gw_2_tag_3, &gw_bulb_2, 2);
    static struct lgtd_lifx_bulb bulb_2 = {
        .addr = { 5, 4, 3, 2, 1 },
        .state = {
            .hue = 0x0000,
            .saturation = 0x0000,
            .brightness = 0xffff,
            .kelvin = 4000,
            .label = "light",
            .power = LGTD_LIFX_POWER_OFF,
            .tags = 0x3
        },
        .gw = &gw_bulb_2
    };
    static struct lgtd_router_device device_2 = { .device = &bulb_2 };
    SLIST_INSERT_HEAD(&devices, &device_2, link);

    return &devices;
}

static int router_send_to_device_call_count = 0;

void
lgtd_router_send_to_device(struct lgtd_lifx_bulb *bulb,
                           enum lgtd_lifx_packet_type pkt_type,
                           void *pkt)
{
    if (!bulb) {
        errx(1, "lgtd_router_send_to_device called without a device");
    }

    if (pkt_type != LGTD_LIFX_SET_POWER_STATE) {
        errx(
            1, "lgtd_router_send_to_device got packet type %#x (expected %#x)",
            pkt_type, LGTD_LIFX_SET_POWER_STATE
        );
    }

    if (!pkt) {
        errx(1, "lgtd_router_send_to_device called without a packet");
    }

    struct lgtd_lifx_packet_power_state *payload = pkt;

    if (!strcmp(bulb->state.label, "light")) {
        if (payload->power != LGTD_LIFX_POWER_ON) {
            errx(1, "bulb light should be turned off");
        }
    } else if (!strcmp(bulb->state.label, "wave")) {
        if (payload->power != LGTD_LIFX_POWER_OFF) {
            errx(1, "bulb wave should be turned on");
        }
    } else {
        errx(
            1, "lgtd_router_send_to_deviceg got an unknown bulb: %s",
            bulb->state.label
        );
    }

    router_send_to_device_call_count++;
}

int
main(void)
{
    struct lgtd_client *client;
    client = lgtd_tests_insert_mock_client(FAKE_BUFFEREVENT);
    struct lgtd_proto_target_list *targets = (void *)0x2a;

    lgtd_proto_power_toggle(client, targets);

    const char expected[] = "true";

    if (client_write_buf_idx != sizeof(expected) - 1) {
        errx(
            1, "%d bytes written, expected %lu (got %.*s instead of %s)",
            client_write_buf_idx, sizeof(expected) - 1UL,
            client_write_buf_idx, client_write_buf, expected
        );
    }

    if (memcmp(expected, client_write_buf, sizeof(expected) - 1)) {
        errx(
            1, "got %.*s instead of %s",
            client_write_buf_idx, client_write_buf, expected
        );
    }

    if (!device_list_free_called) {
        errx(1, "the list of devices hasn't been freed");
    }

    if (router_send_to_device_call_count != 2) {
        errx(
            1, "lgtd_router_send_to_device called %d times (expected 2)",
            router_send_to_device_call_count
        );
    }

    return 0;
}
