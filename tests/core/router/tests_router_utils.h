#pragma once

#include "mock_gateway.h"

int lgtd_tests_gw_pkt_queue_size = 0;
struct {
    struct lgtd_lifx_gateway        *gw;
    struct lgtd_lifx_packet_header  *hdr;
    const void                      *pkt;
    int                             pkt_size;
} lgtd_tests_gw_pkt_queue[16] = { { NULL, NULL, NULL, 0}, };

void
lgtd_lifx_gateway_enqueue_packet(struct lgtd_lifx_gateway *gw,
                                 const struct lgtd_lifx_packet_header *hdr,
                                 const struct lgtd_lifx_packet_info *pkt_info,
                                 void *pkt)
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
    lgtd_tests_gw_pkt_queue[lgtd_tests_gw_pkt_queue_size].pkt_size = pkt_info->size;
    lgtd_tests_gw_pkt_queue_size++;
}

void
lgtd_tests_router_reset_pkt_queue(void)
{
    memset(lgtd_tests_gw_pkt_queue, 0, sizeof(lgtd_tests_gw_pkt_queue));
    lgtd_tests_gw_pkt_queue_size = 0;
}
