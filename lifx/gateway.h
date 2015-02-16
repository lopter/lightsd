// Copyright (c) 2014, Louis Opter <kalessin@kalessin.fr>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

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
