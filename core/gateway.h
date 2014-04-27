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

struct lifxd_gateway {
    LIST_ENTRY(lifxd_gateway)   link;
    char                        *hostname;
    uint16_t                    port;
    struct bufferevent          *io;
    uint8_t                     addr[LIFXD_ADDR_LENGTH];
    struct lifxd_packet_header  cur_hdr;
    unsigned                    cur_hdr_offset;
    void                        *cur_pkt;
    unsigned                    cur_pkt_offset;
    unsigned                    cur_pkt_size;
};
LIST_HEAD(lifxd_gateway_list, lifxd_gateway);

struct lifxd_packet_infos {
    RB_ENTRY(lifxd_packet_infos)    link;
    const char                      *name;
    enum lifxd_packet_type          type;
    unsigned                        size;
    void                            (*decode)(void *);
    void                            (*encode)(void *);
    void                            (*handle)(struct lifxd_gateway *,
                                              const struct lifxd_packet_header *,
                                              const void *);
};
RB_HEAD(lifxd_packet_infos_map, lifxd_packet_infos);

static inline int
lifxd_packet_infos_cmp(struct lifxd_packet_infos *a,
                       struct lifxd_packet_infos *b)
{
    return a->type - b->type;
}

void lifxd_gateway_load_packet_infos_map(void);
const struct lifxd_packet_infos *lifxd_gateway_get_packet_infos(enum lifxd_packet_type);

struct lifxd_gateway *lifxd_gateway_get(const uint8_t *);
struct lifxd_gateway *lifxd_gateway_open(const char *,
                                         uint16_t,
                                         const uint8_t *);
void lifxd_gateway_close_all(void);

void lifxd_gateway_get_pan_gateway(struct lifxd_gateway *);
void lifxd_gateway_handle_pan_gateway(struct lifxd_gateway *,
                                      const struct lifxd_packet_header *,
                                      const struct lifxd_packet_pan_gateway *);
void lifxd_gateway_get_light_status(struct lifxd_gateway *);
void lifxd_gateway_handle_light_status(struct lifxd_gateway *,
                                       const struct lifxd_packet_header *,
                                       const struct lifxd_packet_light_status *);

void lifxd_gateway_handle_power_state(struct lifxd_gateway *,
                                      const struct lifxd_packet_header *,
                                      const struct lifxd_packet_power_state *);
