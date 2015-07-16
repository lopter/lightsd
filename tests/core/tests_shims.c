#include <sys/queue.h>
#include <sys/tree.h>
#include <sys/socket.h>
#include <endian.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <event2/event.h>
#include <event2/util.h>

#include "lifx/wire_proto.h"
#include "core/time_monotonic.h"
#include "lifx/bulb.h"
#include "lifx/gateway.h"
#include "lifx/tagging.h"
#include "lightsd.h"

struct lgtd_opts lgtd_opts = {
    .foreground = false,
    .log_timestamps = false,
    .verbosity = LGTD_DEBUG
};

struct event_base *lgtd_ev_base = NULL;

void
lgtd_cleanup(void)
{
}


void lgtd_lifx_gateway_handle_pan_gateway(struct lgtd_lifx_gateway *gw,
                                          const struct lgtd_lifx_packet_header *hdr,
                                          const struct lgtd_lifx_packet_pan_gateway *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}

void lgtd_lifx_gateway_handle_light_status(struct lgtd_lifx_gateway *gw,
                                           const struct lgtd_lifx_packet_header *hdr,
                                           const struct lgtd_lifx_packet_light_status *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}

void lgtd_lifx_gateway_handle_power_state(struct lgtd_lifx_gateway *gw,
                                          const struct lgtd_lifx_packet_header *hdr,
                                          const struct lgtd_lifx_packet_power_state *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}

void lgtd_lifx_gateway_handle_tag_labels(struct lgtd_lifx_gateway *gw,
                                         const struct lgtd_lifx_packet_header *hdr,
                                         const struct lgtd_lifx_packet_tag_labels *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}

struct lgtd_lifx_tag *
lgtd_lifx_tagging_find_tag(const char *tag_label)
{
    struct lgtd_lifx_tag *tag = NULL;
    LIST_FOREACH(tag, &lgtd_lifx_tags, link) {
        if (!strcmp(tag->label, tag_label)) {
            break;
        }
    }
    return tag;
}
