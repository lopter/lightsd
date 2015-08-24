// Copyright (c) 2014, Louis Opter <kalessin@kalessin.fr>
//
// This file is part of lighstd.
//
// lighstd is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// lighstd is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with lighstd.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

// Send GET_LIGHT_STATE to the gateway at most every this interval. FYI,
// according to my own tests, aggressively polling a bulb doesn't raise its
// consumption at all (and it's interesting to note that a turned off bulb
// still draw about 2W in ZigBee and about 3W in WiFi).
enum { LGTD_LIFX_GATEWAY_MIN_REFRESH_INTERVAL_MSECS = 800 };

// You can't send more than one lifx packet per UDP datagram.
enum { LGTD_LIFX_GATEWAY_PACKET_RING_SIZE = 16 };

enum { LGTD_LIFX_GATEWAY_MAX_TAGS = 64 };

struct lgtd_lifx_message {
    enum lgtd_lifx_packet_type  type;
    int                         size;
};

struct lgtd_lifx_gateway {
    LIST_ENTRY(lgtd_lifx_gateway)   link;
    struct lgtd_lifx_bulb_list      bulbs;
#define LGTD_LIFX_GATEWAY_GET_BULB_OR_RETURN(b, gw, bulb_addr)  do {    \
    (b) = lgtd_lifx_gateway_get_or_open_bulb((gw), (bulb_addr));        \
    if (!(b)) {                                                         \
        return;                                                         \
    }                                                                   \
} while (0)
    // Multiple gateways can share the same site (that happens when bulbs are
    // far away enough that ZigBee can't be used). Moreover the SET_PAN_GATEWAY
    // packet doesn't include the device address in the header (i.e: site and
    // device_addr have the same value) so we have no choice but to use the
    // remote ip address to identify a gateway:
    struct sockaddr_storage         peer;
    uint8_t                         addr[LGTD_LIFX_ADDR_LENGTH];
    char                            ip_addr[INET6_ADDRSTRLEN];
    uint16_t                        port;
    // TODO: just use an integer and rename it to site_id:
    union {
        uint8_t                     as_array[LGTD_LIFX_ADDR_LENGTH];
        uint64_t                    as_integer;
    }                               site;
    uint64_t                        tag_ids;
    struct lgtd_lifx_tag            *tags[LGTD_LIFX_GATEWAY_MAX_TAGS];
    uint8_t                         tag_refcounts[LGTD_LIFX_GATEWAY_MAX_TAGS];
    evutil_socket_t                 socket;
    // Those three timestamps let us measure the latency of the gateway. If we
    // aren't the only client on the network then this won't be accurate since
    // we will get pushed packets we didn't ask for, but good enough for our
    // purpose of rate limiting our requests to the gateway:
    lgtd_time_mono_t                last_req_at;
    lgtd_time_mono_t                next_req_at;
    lgtd_time_mono_t                last_pkt_at;
#define LGTD_LIFX_GATEWAY_LATENCY(gw) ((gw)->last_pkt_at - (gw)->last_req_at)
    struct lgtd_lifx_message        pkt_ring[LGTD_LIFX_GATEWAY_PACKET_RING_SIZE];
#define LGTD_LIFX_GATEWAY_INC_MESSAGE_RING_INDEX(idx)  do { \
    (idx) += 1;                                             \
    (idx) %= LGTD_LIFX_GATEWAY_PACKET_RING_SIZE;            \
} while(0)
    int                             pkt_ring_head;
    int                             pkt_ring_tail;
    bool                            pkt_ring_full;
    struct event                    *write_ev;
    struct evbuffer                 *write_buf;
    bool                            pending_refresh_req;
    struct lgtd_timer               *refresh_timer;
};
LIST_HEAD(lgtd_lifx_gateway_list, lgtd_lifx_gateway);

extern struct lgtd_lifx_gateway_list lgtd_lifx_gateways;

