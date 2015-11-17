#include <endian.h>

#include "proto.c"

#include "mock_client_buf.h"
#include "mock_daemon.h"
#include "mock_event2.h"
#include "mock_log.h"
#include "mock_timer.h"
#define MOCKED_LGTD_LIFX_WIRE_ENCODE_LIGHT_COLOR
#include "mock_wire_proto.h"
#include "tests_utils.h"

#define MOCKED_CLIENT_SEND_RESPONSE
#define MOCKED_ROUTER_SEND
#include "tests_proto_utils.h"

int lifx_wire_encode_light_color_call_count = 0;

void
lgtd_lifx_wire_encode_light_color(struct lgtd_lifx_packet_light_color *pkt)
{
    (void)pkt;

    lifx_wire_encode_light_color_call_count++;
}

int router_send_call_count = 0;

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
    if (lifx_wire_encode_light_color_call_count != 1) {
        errx(
            1, "lifx_wire_encode_light_color_call_count = %d (expected 1)",
            lifx_wire_encode_light_color_call_count
        );
    }
    if (light_color->hue != 42) {
        errx(1, "got hue = %d (expected 42)", light_color->hue);
    }
    if (light_color->saturation != 10000) {
        errx(1, "got saturation = %d (expected 10000)", light_color->saturation);
    }
    if (light_color->brightness != 20000) {
        errx(1, "got brightness = %d (expected 20000)", light_color->brightness);
    }
    if (light_color->kelvin != 4500) {
        errx(1, "got kelvin = %d (expected 4500)", light_color->kelvin);
    }
    if (light_color->transition != 150) {
        errx(1, "got transition = %d (expected 150)", light_color->transition);
    }

    router_send_call_count++;

    return false;
}

int client_send_response_call_count = 0;

void
lgtd_client_send_response(struct lgtd_client *client, const char *msg)
{
    if (!client) {
        errx(1, "client shouldn't ne NULL");
    }

    if (strcmp(msg, "false")) {
        errx(1, "unexpected response [%s] (expected [false])", msg);
    }

    client_send_response_call_count++;
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

    if (router_send_call_count != 1) {
        errx(
            1, "router_send_call_count = %d (expected 1)",
            router_send_call_count
        );
    }
    if (client_send_response_call_count != 1) {
        errx(
            1, "client_send_response_call_count = %d (expected 1)",
            client_send_response_call_count
        );
    }

    return 0;
}
