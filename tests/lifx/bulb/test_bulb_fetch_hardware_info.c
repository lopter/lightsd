#include "bulb.c"

#include "mock_gateway.h"
#include "mock_log.h"
#define MOCKED_LGTD_ROUTER_SEND_TO_DEVICE
#include "mock_router.h"
#define MOCKED_LGTD_TIMER_STOP
#include "mock_timer.h"

#include "tests_utils.h"

#define FAKE_TIMER (void *)4

static struct lgtd_lifx_bulb *test_bulb = NULL;

static int timer_stop_call_count = 0;

void
lgtd_timer_stop(struct lgtd_timer *timer)
{
    if (timer != FAKE_TIMER) {
        errx(1, "got timer %p (expected %p)", timer, FAKE_TIMER);
    }

    timer_stop_call_count++;
}

static int get_version_sent = 0;
static int get_mesh_firmware_state_sent = 0;
static int get_wifi_firmware_state_sent = 0;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch" // we don't test the whole enum
void
lgtd_router_send_to_device(struct lgtd_lifx_bulb *bulb,
                           enum lgtd_lifx_packet_type pkt_type,
                           void *pkt)
{
    if (bulb != test_bulb) {
        errx(
            1, "router_send_to_device got bulb %p (expected %p)",
            bulb, test_bulb
        );
    }

    if (pkt) {
        errx(1, "got unexpected pkt");
    }

    switch (pkt_type) {
    case LGTD_LIFX_GET_VERSION:
        get_version_sent++;
        break;
    case LGTD_LIFX_GET_MESH_FIRMWARE:
        get_mesh_firmware_state_sent++;
        break;
    case LGTD_LIFX_GET_WIFI_FIRMWARE_STATE:
        get_wifi_firmware_state_sent++;
        break;
    }
}
#pragma GCC diagnostic pop

static void
test_counters(int get_version, int mcu_fw_info, int wifi_fw_info)
{
    if (get_version_sent != get_version) {
        errx(
            1, "get_version_sent %d (expected %d)",
            get_version_sent, get_version
        );
    }
    if (get_mesh_firmware_state_sent != mcu_fw_info) {
        errx(
            1, "get_mesh_firmware_state_sent %d (expected %d)",
            get_mesh_firmware_state_sent, mcu_fw_info
        );
    }
    if (get_wifi_firmware_state_sent != wifi_fw_info) {
        errx(
            1, "get_wifi_firmware_state_sent %d (expected %d)",
            get_wifi_firmware_state_sent, wifi_fw_info
        );
    }
}

int
main(void)
{
    struct lgtd_lifx_gateway gw;
    memset(&gw, 0, sizeof(gw));
    test_bulb = lgtd_tests_insert_mock_bulb(&gw, 75);
    union lgtd_timer_ctx ctx = { .as_uint = 1 };

    // make sure it handles non-existent bulbs:
    lgtd_lifx_bulb_fetch_hardware_info(FAKE_TIMER, ctx);
    if (timer_stop_call_count != 1) {
        errx(1, "timer_stop wasn't called");
    }

    ctx.as_uint = 0;
    memcpy(&ctx.as_uint, test_bulb->addr, LGTD_LIFX_ADDR_LENGTH);
    lgtd_lifx_bulb_fetch_hardware_info(FAKE_TIMER, ctx);
    test_counters(1, 1, 0);

    memset(&test_bulb->product_info, 1, sizeof(test_bulb->product_info));
    lgtd_lifx_bulb_fetch_hardware_info(FAKE_TIMER, ctx);
    test_counters(1, 2, 0);
    struct lgtd_lifx_bulb_ip *ip = &test_bulb->ips[LGTD_LIFX_BULB_MCU_IP];
    memset(ip, 1, sizeof(*ip));

    // the retry logic for the wifi firmware is a bit more complex because of
    // that v1.1 bug for non-gateway bulbs:

    // state_updated_at for the mcu ip is at zero so we give up and stop the
    // timer instead of sending a new packet:
    ip->state_updated_at = 0;
    lgtd_lifx_bulb_fetch_hardware_info(FAKE_TIMER, ctx);
    test_counters(1, 2, 0);
    if (timer_stop_call_count != 2) {
        errx(1, "timer_stop wasn't called");
    }

    // set it to the current time, the packet should be sent again:
    ip = &test_bulb->ips[LGTD_LIFX_BULB_MCU_IP];
    ip->state_updated_at = lgtd_time_monotonic_msecs();
    lgtd_lifx_bulb_fetch_hardware_info(FAKE_TIMER, ctx);
    test_counters(1, 2, 1);
    if (timer_stop_call_count != 2) {
        errx(1, "timer_stop shouldn't have been called");
    }

    // finally make sure we stop the timer if we got the info alright as it
    // should just be without the bug:
    ip = &test_bulb->ips[LGTD_LIFX_BULB_WIFI_IP];
    memset(ip, 1, sizeof(*ip));
    lgtd_lifx_bulb_fetch_hardware_info(FAKE_TIMER, ctx);
    test_counters(1, 2, 1);
    if (timer_stop_call_count != 3) {
        errx(1, "timer_stop wasn't called");
    }

    return 0;
}
