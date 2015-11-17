#include <endian.h>

#include "proto.c"

#include "mock_client_buf.h"
#include "mock_daemon.h"
#include "mock_event2.h"
#include "mock_log.h"
#include "mock_timer.h"
#define MOCKED_LGTD_LIFX_WIRE_ENCODE_WAVEFORM
#include "mock_wire_proto.h"
#include "tests_utils.h"

#define MOCKED_CLIENT_SEND_RESPONSE
#define MOCKED_ROUTER_SEND
#include "tests_proto_utils.h"

int lifx_wire_encode_waveform_call_count = 0;

void
lgtd_lifx_wire_encode_waveform(struct lgtd_lifx_packet_waveform *pkt)
{
    (void)pkt;

    lifx_wire_encode_waveform_call_count++;
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

    if (pkt_type != LGTD_LIFX_SET_WAVEFORM) {
        errx(
            1, "invalid packet type %d (expected %d)",
            pkt_type, LGTD_LIFX_SET_WAVEFORM
        );
    }

    struct lgtd_lifx_packet_waveform *waveform = pkt;
    if (lifx_wire_encode_waveform_call_count != 1) {
        errx(
            1, "lifx_wire_encode_waveform_call_count = %d (expected 1)",
            lifx_wire_encode_waveform_call_count
        );
    }
    if (waveform->waveform != LGTD_LIFX_WAVEFORM_SAW) {
        errx(
            1, "got waveform = %d (expected %d)",
            waveform->waveform, LGTD_LIFX_WAVEFORM_SAW
        );
    }
    if (waveform->hue != 42) {
        errx(1, "got hue = %d (expected 42)", waveform->hue);
    }
    if (waveform->saturation != 10000) {
        errx(1, "got saturation = %d (expected 10000)", waveform->saturation);
    }
    if (waveform->brightness != 20000) {
        errx(1, "got brightness = %d (expected 20000)", waveform->brightness);
    }
    if (waveform->kelvin != 4500) {
        errx(1, "got kelvin = %d (expected 4500)", waveform->kelvin);
    }
    if (waveform->period != 200) {
        errx(1, "got period = %d (expected 200)", waveform->period);
    }
    if (waveform->cycles != 10.) {
        errx(1, "got cycles = %f (expected 10)", waveform->cycles);
    }
    if (waveform->skew_ratio != 0) {
        errx(1, "got skew_ratio = %d (expected 0)", waveform->skew_ratio);
    }

    router_send_call_count++;

    return true;
}

int client_send_response_call_count = 0;

void
lgtd_client_send_response(struct lgtd_client *client, const char *msg)
{
    if (!client) {
        errx(1, "client shouldn't ne NULL");
    }

    if (strcmp(msg, "true")) {
        errx(1, "unexpected response [%s] (expected [true])", msg);
    }

    client_send_response_call_count++;
}

int
main(void)
{
    struct lgtd_proto_target_list *targets;
    targets = lgtd_tests_build_target_list("*", NULL);

    struct lgtd_client client;

    lgtd_proto_set_waveform(
        &client, targets, LGTD_LIFX_WAVEFORM_SAW,
        42, 10000, 20000, 4500, 200, 10., 0, false
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
