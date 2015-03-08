#include <sys/queue.h>
#include <sys/tree.h>
#include <sys/socket.h>
#include <assert.h>
#include <endian.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <event2/util.h>

#include "lifx/wire_proto.h"
#include "core/time_monotonic.h"
#include "lifx/bulb.h"
#include "lifx/gateway.h"
#include "tests_utils.h"

struct lgtd_lifx_gateway_list lgtd_lifx_gateways =
    LIST_HEAD_INITIALIZER(&lgtd_lifx_gateways);

struct lgtd_lifx_gateway *
lgtd_tests_insert_mock_gateway(int id)
{
    struct lgtd_lifx_gateway *gw = calloc(1, sizeof(*gw));

    gw->socket = id;
    gw->site[0] = id;

    LIST_INSERT_HEAD(&lgtd_lifx_gateways, gw, link);

    return gw;
}

struct lgtd_lifx_bulb *
lgtd_tests_insert_mock_bulb(struct lgtd_lifx_gateway *gw, uint64_t addr)
{
    assert(gw);

    union {
        uint8_t     as_array[LGTD_LIFX_ADDR_LENGTH];
        uint64_t    as_scalar;
    } bulb_addr = { .as_scalar = htobe64(addr) >> 16 };

    return lgtd_lifx_bulb_open(gw, bulb_addr.as_array);
}
