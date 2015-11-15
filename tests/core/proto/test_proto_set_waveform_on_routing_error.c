#include <endian.h>

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
    enum lgtd_lifx_waveform_type waveform_type = waveform->waveform;
    int hue = le16toh(waveform->hue);
    int saturation = le16toh(waveform->saturation);
    int brightness = le16toh(waveform->brightness);
    int kelvin = le16toh(waveform->kelvin);
    int period = le32toh(waveform->period);
    float cycles = lgtd_lifx_wire_lefloattoh(waveform->cycles);
    int skew_ratio = le16toh(waveform->skew_ratio);

    if (waveform_type != LGTD_LIFX_WAVEFORM_SAW) {
        errx(
            1, "got waveform = %d (expected %d)",
            waveform_type, LGTD_LIFX_WAVEFORM_SAW
        );
    }
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
    if (period != 200) {
        errx(1, "got period = %d (expected 200)", period);
    }
    if (cycles != 10.) {
        errx(1, "got cycles = %f (expected 10)", cycles);
    }
    if (skew_ratio != 0) {
        errx(1, "got skew_ratio = %d (expected 0)", skew_ratio);
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

    lgtd_proto_set_waveform(
        &client, targets, LGTD_LIFX_WAVEFORM_SAW,
        42, 10000, 20000, 4500, 200, 10., 0, false
    );

    return 0;
}
