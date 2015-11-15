#include "wire_proto.c"

#include "mock_daemon.h"
#include "mock_gateway.h"
#include "mock_log.h"

int
main(void)
{
    enum lgtd_lifx_waveform_type rv = LGTD_LIFX_WAVEFORM_INVALID;

    rv = lgtd_lifx_wire_waveform_string_id_to_type("SAW", 3);
    if (rv != LGTD_LIFX_WAVEFORM_SAW) {
        errx(1, "Expected WAVEFORM_SAW");
    }

    rv = lgtd_lifx_wire_waveform_string_id_to_type("SINE", 4);
    if (rv != LGTD_LIFX_WAVEFORM_SINE) {
        errx(1, "Expected WAVEFORM_SINE");
    }

    rv = lgtd_lifx_wire_waveform_string_id_to_type("HALF_SINE", 9);
    if (rv != LGTD_LIFX_WAVEFORM_HALF_SINE) {
        errx(1, "Expected WAVEFORM_HALF_SINE");
    }

    rv = lgtd_lifx_wire_waveform_string_id_to_type("TRIANGLE", 8);
    if (rv != LGTD_LIFX_WAVEFORM_TRIANGLE) {
        errx(1, "Expected WAVEFORM_TRIANGLE");
    }

    rv = lgtd_lifx_wire_waveform_string_id_to_type("SQUARE", 6);
    if (rv != LGTD_LIFX_WAVEFORM_SQUARE) {
        errx(1, "Expected WAVEFORM_SQUARE");
    }

    rv = lgtd_lifx_wire_waveform_string_id_to_type("TEST", 4);
    if (rv != LGTD_LIFX_WAVEFORM_INVALID) {
        errx(1, "Expected WAVEFORM_INVALID");
    }

    rv = lgtd_lifx_wire_waveform_string_id_to_type("", 0);
    if (rv != LGTD_LIFX_WAVEFORM_INVALID) {
        errx(1, "Expected WAVEFORM_INVALID");
    }

    int cmp = strcmp(
        lgtd_lifx_waveform_table[LGTD_LIFX_WAVEFORM_INVALID].str,
        "INVALID"
    );
    if (cmp) {
        errx(
            1, "Expected INVALID got %s",
            lgtd_lifx_waveform_table[LGTD_LIFX_WAVEFORM_INVALID].str
        );
    }

    return 0;
}
