#include <string.h>

#include "wire_proto.c"
#include "mock_daemon.h"
#include "mock_gateway.h"
#include "mock_log.h"

int
main(void)
{
    struct lgtd_lifx_packet_light_color pkt = {
        .hue = 42,
        .saturation = 10000,
        .brightness = 20000,
        .kelvin = 4500,
        .transition = 150
    };

    lgtd_lifx_wire_encode_light_color(&pkt);

    int hue = le16toh(pkt.hue);
    int saturation = le16toh(pkt.saturation);
    int brightness = le16toh(pkt.brightness);
    int kelvin = le16toh(pkt.kelvin);
    int transition = htole32(pkt.transition);
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

    return 0;
}
