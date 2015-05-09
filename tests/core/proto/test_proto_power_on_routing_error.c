#include "proto.c"

#include "mock_client_buf.h"
#include "tests_utils.h"

#define MOCKED_CLIENT_SEND_RESPONSE
#define MOCKED_ROUTER_SEND
#include "tests_proto_utils.h"

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

    if (pkt_type != LGTD_LIFX_SET_POWER_STATE) {
        errx(1, "invalid packet type %d (expected %d)",
             pkt_type, LGTD_LIFX_SET_POWER_STATE
        );
    }

    struct lgtd_lifx_packet_power_state *power_on = pkt;
    if (power_on->power != LGTD_LIFX_POWER_ON) {
        errx(1, "invalid power state %hx (expected %x)",
             power_on->power, LGTD_LIFX_POWER_ON);
    }

    return false;
}

void
lgtd_client_send_response(struct lgtd_client *client, const char *msg)
{
    if (!client) {
        errx(1, "client shouldn't ne NULL");
    }

    if (strcmp(msg, "false")) {
        errx(1, "unexpected response [%s] (expected [false])", msg);
    }
}

int
main(void)
{
    struct lgtd_proto_target_list *targets;
    targets = lgtd_tests_build_target_list("*", NULL);

    struct lgtd_client client;

    lgtd_proto_power_on(&client, targets);

    return 0;
}
