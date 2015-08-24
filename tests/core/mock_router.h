#pragma once

#include "lifx/wire_proto.h"  // enum lgtd_lifx_packet_type

struct lgtd_proto_target_list;
struct lgtd_router_device_list;

#ifndef MOCKED_LGTD_ROUTER_SEND
bool
lgtd_router_send(const struct lgtd_proto_target_list *targets,
                 enum lgtd_lifx_packet_type pkt_type,
                 void *pkt)
{
    (void)targets;
    (void)pkt_type;
    (void)pkt;
    return true;
}
#endif

#ifndef MOCKED_LGTD_ROUTER_SEND_TO_DEVICE
void
lgtd_router_send_to_device(struct lgtd_lifx_bulb *bulb,
                           enum lgtd_lifx_packet_type pkt_type,
                           void *pkt)
{
    (void)bulb;
    (void)pkt_type;
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_ROUTER_SEND_TO_TAG
void
lgtd_router_send_to_tag(const struct lgtd_lifx_tag *tag,
                        enum lgtd_lifx_packet_type pkt_type,
                        void *pkt)
{
    (void)tag;
    (void)pkt_type;
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_ROUTER_SEND_TO_LABEL
void
lgtd_router_send_to_label(const char *label,
                          enum lgtd_lifx_packet_type pkt_type,
                          void *pkt)
{
    (void)label;
    (void)pkt_type;
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_ROUTER_BROADCAST
void
lgtd_router_broadcast(enum lgtd_lifx_packet_type pkt_type, void *pkt)
{
    (void)pkt_type;
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_ROUTER_TARGETS_TO_DEVICES
struct lgtd_router_device_list *
lgtd_router_targets_to_devices(const struct lgtd_proto_target_list *targets)
{
    (void)targets;
    return NULL;
}
#endif

#ifndef MOCKED_LGTD_ROUTER_DEVICE_LIST_FREE
void
lgtd_router_device_list_free(struct lgtd_router_device_list *devices)
{
    (void)devices;
}
#endif