#define LGTD_LIFX_GATEWAY_SET_BULB_ATTR(gw, bulb_addr, bulb_fn, ...) do {   \
    struct lgtd_lifx_bulb *b;                                               \
    LGTD_LIFX_GATEWAY_GET_BULB_OR_RETURN(b, gw, bulb_addr);                 \
    (bulb_fn)(b, __VA_ARGS__);                                              \
} while (0)

struct lgtd_lifx_gateway *lgtd_lifx_gateway_get(const struct sockaddr_storage *);
struct lgtd_lifx_gateway *lgtd_lifx_gateway_open(const struct sockaddr_storage *,
                                                 ev_socklen_t,
                                                 const uint8_t *,
                                                 lgtd_time_mono_t);

void lgtd_lifx_gateway_close(struct lgtd_lifx_gateway *);
void lgtd_lifx_gateway_close_all(void);
void lgtd_lifx_gateway_remove_and_close_bulb(struct lgtd_lifx_gateway *, struct lgtd_lifx_bulb *);

void lgtd_lifx_gateway_force_refresh(struct lgtd_lifx_gateway *);

void lgtd_lifx_gateway_enqueue_packet(struct lgtd_lifx_gateway *,
                                      const struct lgtd_lifx_packet_header *,
                                      const struct lgtd_lifx_packet_info *,
                                      void *);
// This could be on router but it's LIFX specific so I'd rather keep it here:
bool lgtd_lifx_gateway_send_to_site(struct lgtd_lifx_gateway *,
                                    enum lgtd_lifx_packet_type,
                                    void *);

void lgtd_lifx_gateway_update_tag_refcounts(struct lgtd_lifx_gateway *, uint64_t, uint64_t);

int lgtd_lifx_gateway_get_tag_id(const struct lgtd_lifx_gateway *, const struct lgtd_lifx_tag *);
int lgtd_lifx_gateway_allocate_tag_id(struct lgtd_lifx_gateway *, int, const char *);
void lgtd_lifx_gateway_deallocate_tag_id(struct lgtd_lifx_gateway *, int);

void lgtd_lifx_gateway_handle_pan_gateway(struct lgtd_lifx_gateway *,
                                          const struct lgtd_lifx_packet_header *,
                                          const struct lgtd_lifx_packet_pan_gateway *);
void lgtd_lifx_gateway_handle_light_status(struct lgtd_lifx_gateway *,
                                           const struct lgtd_lifx_packet_header *,
                                           const struct lgtd_lifx_packet_light_status *);
void lgtd_lifx_gateway_handle_power_state(struct lgtd_lifx_gateway *,
                                          const struct lgtd_lifx_packet_header *,
                                          const struct lgtd_lifx_packet_power_state *);
void lgtd_lifx_gateway_handle_tag_labels(struct lgtd_lifx_gateway *,
                                         const struct lgtd_lifx_packet_header *,
                                         const struct lgtd_lifx_packet_tag_labels *);
void lgtd_lifx_gateway_handle_tags(struct lgtd_lifx_gateway *,
                                   const struct lgtd_lifx_packet_header *,
                                   const struct lgtd_lifx_packet_tags *);
void lgtd_lifx_gateway_handle_ip_state(struct lgtd_lifx_gateway *,
                                        const struct lgtd_lifx_packet_header *,
                                        const struct lgtd_lifx_packet_ip_state *);
void lgtd_lifx_gateway_handle_ip_firmware_info(struct lgtd_lifx_gateway *,
                                               const struct lgtd_lifx_packet_header *,
                                               const struct lgtd_lifx_packet_ip_firmware_info *);
void lgtd_lifx_gateway_handle_product_info(struct lgtd_lifx_gateway *,
                                           const struct lgtd_lifx_packet_header *,
                                           const struct lgtd_lifx_packet_product_info *);
void lgtd_lifx_gateway_handle_runtime_info(struct lgtd_lifx_gateway *,
                                           const struct lgtd_lifx_packet_header *,
                                           const struct lgtd_lifx_packet_runtime_info *);
