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

struct lgtd_lifx_ip_state {
    float       signal_strength;    // mW
    uint32_t    tx_bytes;
    uint32_t    rx_bytes;
    uint16_t    temperature;        // Deci-celcius: e.g 24.3 -> 2430
};

struct lgtd_lifx_ip_firmware_info {
    uint64_t    built_at;       // ns since epoch
    uint64_t    installed_at;   // ns since epoch
    uint32_t    version;
};

struct lgtd_lifx_product_info {
    uint32_t    vendor_id;
    uint32_t    product_id;
    uint32_t    version;
};

struct lgtd_lifx_runtime_info {
    uint64_t    time;       // ns since epoch
    uint64_t    uptime;     // ns
    uint64_t    downtime;   // ns, last power off period duration
};
#pragma pack(pop)

enum { LGTD_LIFX_BULB_FETCH_HARDWARE_INFO_TIMER_MSECS = 5000 };
// non-gateway fw 1.1 bulbs will never send this information, so just bail out
// after a few tries:
enum {
    LGTD_LIFX_BULB_FETCH_WIFI_FW_INFO_TIMEOUT_MSECS =
        LGTD_LIFX_BULB_FETCH_HARDWARE_INFO_TIMER_MSECS * 4
};

enum lgtd_lifx_bulb_ips {
    LGTD_LIFX_BULB_MCU_IP = 0,
    LGTD_LIFX_BULB_WIFI_IP,
    LGTD_LIFX_BULB_IP_COUNT,
};

// keyed with enum lgtd_lifx_bulb_ips:
extern const char * const lgtd_lifx_bulb_ip_names[];

struct lgtd_lifx_bulb_ip {
    struct lgtd_lifx_ip_state           state;
    lgtd_time_mono_t                    state_updated_at;
    struct lgtd_lifx_ip_firmware_info   fw_info;
    lgtd_time_mono_t                    fw_info_updated_at;
};

struct lgtd_lifx_bulb {
    RB_ENTRY(lgtd_lifx_bulb)        link;
    SLIST_ENTRY(lgtd_lifx_bulb)     link_by_gw;
    lgtd_time_mono_t                last_light_state_at;
    lgtd_time_mono_t                runtime_info_updated_at;
    lgtd_time_mono_t                dirty_at;
    uint16_t                        expected_power_on;
    uint8_t                         addr[LGTD_LIFX_ADDR_LENGTH];
    const char                      *model;
    const char                      *vendor;
    struct lgtd_lifx_gateway        *gw;
    struct lgtd_lifx_light_state    state;
    struct lgtd_lifx_bulb_ip        ips[LGTD_LIFX_BULB_IP_COUNT];
    struct lgtd_lifx_product_info   product_info;
    struct lgtd_lifx_runtime_info   runtime_info;
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

bool lgtd_lifx_bulb_has_label(const struct lgtd_lifx_bulb *,
                              const char *);

void lgtd_lifx_bulb_set_light_state(struct lgtd_lifx_bulb *,
                                    const struct lgtd_lifx_light_state *,
                                    lgtd_time_mono_t);
void lgtd_lifx_bulb_set_power_state(struct lgtd_lifx_bulb *, uint16_t);
void lgtd_lifx_bulb_set_tags(struct lgtd_lifx_bulb *, uint64_t);

void lgtd_lifx_bulb_set_ip_state(struct lgtd_lifx_bulb *bulb,
                                 enum lgtd_lifx_bulb_ips ip_id,
                                 const struct lgtd_lifx_ip_state *state,
                                 lgtd_time_mono_t received_at);
void lgtd_lifx_bulb_set_ip_firmware_info(struct lgtd_lifx_bulb *bulb,
                                         enum lgtd_lifx_bulb_ips ip_id,
                                         const struct lgtd_lifx_ip_firmware_info *info,
                                         lgtd_time_mono_t received_at);
void lgtd_lifx_bulb_set_product_info(struct lgtd_lifx_bulb *,
                                     const struct lgtd_lifx_product_info *);
void lgtd_lifx_bulb_set_runtime_info(struct lgtd_lifx_bulb *,
                                     const struct lgtd_lifx_runtime_info *,
                                     lgtd_time_mono_t);
void lgtd_lifx_bulb_set_label(struct lgtd_lifx_bulb *,
                              const char [LGTD_LIFX_LABEL_SIZE]);
