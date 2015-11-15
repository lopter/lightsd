#include "proto.c"

#include "mock_client_buf.h"
#include "mock_daemon.h"
#include "mock_event2.h"
#include "mock_log.h"
#include "mock_timer.h"
#include "mock_wire_proto.h"
#include "tests_utils.h"

#define MOCKED_CLIENT_SEND_RESPONSE
#define MOCKED_ROUTER_SEND
#include "tests_proto_utils.h"

static const char *test_label = "test of a very long label over 32 chars";

static int router_send_call_count = 0;

bool
lgtd_router_send(const struct lgtd_proto_target_list *targets,
                 enum lgtd_lifx_packet_type pkt_type,
                 void *pkt)
{
    if (strcmp(SLIST_FIRST(targets)->target, "*")) {
        errx(
            1, "invalid target [%s] (expected=[*])",
            SLIST_FIRST(targets)->target
        );
    }

    if (pkt_type != LGTD_LIFX_SET_BULB_LABEL) {
        errx(1, "invalid packet type %d (expected %d)",
             pkt_type, LGTD_LIFX_SET_BULB_LABEL
        );
    }

    struct lgtd_lifx_packet_label *set_label = pkt;
    if (memcmp(set_label->label, test_label, sizeof(set_label->label))) {
        errx(
            1, "invalid packet label %2$.*1$s (expected %3$.*1$s)",
            LGTD_LIFX_LABEL_SIZE, set_label->label, test_label
        );
    }

    if (router_send_call_count++) {
        errx(1, "lgtd_router_send should be called once");
    }

    return true;
}

static int client_send_response_call_count = 0;

void
lgtd_client_send_response(struct lgtd_client *client, const char *msg)
{
    if (!client) {
        errx(1, "client shouldn't be NULL");
    }

    if (strcmp(msg, "true")) {
        errx(1, "unexpected response [%s] (expected [true])", msg);
    }

    if (client_send_response_call_count++) {
        errx(1, "lgtd_client_send_response should be called once");
    }
}

int
main(void)
{
    struct lgtd_proto_target_list *targets;
    targets = lgtd_tests_build_target_list("*", NULL);

    struct lgtd_client *client;
    client = lgtd_tests_insert_mock_client(FAKE_BUFFEREVENT);

    lgtd_proto_set_label(client, targets, test_label);

    if (!router_send_call_count) {
        errx(1, "lgtd_router_send wasn't called");
    }

    if (!client_send_response_call_count) {
        errx(1, "lgtd_client_send_response wasn't called");
    }

    return 0;
}
