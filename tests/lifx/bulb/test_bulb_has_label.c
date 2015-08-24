#include "bulb.c"

#include "mock_gateway.h"
#include "mock_router.h"
#include "mock_timer.h"

void
test_label(struct lgtd_lifx_bulb *bulb,
           const char *bulb_label,
           const char *label,
           bool expected)
{
    memcpy(bulb->state.label, bulb_label, LGTD_MIN(
        strlen(bulb_label) + 1, LGTD_LIFX_LABEL_SIZE
    ));
    bool rv = lgtd_lifx_bulb_has_label(bulb, label);
    if (rv != expected) {
        errx(
            1, "bulb_has_label(%s, %s) -> %s (expected %s)",
            bulb_label, label,
            rv ? "true" : "false",
            expected ? "true" : "false"
        );
    }
}

int
main(void)
{
    struct lgtd_lifx_gateway gw;
    uint8_t bulb_addr[LGTD_LIFX_ADDR_LENGTH] = { 5, 4, 3, 2, 1, 0 };
    struct lgtd_lifx_bulb *bulb = lgtd_lifx_bulb_open(&gw, bulb_addr);

    test_label(bulb, "", "test", false);
    test_label(bulb, "test", "test", true);
    test_label(bulb, "testtest", "test", false);
    test_label(bulb, "test", "testtest", false);
    test_label(bulb, "", "", true);
    test_label(
        bulb,
        "testtesttesttesttesttesttesttest",
        "testtesttesttesttesttesttesttesttest",
        true
    );

    return 0;
}
