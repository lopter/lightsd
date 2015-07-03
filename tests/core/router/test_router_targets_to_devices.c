#include "router.c"

#include "tests_utils.h"
#include "tests_router_utils.h"

static int
count_device(const struct lgtd_router_device_list *devices, const void *device)
{
    if (!devices) {
        lgtd_errx(1, "unexpected NULL devices list");
    }

    int count = 0;
    struct lgtd_router_device *it;
    SLIST_FOREACH(it, devices, link) {
        if (it->device == device) {
            count++;
        }
    }

    return count;
}

static int
len(const struct lgtd_router_device_list *devices)
{
    if (!devices) {
        lgtd_errx(1, "unexpected NULL devices list");
    }

    int count = 0;
    struct lgtd_router_device *it;
    SLIST_FOREACH(it, devices, link) {
        count++;
    }

    return count;
}

int
main(void)
{
    lgtd_lifx_wire_load_packet_infos_map();

    struct lgtd_lifx_gateway *gw_1 = lgtd_tests_insert_mock_gateway(1);
    struct lgtd_lifx_gateway *gw_2 = lgtd_tests_insert_mock_gateway(2);

    struct lgtd_lifx_tag *tag_foo = lgtd_tests_insert_mock_tag("foo");
    lgtd_tests_add_tag_to_gw(tag_foo, gw_1, 42);
    lgtd_tests_add_tag_to_gw(tag_foo, gw_2, 63);

    struct lgtd_lifx_tag *tag_bar = lgtd_tests_insert_mock_tag("bar");
    lgtd_tests_add_tag_to_gw(tag_bar, gw_2, 42);

    struct lgtd_lifx_bulb *bulb_1_gw_1 = lgtd_tests_insert_mock_bulb(gw_1, 3);
    bulb_1_gw_1->state.tags = LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(42);

    struct lgtd_lifx_bulb *bulb_2_gw_1 = lgtd_tests_insert_mock_bulb(gw_1, 4);

    struct lgtd_lifx_bulb *bulb_1_gw_2 = lgtd_tests_insert_mock_bulb(gw_2, 5);
    strcpy(bulb_1_gw_2->state.label, "desk");

    struct lgtd_lifx_bulb *bulb_2_gw_2 = lgtd_tests_insert_mock_bulb(gw_2, 6);
    bulb_2_gw_2->state.tags =
        LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(63) | LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(42);

    struct lgtd_proto_target_list *targets;
    struct lgtd_router_device_list *devices;
    int count;

    targets = lgtd_tests_build_target_list(NULL);
    devices = lgtd_router_targets_to_devices(targets);
    if ((count = len(devices)) != 0) {
        lgtd_errx(1, "expected 0 devices but got %d", count);
    }

    targets = lgtd_tests_build_target_list("#pouet", NULL);
    devices = lgtd_router_targets_to_devices(targets);
    if ((count = len(devices))) {
        lgtd_errx(1, "expected 0 device but got %d", count);
    }

    targets = lgtd_tests_build_target_list("#pouet", "label", NULL);
    devices = lgtd_router_targets_to_devices(targets);
    if ((count = len(devices))) {
        lgtd_errx(1, "expected 0 device but got %d", count);
    }

    targets = lgtd_tests_build_target_list("#foo", NULL);
    devices = lgtd_router_targets_to_devices(targets);
    if ((count = count_device(devices, bulb_1_gw_1)) != 1) {
        lgtd_errx(1, "bulb bulb_1_gw_1 found %d times, expected 1", count);
    }
    if ((count = count_device(devices, bulb_2_gw_2)) != 1) {
        lgtd_errx(1, "bulb bulb_2_gw_2 found %d times, expected 1", count);
    }
    if ((count = len(devices)) != 2) {
        lgtd_errx(1, "expected 2 devices but got %d", count);
    }

    targets = lgtd_tests_build_target_list("#bar", NULL);
    devices = lgtd_router_targets_to_devices(targets);
    if ((count = count_device(devices, bulb_2_gw_2)) != 1) {
        lgtd_errx(1, "bulb bulb_2_gw_2 found %d times, expected 1", count);
    }
    if ((count = len(devices)) != 1) {
        lgtd_errx(1, "expected 1 devices but got %d", count);
    }

    targets = lgtd_tests_build_target_list("desk", NULL);
    devices = lgtd_router_targets_to_devices(targets);
    if ((count = count_device(devices, bulb_1_gw_2)) != 1) {
        lgtd_errx(1, "bulb bulb_1_gw_2 found %d times, expected 1", count);
    }
    if ((count = len(devices)) != 1) {
        lgtd_errx(1, "expected 1 device but got %d", count);
    }

    targets = lgtd_tests_build_target_list("4", NULL);
    devices = lgtd_router_targets_to_devices(targets);
    if ((count = count_device(devices, bulb_2_gw_1)) != 1) {
        lgtd_errx(1, "bulb bulb_2_gw_1 found %d times, expected 1", count);
    }
    if ((count = len(devices)) != 1) {
        lgtd_errx(1, "expected 1 device but got %d", count);
    }

    targets = lgtd_tests_build_target_list("desk", "5", NULL);
    devices = lgtd_router_targets_to_devices(targets);
    if ((count = count_device(devices, bulb_1_gw_2)) != 1) {
        lgtd_errx(1, "bulb bulb_1_gw_2 found %d times, expected 1", count);
    }
    if ((count = len(devices)) != 1) {
        lgtd_errx(1, "expected 1 device but got %d", count);
    }

    targets = lgtd_tests_build_target_list("desk", "5", "#foo", NULL);
    devices = lgtd_router_targets_to_devices(targets);
    if ((count = count_device(devices, bulb_1_gw_1)) != 1) {
        lgtd_errx(1, "bulb bulb_1_gw_1 found %d times, expected 1", count);
    }
    if ((count = count_device(devices, bulb_1_gw_2)) != 1) {
        lgtd_errx(1, "bulb bulb_1_gw_2 found %d times, expected 1", count);
    }
    if ((count = count_device(devices, bulb_2_gw_2)) != 1) {
        lgtd_errx(1, "bulb bulb_2_gw_2 found %d times, expected 1", count);
    }
    if ((count = len(devices)) != 3) {
        lgtd_errx(1, "expected 3 device but got %d", count);
    }

    targets = lgtd_tests_build_target_list("*", "#foo", "*", NULL);
    devices = lgtd_router_targets_to_devices(targets);
    if ((count = count_device(devices, bulb_1_gw_1)) != 1) {
        lgtd_errx(1, "bulb bulb_1_gw_1 found %d times, expected 1", count);
    }
    if ((count = count_device(devices, bulb_1_gw_2)) != 1) {
        lgtd_errx(1, "bulb bulb_1_gw_2 found %d times, expected 1", count);
    }
    if ((count = count_device(devices, bulb_1_gw_2)) != 1) {
        lgtd_errx(1, "bulb bulb_1_gw_2 found %d times, expected 1", count);
    }
    if ((count = count_device(devices, bulb_2_gw_2)) != 1) {
        lgtd_errx(1, "bulb bulb_2_gw_2 found %d times, expected 1", count);
    }
    if ((count = len(devices)) != 4) {
        lgtd_errx(1, "expected 4 device but got %d", count);
    }

    return 0;
}
