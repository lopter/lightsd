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

struct lifxd_gateway;

#pragma pack(push, 1)
struct lifxd_light_status {
    uint16_t    hue;
    uint16_t    saturation;
    uint16_t    brightness;
    uint16_t    kelvin;
    uint16_t    dim;
    uint16_t    power;
    char        label[LIFXD_LABEL_SIZE];
    uint64_t    tags;
};
#pragma pack(pop)

struct lifxd_bulb {
    RB_ENTRY(lifxd_bulb)        link;
    SLIST_ENTRY(lifxd_bulb)     link_by_gw;
    struct lifxd_gateway        *gw;
    uint8_t                     addr[LIFXD_ADDR_LENGTH];
    struct lifxd_light_status   status;
};
RB_HEAD(lifxd_bulb_map, lifxd_bulb);
SLIST_HEAD(lifxd_bulb_list, lifxd_bulb);

static inline int
lifxd_bulb_cmp(const struct lifxd_bulb *a, const struct lifxd_bulb *b)
{
    return memcmp(a->addr, b->addr, sizeof(a->addr));
}

struct lifxd_bulb *lifxd_bulb_get(struct lifxd_gateway *, const uint8_t *);
struct lifxd_bulb *lifxd_bulb_open(struct lifxd_gateway *, const uint8_t *);
void lifxd_bulb_close(struct lifxd_bulb *);

void lifxd_bulb_set_light_status(struct lifxd_bulb *, const struct lifxd_light_status *);
void lifxd_bulb_set_power_state(struct lifxd_bulb *, uint16_t);
