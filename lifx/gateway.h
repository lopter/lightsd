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
enum { LGTD_LIFX_GATEWAY_MIN_REFRESH_INTERVAL_MSECS = 200 };

struct lgtd_lifx_gateway {
    LIST_ENTRY(lgtd_lifx_gateway)   link;
    struct lgtd_lifx_bulb_list      bulbs;
    // Multiple gateways can share the same site (that happens when bulbs are
    // far away enough that ZigBee can't be used). Moreover the SET_PAN_GATEWAY
    // packet doesn't include the device address in the header (i.e: site and
    // device_addr have the same value) so we have no choice but to use the
    // remote ip address to identify a gateway:
    struct sockaddr_storage         peer;
    char                            ip_addr[INET6_ADDRSTRLEN];
    uint16_t                        port;
    uint8_t                         site[LGTD_LIFX_ADDR_LENGTH];
    evutil_socket_t                 socket;
    // Those three timers let us measure the latency of the gateway. If we
    // aren't the only client on the network then this won't be accurate since
    // we will get pushed packets we didn't ask for, but good enough for our
    // purpose of rate limiting our requests to the gateway:
    lgtd_time_mono_t                last_req_at;
    lgtd_time_mono_t                next_req_at;
    lgtd_time_mono_t                last_pkt_at;
    struct event                    *write_ev;
    struct evbuffer                 *write_buf;
    bool                            pending_refresh_req;
    struct event                    *refresh_ev;
};
LIST_HEAD(lgtd_lifx_gateway_list, lgtd_lifx_gateway);

extern struct lgtd_lifx_gateway_list lgtd_lifx_gateways;

struct lgtd_lifx_gateway *lgtd_lifx_gateway_get(const struct sockaddr_storage *);
struct lgtd_lifx_gateway *lgtd_lifx_gateway_open(const struct sockaddr_storage *,
                                                 ev_socklen_t,
                                                 const uint8_t *,
                                                 lgtd_time_mono_t);

void lgtd_lifx_gateway_close(struct lgtd_lifx_gateway *);
void lgtd_lifx_gateway_close_all(void);

void lgtd_lifx_gateway_force_refresh(struct lgtd_lifx_gateway *);

void lgtd_lifx_gateway_send_packet(struct lgtd_lifx_gateway *,
                                   const struct lgtd_lifx_packet_header *,
                                   const void *,
                                   int);

void lgtd_lifx_gateway_handle_pan_gateway(struct lgtd_lifx_gateway *,
                                          const struct lgtd_lifx_packet_header *,
                                          const struct lgtd_lifx_packet_pan_gateway *);
void lgtd_lifx_gateway_handle_light_status(struct lgtd_lifx_gateway *,
                                           const struct lgtd_lifx_packet_header *,
                                           const struct lgtd_lifx_packet_light_status *);
void lgtd_lifx_gateway_handle_power_state(struct lgtd_lifx_gateway *,
                                          const struct lgtd_lifx_packet_header *,
                                          const struct lgtd_lifx_packet_power_state *);
