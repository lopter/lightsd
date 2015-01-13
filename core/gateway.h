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
enum { LIFXD_GATEWAY_MIN_REFRESH_INTERVAL_MSECS = 200 };

struct lifxd_gateway {
    LIST_ENTRY(lifxd_gateway)   link;
    struct lifxd_bulb_list      bulbs;
    // Multiple gateways can share the same site (that happens when bulbs are
    // far away enough that ZigBee can't be used). Moreover the SET_PAN_GATEWAY
    // packet doesn't include the device address in the header (i.e: site and
    // device_addr have the same value) so we have no choice but to use the
    // remote ip address to identify a gateway:
    struct sockaddr_storage     peer;
    char                        ip_addr[INET6_ADDRSTRLEN];
    uint16_t                    port;
    uint8_t                     site[LIFXD_ADDR_LENGTH];
    evutil_socket_t             socket;
    // Those three timers let us measure the latency of the gateway. If we
    // aren't the only client on the network then this won't be accurate since
    // we will get pushed packets we didn't ask for, but good enough for our
    // purpose of rate limiting our requests to the gateway:
    lifxd_time_mono_t           last_req_at;
    lifxd_time_mono_t           next_req_at;
    lifxd_time_mono_t           last_pkt_at;
    struct event                *write_ev;
    struct evbuffer             *write_buf;
    struct event                *refresh_ev;
};
LIST_HEAD(lifxd_gateway_list, lifxd_gateway);

extern struct lifxd_gateway_list lifxd_gateways;

struct lifxd_gateway *lifxd_gateway_get(const struct sockaddr_storage *);
struct lifxd_gateway *lifxd_gateway_open(const struct sockaddr_storage *,
                                         const uint8_t *,
                                         lifxd_time_mono_t);

void lifxd_gateway_close(struct lifxd_gateway *);
void lifxd_gateway_close_all(void);

void lifxd_gateway_send_packet(struct lifxd_gateway *,
                               const struct lifxd_packet_header *,
                               const void *,
                               int);

void lifxd_gateway_handle_pan_gateway(struct lifxd_gateway *,
                                      const struct lifxd_packet_header *,
                                      const struct lifxd_packet_pan_gateway *);
void lifxd_gateway_handle_light_status(struct lifxd_gateway *,
                                       const struct lifxd_packet_header *,
                                       const struct lifxd_packet_light_status *);
void lifxd_gateway_handle_power_state(struct lifxd_gateway *,
                                      const struct lifxd_packet_header *,
                                      const struct lifxd_packet_power_state *);
