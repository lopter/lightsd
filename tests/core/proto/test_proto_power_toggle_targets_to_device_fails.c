#include "proto.c"

#include "mock_client_buf.h"
#include "mock_daemon.h"
#include "mock_gateway.h"
#include "tests_utils.h"

#define MOCKED_ROUTER_SEND_TO_DEVICE
#define MOCKED_ROUTER_TARGETS_TO_DEVICES
#define MOCKED_ROUTER_DEVICE_LIST_FREE
#define MOCKED_CLIENT_SEND_ERROR
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

    return NULL;
}

static int router_send_to_device_call_count = 0;

void
lgtd_router_send_to_device(struct lgtd_lifx_bulb *bulb,
                           enum lgtd_lifx_packet_type pkt_type,
                           void *pkt)
{
    (void)bulb;
    (void)pkt_type;
    (void)pkt;

    router_send_to_device_call_count++;
}

static int client_send_error_call_count = 0;

void
lgtd_client_send_error(struct lgtd_client *client,
                       enum lgtd_client_error_code error,
                       const char *msg)
{
    (void)client;
    (void)error;
    (void)msg;

    client_send_error_call_count++;
}

int
main(void)
{
    struct lgtd_client client = { .io = FAKE_BUFFEREVENT };
    struct lgtd_proto_target_list *targets = (void *)0x2a;

    lgtd_proto_power_toggle(&client, targets);

    if (client_send_error_call_count != 1) {
        errx(1, "lgtd_client_send_error called %d times (expected 1)",
            client_send_error_call_count
        );
    }

    if (router_send_to_device_call_count) {
        errx(
            1, "lgtd_router_send_to_device called %d times (expected 0)",
            router_send_to_device_call_count
        );
    }

    return 0;
}
