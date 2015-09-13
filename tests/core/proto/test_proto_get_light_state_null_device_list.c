#include "proto.c"

#include "mock_client_buf.h"
#include "mock_daemon.h"
#include "mock_event2.h"
#include "mock_timer.h"
#include "tests_utils.h"

#define MOCKED_ROUTER_TARGETS_TO_DEVICES
#define MOCKED_CLIENT_SEND_ERROR
#include "tests_proto_utils.h"

static bool send_error_called = false;

struct lgtd_router_device_list *
lgtd_router_targets_to_devices(const struct lgtd_proto_target_list *targets)
{
    if (targets != (void *)0x2a) {
        lgtd_errx(1, "unexpected targets list");
    }

    return NULL;
}

void
lgtd_client_send_error(struct lgtd_client *client,
                       enum lgtd_client_error_code error,
                       const char *msg)
{
    if (!client) {
        lgtd_errx(1, "Expected client");
    }

    if (error != LGTD_CLIENT_INTERNAL_ERROR) {
        lgtd_errx(
            1, "Got error code %d (expected %d)",
            error, LGTD_CLIENT_INTERNAL_ERROR
        );
    }

    if (!msg) {
        lgtd_errx(1, "Expected error message");
    }

    send_error_called = true;
}

int
main(void)
{
    struct lgtd_client *client;
    client = lgtd_tests_insert_mock_client(FAKE_BUFFEREVENT);
    struct lgtd_proto_target_list *targets = (void *)0x2a;

    lgtd_proto_get_light_state(client, targets);

    if (!send_error_called) {
        lgtd_errx(1, "lgtd_client_send_error hasn't been called");
    }

    return 0;
}
