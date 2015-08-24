#include "proto.c"

#include "mock_client_buf.h"
#include "mock_daemon.h"
#include "mock_event2.h"
#include "mock_timer.h"
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

    return &devices;
}

int
main(void)
{
    struct lgtd_client client = { .io = FAKE_BUFFEREVENT };
    struct lgtd_proto_target_list *targets = (void *)0x2a;

    lgtd_proto_get_light_state(&client, targets);

    const char expected[] = "[]";

    if (client_write_buf_idx != sizeof(expected) - 1) {
        lgtd_errx(
            1, "%d bytes written, expected %lu",
            client_write_buf_idx, sizeof(expected) - 1UL
        );
    }

    if (memcmp(expected, client_write_buf, sizeof(expected) - 1)) {
        lgtd_errx(
            1, "got %.*s instead of %s",
            client_write_buf_idx, client_write_buf, expected
        );
    }

    return 0;
}
