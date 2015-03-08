#include <sys/queue.h>
#include <sys/tree.h>
#include <sys/socket.h>
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

int lgtd_tests_gw_pkt_queue_size = 0;
struct {
    struct lgtd_lifx_gateway        *gw;
    struct lgtd_lifx_packet_header  *hdr;
    const void                      *pkt;
    int                             pkt_size;
} lgtd_tests_gw_pkt_queue[16] = { { NULL, NULL, NULL, 0}, };

void
lgtd_lifx_gateway_send_packet(struct lgtd_lifx_gateway *gw,
                              const struct lgtd_lifx_packet_header *hdr,
                              const void *pkt,
                              int pkt_size)
{
    lgtd_tests_gw_pkt_queue[lgtd_tests_gw_pkt_queue_size].gw = gw;
    // headers are created on the stack so we need to dup them:
    lgtd_tests_gw_pkt_queue[lgtd_tests_gw_pkt_queue_size].hdr = malloc(
        sizeof(*hdr)
    );
    memcpy(
        lgtd_tests_gw_pkt_queue[lgtd_tests_gw_pkt_queue_size].hdr,
        hdr,
        sizeof(*hdr)
    );
    lgtd_tests_gw_pkt_queue[lgtd_tests_gw_pkt_queue_size].pkt = pkt;
    lgtd_tests_gw_pkt_queue[lgtd_tests_gw_pkt_queue_size].pkt_size = pkt_size;
    lgtd_tests_gw_pkt_queue_size++;
}
