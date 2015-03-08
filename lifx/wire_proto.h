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

typedef uint16_t uint16le_t;
typedef uint16_t uint16be_t;
typedef uint32_t uint32le_t;
typedef uint32_t uint32be_t;
typedef uint64_t uint64le_t;
typedef uint64_t uint64be_t;

enum { LGTD_LIFX_PROTOCOL_PORT = 56700 };

enum { LGTD_LIFX_ADDR_LENGTH = 6 };

#pragma pack(push, 1)

struct lgtd_lifx_packet_header {
    //! Packet size including the headers (i.e: this structure).
    uint16le_t      size;
    struct {
        //! Protocol version should be LGTD_LIFX_LIFX_PROTOCOL_V1.
        uint16le_t  version:12;
        //! True when the target field holds a device address.
        uint16le_t  addressable:1;
        //! True when the target field holds tags.
        uint16le_t  tagged:1;
        //! LIFX internal use should be 0.
        uint16le_t  origin:2;
    }               protocol;
    //! This seems to be for LIFX internal use only.
    uint32le_t      source;
    union {
        //! All targeted tags ORed together.
        uint64le_t  tags;
        //! Address of the targeted device.
        uint8_t     device_addr[LGTD_LIFX_ADDR_LENGTH];
    }               target;
    uint8_t         site[LGTD_LIFX_ADDR_LENGTH];
    struct {
        //! True when a response is required, called acknowledge in lifx-gem...
        uint8_t     response_required:1;
        //! True when an acknowledgement is required, no idea what it means.
        uint8_t     ack_required:1;
        uint8_t     reserved:6;
    }               flags;
    //! Wrap-around sequence number, LIFX internal use.
    uint8_t         seqn;
    uint64le_t      timestamp;
    uint16le_t      packet_type;
    uint8_t         reserved[2];
};

enum { LGTD_LIFX_PACKET_HEADER_SIZE = sizeof(struct lgtd_lifx_packet_header) };

enum { LGTD_LIFX_PROTOCOL_V1 = 1024 };

// Let's define a maximum packet size just in case somebody sends us weird
// headers:
enum { LGTD_LIFX_MAX_PACKET_SIZE = 4096 };

enum lgtd_lifx_packet_type {
    LGTD_LIFX_GET_PAN_GATEWAY = 0x02,
    LGTD_LIFX_PAN_GATEWAY = 0x03,
    LGTD_LIFX_GET_TIME = 0x04,
    LGTD_LIFX_SET_TIME = 0x05,
    LGTD_LIFX_TIME_STATE = 0x06,
    LGTD_LIFX_GET_RESET_SWITCH_STATE = 0x07,
    LGTD_LIFX_RESET_SWITCH_STATE = 0x08,
    LGTD_LIFX_GET_MESH_INFO = 0x0c,
    LGTD_LIFX_MESH_INFO = 0x0d,
    LGTD_LIFX_GET_MESH_FIRMWARE = 0x0e,
    LGTD_LIFX_MESH_FIRMWARE = 0x0f,
    LGTD_LIFX_GET_WIFI_INFO = 0x10,
    LGTD_LIFX_WIFI_INFO = 0x11,
    LGTD_LIFX_GET_WIFI_FIRMWARE_STATE = 0x12,
    LGTD_LIFX_WIFI_FIRMWARE_STATE = 0x13,
    LGTD_LIFX_GET_POWER_STATE = 0x14,
    LGTD_LIFX_SET_POWER_STATE = 0x15,
    LGTD_LIFX_POWER_STATE = 0x16,
    LGTD_LIFX_GET_BULB_LABEL = 0x17,
    LGTD_LIFX_SET_BULB_LABEL = 0x18,
    LGTD_LIFX_BULB_LABEL = 0x19,
    LGTD_LIFX_GET_TAGS = 0x1a,
    LGTD_LIFX_SET_TAGS = 0x1b,
    LGTD_LIFX_TAGS = 0x1c,
    LGTD_LIFX_GET_TAG_LABELS = 0x1d,
    LGTD_LIFX_SET_TAG_LABELS = 0x1e,
    LGTD_LIFX_TAG_LABELS = 0x1f,
    LGTD_LIFX_GET_VERSION = 0x20,
    LGTD_LIFX_VERSION_STATE = 0x21,
    LGTD_LIFX_GET_INFO = 0x22,
    LGTD_LIFX_INFO_STATE = 0x23,
    LGTD_LIFX_GET_MCU_RAIL_VOLTAGE = 0x24,
    LGTD_LIFX_MCU_RAIL_VOLTAGE = 0x25,
    LGTD_LIFX_REBOOT = 0x26,
    LGTD_LIFX_SET_FACTORY_TEST_MODE = 0x27,
    LGTD_LIFX_DISABLE_FACTORY_TEST_MODE = 0x28,
    LGTD_LIFX_GET_LIGHT_STATE = 0x65,
    LGTD_LIFX_SET_LIGHT_COLOR = 0x66,
    LGTD_LIFX_SET_WAVEFORM = 0x67,
    LGTD_LIFX_SET_DIM_ABSOLUTE = 0x68,
    LGTD_LIFX_SET_DIM_RELATIVE = 0x69,
    LGTD_LIFX_LIGHT_STATUS = 0x6b,
    LGTD_LIFX_GET_WIFI_STATE = 0x12d,
    LGTD_LIFX_SET_WIFI_STATE = 0x12e,
    LGTD_LIFX_WIFI_STATE = 0x12f,
    LGTD_LIFX_GET_ACCESS_POINTS = 0x130,
    LGTD_LIFX_SET_ACCESS_POINTS = 0x131,
    LGTD_LIFX_ACCESS_POINT = 0x132,
};

