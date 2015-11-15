#include "proto.c"

#include "mock_client_buf.h"
#include "mock_daemon.h"
#include "mock_gateway.h"
#include "mock_event2.h"
#include "mock_log.h"
#include "mock_timer.h"
#include "mock_wire_proto.h"
#include "tests_utils.h"

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
    if (SLIST_FIRST(&devices)) {
        return &devices;
    }

    static struct lgtd_lifx_gateway gw_bulb_1 = {
        .bulbs = LIST_HEAD_INITIALIZER(&gw_bulb_1.bulbs),
        .peeraddr = "[::ffff:127.0.0.1]:1"
    };
    static struct lgtd_lifx_bulb bulb_1 = {
        .addr = { 1, 2, 3, 4, 5 },
        .state = {
            .hue = 0xaaaa,
            .saturation = 0xffff,
            .brightness = 0xbbbb,
            .kelvin = 3600,
            .power = LGTD_LIFX_POWER_ON,
            .tags = 0
        },
        .ips[LGTD_LIFX_BULB_WIFI_IP] = {
            .fw_info.version = 0x10001
        },
        .model = "testbulb",
        .product_info = {
            .vendor_id = 1,
            .product_id = 0xa,
            .version = 9
        },
        .gw = &gw_bulb_1
    };
    memset(&bulb_1.state.label, 'a', sizeof(bulb_1.state.label));
    static struct lgtd_router_device device_1 = { .device = &bulb_1 };
    SLIST_INSERT_HEAD(&devices, &device_1, link);

    return &devices;
}

int
main(void)
{
    struct lgtd_client *client;
    client = lgtd_tests_insert_mock_client(FAKE_BUFFEREVENT);
    struct lgtd_proto_target_list *targets = (void *)0x2a;

    lgtd_opts.verbosity = LGTD_INFO;

    char expected_info[] = ("["
        "{"
            "\"_lifx\":{"
                "\"addr\":\"01:02:03:04:05:00\","
                "\"gateway\":{"
                    "\"site\":\"00:00:00:00:00:00\","
                    "\"url\":\"tcp://[::ffff:127.0.0.1]:1\","
                    "\"latency\":0"
                "},"
                "\"mcu\":{\"firmware_version\":\"0.0\"},"
                "\"wifi\":{\"firmware_version\":\"1.1\"}"
            "},"
            "\"_model\":\"testbulb\","
            "\"_vendor\":null,"
            "\"hsbk\":[240,1,0.733333,3600],"
            "\"power\":true,"
            "\"label\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\","
            "\"tags\":[]"
        "}"
    "]");

    reset_client_write_buf();

    lgtd_proto_get_light_state(client, targets);

    if (client_write_buf_idx != sizeof(expected_info) - 1) {
        lgtd_errx(
            1,
            "%d bytes written, expected_info %lu "
            "(got %.*s instead of %s)",
            client_write_buf_idx, sizeof(expected_info) - 1UL,
            client_write_buf_idx, client_write_buf, expected_info
        );
    }

    if (memcmp(expected_info, client_write_buf, sizeof(expected_info) - 1)) {
        lgtd_errx(
            1, "got %.*s instead of %s",
            client_write_buf_idx, client_write_buf, expected_info
        );
    }

    return 0;
}
