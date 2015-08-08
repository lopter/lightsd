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

struct lgtd_lifx_gateway;

#pragma pack(push, 1)
struct lgtd_lifx_light_state {
    uint16_t    hue;
    uint16_t    saturation;
    uint16_t    brightness;
    uint16_t    kelvin;
    uint16_t    dim;
    uint16_t    power;
    char        label[LGTD_LIFX_LABEL_SIZE];
    uint64_t    tags;
};
#pragma pack(pop)

struct lgtd_lifx_bulb {
    RB_ENTRY(lgtd_lifx_bulb)        link;
    SLIST_ENTRY(lgtd_lifx_bulb)     link_by_gw;
    struct lgtd_lifx_gateway        *gw;
    uint8_t                         addr[LGTD_LIFX_ADDR_LENGTH];
    struct lgtd_lifx_light_state    state;
    lgtd_time_mono_t                last_light_state_at;
    lgtd_time_mono_t                dirty_at;
    uint16_t                        expected_power_on;
};
RB_HEAD(lgtd_lifx_bulb_map, lgtd_lifx_bulb);
SLIST_HEAD(lgtd_lifx_bulb_list, lgtd_lifx_bulb);

extern struct lgtd_lifx_bulb_map lgtd_lifx_bulbs_table;

static inline int
lgtd_lifx_bulb_cmp(const struct lgtd_lifx_bulb *a, const struct lgtd_lifx_bulb *b)
{
    return memcmp(a->addr, b->addr, sizeof(a->addr));
}

RB_GENERATE_STATIC(
    lgtd_lifx_bulb_map,
    lgtd_lifx_bulb,
    link,
    lgtd_lifx_bulb_cmp
);

struct lgtd_lifx_bulb *lgtd_lifx_bulb_get(const uint8_t *);
struct lgtd_lifx_bulb *lgtd_lifx_bulb_open(struct lgtd_lifx_gateway *, const uint8_t *);
void lgtd_lifx_bulb_close(struct lgtd_lifx_bulb *);

void lgtd_lifx_bulb_set_light_state(struct lgtd_lifx_bulb *,
                                    const struct lgtd_lifx_light_state *,
                                    lgtd_time_mono_t);
void lgtd_lifx_bulb_set_power_state(struct lgtd_lifx_bulb *, uint16_t);
void lgtd_lifx_bulb_set_tags(struct lgtd_lifx_bulb *, uint64_t);
