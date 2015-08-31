#pragma once

#include "core/time_monotonic.h"
#include "lifx/bulb.h"
#include "lifx/gateway.h"

struct lgtd_lifx_tag;

struct lgtd_lifx_gateway_list lgtd_lifx_gateways =
    LIST_HEAD_INITIALIZER(&lgtd_lifx_gateways);

#ifndef MOCKED_LGTD_LIFX_GATEWAY_LATENCY
lgtd_time_mono_t
lgtd_lifx_gateway_latency(const struct lgtd_lifx_gateway *gw)
{
    (void)gw;
    return 0;
}
#endif

#ifndef MOCKED_LIFX_GATEWAY_SEND_TO_SITE
bool
lgtd_lifx_gateway_send_to_site(struct lgtd_lifx_gateway *gw,
                               enum lgtd_lifx_packet_type pkt_type,
                               void *pkt)
{
    (void)gw;
    (void)pkt_type;
    (void)pkt;
    return false;
}
#endif

#ifndef MOCKED_LIFX_GATEWAY_ALLOCATE_TAG_ID
int
lgtd_lifx_gateway_allocate_tag_id(struct lgtd_lifx_gateway *gw,
                                  int tag_id,
                                  const char *tag_label)
{
    (void)gw;
    (void)tag_id;
    (void)tag_label;
    return -1;
}
#endif

#ifndef MOCKED_LGTD_LIFX_GATEWAY_HANDLE_PAN_GATEWAY
void
lgtd_lifx_gateway_handle_pan_gateway(struct lgtd_lifx_gateway *gw,
                                     const struct lgtd_lifx_packet_header *hdr,
                                     const struct lgtd_lifx_packet_pan_gateway *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_LIFX_GATEWAY_HANDLE_LIGHT_STATUS
void
lgtd_lifx_gateway_handle_light_status(struct lgtd_lifx_gateway *gw,
                                      const struct lgtd_lifx_packet_header *hdr,
                                      const struct lgtd_lifx_packet_light_status *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_LIFX_GATEWAY_HANDLE_POWER_STATE
void
lgtd_lifx_gateway_handle_power_state(struct lgtd_lifx_gateway *gw,
                                     const struct lgtd_lifx_packet_header *hdr,
                                     const struct lgtd_lifx_packet_power_state *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_LIFX_GATEWAY_HANDLE_TAG_LABELS
void
lgtd_lifx_gateway_handle_tag_labels(struct lgtd_lifx_gateway *gw,
                                    const struct lgtd_lifx_packet_header *hdr,
                                    const struct lgtd_lifx_packet_tag_labels *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_LIFX_GATEWAY_HANDLE_TAGS
void
lgtd_lifx_gateway_handle_tags(struct lgtd_lifx_gateway *gw,
                              const struct lgtd_lifx_packet_header *hdr,
                              const struct lgtd_lifx_packet_tags *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_LIFX_GATEWAY_DEALLOCATE_TAG_ID
void
lgtd_lifx_gateway_deallocate_tag_id(struct lgtd_lifx_gateway *gw, int tag_id)
{
    (void)gw;
    (void)tag_id;
}
#endif

#ifndef MOCKED_LGTD_LIFX_GATEWAY_GET_TAG_ID
int
lgtd_lifx_gateway_get_tag_id(const struct lgtd_lifx_gateway *gw,
                             const struct lgtd_lifx_tag *tag)
{
    int tag_id;
    LGTD_LIFX_WIRE_FOREACH_TAG_ID(tag_id, gw->tag_ids) {
        if (gw->tags[tag_id] == tag) {
            return tag_id;
        }
    }

    return -1;
}
#endif

#ifndef MOCKED_LGTD_LIFX_GATEWAY_UPDATE_TAG_REFCOUNTS
void
lgtd_lifx_gateway_update_tag_refcounts(struct lgtd_lifx_gateway *gw,
                                       uint64_t bulb_tags,
                                       uint64_t pkt_tags)
{
    (void)gw;
    (void)bulb_tags;
    (void)pkt_tags;
}
#endif

#ifndef MOCKED_LGTD_LIFX_GATEWAY_HANDLE_IP_STATE
void
lgtd_lifx_gateway_handle_ip_state(struct lgtd_lifx_gateway *gw,
                                  const struct lgtd_lifx_packet_header *hdr,
                                  const struct lgtd_lifx_packet_ip_state *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_LIFX_GATEWAY_HANDLE_IP_FIRMWARE_INFO
void
lgtd_lifx_gateway_handle_ip_firmware_info(struct lgtd_lifx_gateway *gw,
                                          const struct lgtd_lifx_packet_header *hdr,
                                          const struct lgtd_lifx_packet_ip_firmware_info *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_LIFX_GATEWAY_HANDLE_PRODUCT_INFO
void
lgtd_lifx_gateway_handle_product_info(struct lgtd_lifx_gateway *gw,
                                      const struct lgtd_lifx_packet_header *hdr,
                                      const struct lgtd_lifx_packet_product_info *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_LIFX_GATEWAY_HANDLE_RUNTIME_INFO
void
lgtd_lifx_gateway_handle_runtime_info(struct lgtd_lifx_gateway *gw,
                                      const struct lgtd_lifx_packet_header *hdr,
                                      const struct lgtd_lifx_packet_runtime_info *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_LIFX_GATEWAY_HANDLE_BULB_LABEL
void
lgtd_lifx_gateway_handle_bulb_label(struct lgtd_lifx_gateway *gw,
                                    const struct lgtd_lifx_packet_header *hdr,
                                    const struct lgtd_lifx_packet_label *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_LIFX_GATEWAY_HANDLE_AMBIENT_LIGHT
void
lgtd_lifx_gateway_handle_ambient_light(struct lgtd_lifx_gateway *gw,
                                       const struct lgtd_lifx_packet_header *hdr,
                                       const struct lgtd_lifx_packet_ambient_light *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}
#endif
