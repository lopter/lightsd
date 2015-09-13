#include "proto.c"

#include "mock_client_buf.h"
#include "mock_daemon.h"
#include "mock_gateway.h"
#include "mock_event2.h"
#include "mock_timer.h"
#include "tests_utils.h"

#define MOCKED_ROUTER_TARGETS_TO_DEVICES
#define MOCKED_ROUTER_SEND_TO_DEVICE
#define MOCKED_ROUTER_DEVICE_LIST_FREE
#include "tests_proto_utils.h"

static bool device_list_free_called = false;

void
lgtd_router_device_list_free(struct lgtd_router_device_list *devices)
{
    (void)devices;

    device_list_free_called = true;
}

static bool targets_to_devices_called = false;

struct lgtd_router_device_list *
lgtd_router_targets_to_devices(const struct lgtd_proto_target_list *targets)
{
    (void)targets;

    targets_to_devices_called = true;

    static struct lgtd_router_device_list devices =
        SLIST_HEAD_INITIALIZER(&devices);

    return &devices;
}

static bool send_to_device_called = false;

void
lgtd_router_send_to_device(struct lgtd_lifx_bulb *bulb,
                           enum lgtd_lifx_packet_type pkt_type,
                           void *pkt)
{
    (void)bulb;
    (void)pkt_type;
    (void)pkt;
    send_to_device_called = true;
}

int
main(void)
{
    struct lgtd_client *client;
    client = lgtd_tests_insert_mock_client(FAKE_BUFFEREVENT);

    struct lgtd_proto_target_list *targets;
    targets = lgtd_tests_build_target_list("*", NULL);

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

    if (targets_to_devices_called) {
        lgtd_errx(1, "unexpected call to targets_to_devices");
    }
    if (device_list_free_called) {
        lgtd_errx(1, "nothing should have been freed");
    }
    if (send_to_device_called) {
        lgtd_errx(1, "nothing should have been sent to any device");
    }

    return 0;
}