enum { LGTD_LIFX_LABEL_SIZE = 32 };

struct lgtd_lifx_packet_light_status {
    uint16le_t  hue;
    uint16le_t  saturation;
    uint16le_t  brightness;
    uint16le_t  kelvin;
    uint16le_t  dim;
    uint16le_t  power;
    uint8_t     label[LGTD_LIFX_LABEL_SIZE];
    uint64be_t  tags;
};

enum lgtd_lifx_power_state {
    LGTD_LIFX_POWER_OFF = 0,
    LGTD_LIFX_POWER_ON = 0xffff
};

struct lgtd_lifx_packet_power_state {
    uint16_t    power; // see enum lgtd_lifx_power_state
};

enum lgtd_lifx_service_type {
    LGTD_LIFX_SERVICE_TCP = 1,
    LGTD_LIFX_SERVICE_UDP = 2
};

struct lgtd_lifx_packet_pan_gateway {
    uint8_t     service_type; // see enum lgtd_lifx_service_type
    uint32le_t  port;
};

enum lgtd_lifx_target_type {
    LGTD_LIFX_TARGET_SITE,
    LGTD_LIFX_TARGET_TAGS,
    LGTD_LIFX_TARGET_DEVICE,
    LGTD_LIFX_TARGET_ALL_DEVICES
};

struct lgtd_lifx_packet_light_color {
    uint8_t     stream; // should be 0
    uint16le_t  hue;
    uint16le_t  saturation;
    uint16le_t  brightness;
    uint16le_t  kelvin;
    uint32le_t  transition; // transition time to the color in msecs
};

#pragma pack(pop)

struct lgtd_lifx_gateway;

struct lgtd_lifx_packet_infos {
    RB_ENTRY(lgtd_lifx_packet_infos)    link;
    const char                          *name;
    enum lgtd_lifx_packet_type          type;
    unsigned                            size;
    void                                (*decode)(void *);
    void                                (*encode)(void *);
    void                                (*handle)(struct lgtd_lifx_gateway *,
                                                  const struct lgtd_lifx_packet_header *,
                                                  const void *);
};
RB_HEAD(lgtd_lifx_packet_infos_map, lgtd_lifx_packet_infos);

static inline int
lgtd_lifx_packet_infos_cmp(struct lgtd_lifx_packet_infos *a,
                           struct lgtd_lifx_packet_infos *b)
{
    return a->type - b->type;
}

union lgtd_lifx_target {
    uint64_t        tags;
    const uint8_t   *addr; //! site or device address
};

extern union lgtd_lifx_target LGTD_LIFX_UNSPEC_TARGET;

const struct lgtd_lifx_packet_infos *lgtd_lifx_wire_get_packet_infos(enum lgtd_lifx_packet_type);
void lgtd_lifx_wire_load_packet_infos_map(void);

const struct lgtd_lifx_packet_infos *lgtd_lifx_wire_setup_header(struct lgtd_lifx_packet_header *,
                                                                 enum lgtd_lifx_target_type,
                                                                 union lgtd_lifx_target,
                                                                 const uint8_t *,
                                                                 enum lgtd_lifx_packet_type);
void lgtd_lifx_wire_decode_header(struct lgtd_lifx_packet_header *);
void lgtd_lifx_wire_encode_header(struct lgtd_lifx_packet_header *);

void lgtd_lifx_wire_decode_pan_gateway(struct lgtd_lifx_packet_pan_gateway *);
void lgtd_lifx_wire_encode_pan_gateway(struct lgtd_lifx_packet_pan_gateway *);
void lgtd_lifx_wire_decode_light_status(struct lgtd_lifx_packet_light_status *);
void lgtd_lifx_wire_encode_light_status(struct lgtd_lifx_packet_light_status *);
void lgtd_lifx_wire_decode_power_state(struct lgtd_lifx_packet_power_state *);

void lgtd_lifx_wire_encode_light_color(struct lgtd_lifx_packet_light_color *);
