#include <endian.h>

#include "proto.c"

#include "mock_client_buf.h"
#include "mock_daemon.h"
#include "mock_event2.h"
#include "mock_timer.h"
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

    if (pkt_type != LGTD_LIFX_SET_LIGHT_COLOR) {
        errx(
            1, "invalid packet type %d (expected %d)",
            pkt_type, LGTD_LIFX_SET_LIGHT_COLOR
        );
    }

    struct lgtd_lifx_packet_light_color *light_color = pkt;
    int hue = le16toh(light_color->hue);
    int saturation = le16toh(light_color->saturation);
    int brightness = le16toh(light_color->brightness);
    int kelvin = le16toh(light_color->kelvin);
    int transition = htole32(light_color->transition);

    if (hue != 42) {
        errx(1, "got hue = %d (expected 42)", hue);
    }
    if (saturation != 10000) {
        errx(1, "got saturation = %d (expected 10000)", saturation);
    }
    if (brightness != 20000) {
        errx(1, "got brightness = %d (expected 20000)", brightness);
    }
    if (kelvin != 4500) {
        errx(1, "got kelvin = %d (expected 4500)", kelvin);
    }
    if (transition != 150) {
        errx(1, "got transition = %d (expected 150)", transition);
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

    lgtd_proto_set_light_from_hsbk(
        &client, targets, 42, 10000, 20000, 4500, 150
    );

    return 0;
}
