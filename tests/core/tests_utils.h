#pragma once

struct lgtd_lifx_gateway *lgtd_tests_insert_mock_gateway(int);
struct lgtd_lifx_bulb *lgtd_tests_insert_mock_bulb(struct lgtd_lifx_gateway *, uint64_t);
