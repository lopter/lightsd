#include <string.h>

#include "wire_proto.c"
#include "mock_daemon.h"
#include "mock_gateway.h"
#include "mock_log.h"

int
main(void)
{
    struct lgtd_lifx_packet_waveform pkt = {
        .hue = 42,
        .saturation = 10000,
        .brightness = 20000,
        .kelvin = 4500,
        .period = 200,
        .cycles = 10.,
        .skew_ratio = 5000
    };

    lgtd_lifx_wire_encode_waveform(&pkt);

    int hue = le16toh(pkt.hue);
    int saturation = le16toh(pkt.saturation);
    int brightness = le16toh(pkt.brightness);
    int kelvin = le16toh(pkt.kelvin);
    int period = le32toh(pkt.period);
    float cycles = lgtd_lifx_wire_lefloattoh(pkt.cycles);
    int skew_ratio = le16toh(pkt.skew_ratio);
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
    if (skew_ratio != 5000) {
        errx(1, "got skew_ratio = %d (expected 5000)", skew_ratio);
    }

    return 0;
}
