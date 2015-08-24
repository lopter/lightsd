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
typedef float    floatle_t;

static inline floatle_t
lgtd_lifx_wire_htolefloat(float f)
{
    union { float f; uint32_t i; } u = { .f = f };
    htole32(u.i);
    return u.f;
}

static inline floatle_t
lgtd_lifx_wire_lefloattoh(float f)
{
    union { float f; uint32_t i; } u = { .f = f };
    le32toh(u.i);
    return u.f;
}

enum { LGTD_LIFX_PROTOCOL_PORT = 56700 };

enum { LGTD_LIFX_ADDR_LENGTH = 6 };
enum { LGTD_LIFX_ADDR_STRLEN = 32 };

#pragma pack(push, 1)

struct lgtd_lifx_packet_header {
    //! Packet size including the headers (i.e: this structure).
    uint16le_t      size;
    //! 15 (MSB)          13           12                11             0
    //! +-----------------+------------+-----------------+--------------+
    //! | origin (2 bits) | tagged (1) | addressable (1) | version (12) |
    //! +-----------------+------------+-----------------+--------------+
    //!
    //! - version: protocol version should be LGTD_LIFX_PROTOCOL_V1;
    //! - addressable: true when the target field holds a device address;
    //! - tagged: true when the target field holds tags;
    //! - origin: LIFX internal use, should be 0.
    uint16le_t      protocol;
    //! Here is what LIFXKit says about it, maybe it's related to zigbee:
    //! Message source identifier from NAT table (Internal LIFX use)
    uint32le_t      source;
    union {
        //! All targeted tags ORed together.
        uint64le_t  tags;
        //! Address of the targeted device.
        uint8_t     device_addr[LGTD_LIFX_ADDR_LENGTH];
    }               target;
    uint8_t         site[LGTD_LIFX_ADDR_LENGTH];
    //! 7                   2                  1                  0
    //! +-------------------+------------------+------------------+
    //! | reserved (6 bits) | ack required (1) | res required (1) |
    //! +-------------------+------------------+------------------+
    //!
    //! - ack required: true when an acknowledge packet is required;
    //! - res required: true when a response is required (the response type
    //!   depends on the request type).
    uint8_t         flags;
    //! Wrap-around sequence number, LIFX internal use.
    uint8_t         seqn;
    //! Apparently this is a unix epoch timestamp in milliseconds at which the
    //! payload should be run.
    uint64le_t      at_time;
    uint16le_t      packet_type;
    uint8_t         reserved[2];
};

enum { LGTD_LIFX_PACKET_HEADER_SIZE = sizeof(struct lgtd_lifx_packet_header) };

enum lgtd_lifx_protocol {
    LGTD_LIFX_PROTOCOL_V1 = 0x400,
#if LGTD_BIG_ENDIAN_SYSTEM
    LGTD_LIFX_PROTOCOL_VERSION_MASK = 0xff0f,
    LGTD_LIFX_PROTOCOL_FLAGS_MASK = 0x00f0,
    LGTD_LIFX_PROTOCOL_ADDRESSABLE = 0x0010,
    LGTD_LIFX_PROTOCOL_TAGGED = 0x0020
#else
    LGTD_LIFX_PROTOCOL_VERSION_MASK = 0x0fff,
    LGTD_LIFX_PROTOCOL_FLAGS_MASK = 0xf000,
    LGTD_LIFX_PROTOCOL_ADDRESSABLE = 0x1000,
    LGTD_LIFX_PROTOCOL_TAGGED = 0x2000
#endif
};

enum lgtd_lifx_flags {
    LGTD_LIFX_FLAG_ACK_REQUIRED = 1 << 1,
    LGTD_LIFX_FLAG_RES_REQUIRED = 1
};

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
    LGTD_LIFX_ACK = 0x2d,
    LGTD_LIFX_ECHO_REQUEST = 0x3a,
    LGTD_LIFX_ECHO_RESPONSE = 0x3b,
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

// note: those can be used as indexes for lgtd_lifx_waveform_table
enum lgtd_lifx_waveform_type {
    LGTD_LIFX_WAVEFORM_SAW = 0,
    LGTD_LIFX_WAVEFORM_SINE = 1,
    LGTD_LIFX_WAVEFORM_HALF_SINE = 2,
    LGTD_LIFX_WAVEFORM_TRIANGLE = 3,
    LGTD_LIFX_WAVEFORM_SQUARE = 4,
    LGTD_LIFX_WAVEFORM_INVALID = 5,
};

struct lgtd_lifx_packet_waveform {
    uint8_t     stream;
    uint8_t     transient;
    uint16le_t  hue;
    uint16le_t  saturation;
    uint16le_t  brightness;
    uint16le_t  kelvin;
    uint32le_t  period; // milliseconds
    floatle_t   cycles; // yes, this value is really encoded as a float.
    uint16le_t  skew_ratio;
    uint8_t     waveform; // see enum lgtd_lifx_waveform_type
};

enum { LGTD_LIFX_ALL_TAGS = ~0 };
struct lgtd_lifx_packet_tags {
    uint64le_t  tags;
};

struct lgtd_lifx_packet_tag_labels {
    uint64le_t  tags;
    char        label[LGTD_LIFX_LABEL_SIZE];
};

struct lgtd_lifx_packet_ip_state {
    floatle_t   signal_strength;
    uint32le_t  tx_bytes;
    uint32le_t  rx_bytes;
    uint16le_t  temperature;
};

struct lgtd_lifx_packet_ip_firmware_info {
    uint64le_t  built_at;
    uint64le_t  installed_at;
    uint32le_t  version;
};

struct lgtd_lifx_packet_product_info {
    uint32le_t  vendor_id;
    uint32le_t  product_id;
    uint32le_t  version;
};

struct lgtd_lifx_packet_runtime_info {
    uint64le_t  time;
    uint64le_t  uptime;
    uint64le_t  downtime;
};
#pragma pack(pop)

enum { LGTD_LIFX_VENDOR_ID = 1 };

enum lgtd_lifx_header_flags {
    LGTD_LIFX_ADDRESSABLE = 1,
    LGTD_LIFX_TAGGED = 1 << 1,
    LGTD_LIFX_ACK_REQUIRED = 1 << 2,
    LGTD_LIFX_RES_REQUIRED = 1 << 3
};

struct lgtd_lifx_waveform_string_id {
    const char  *str;
    int         len;
};

extern const struct lgtd_lifx_waveform_string_id lgtd_lifx_waveform_table[];

struct lgtd_lifx_gateway;

struct lgtd_lifx_packet_info {
    RB_ENTRY(lgtd_lifx_packet_info)     link;
    const char                          *name;
    enum lgtd_lifx_packet_type          type;
    unsigned                            size;
    void                                (*decode)(void *);
    void                                (*encode)(void *);
    void                                (*handle)(struct lgtd_lifx_gateway *,
                                                  const struct lgtd_lifx_packet_header *,
                                                  const void *);
};
RB_HEAD(lgtd_lifx_packet_info_map, lgtd_lifx_packet_info);

static inline int
lgtd_lifx_packet_info_cmp(struct lgtd_lifx_packet_info *a,
                           struct lgtd_lifx_packet_info *b)
{
    return a->type - b->type;
}

union lgtd_lifx_target {
    uint64_t        tags;
    const uint8_t   *addr; //! site or device address
};

extern const union lgtd_lifx_target LGTD_LIFX_UNSPEC_TARGET;

#if LGTD_SIZEOF_VOID_P == 8
#   define LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(x) (1UL << (x))
#elif LGTD_SIZEOF_VOID_P == 4
#   define LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(x) (1ULL << (x))
#endif

// Kim Walisch (2012)
// http://chessprogramming.wikispaces.com/BitScan#DeBruijnMultiplation

enum { LGTD_LIFX_DEBRUIJN_NUMBER = 0x03f79d71b4cb0a89 };
extern const int LGTD_LIFX_DEBRUIJN_SEQUENCE[64];

static inline int
lgtd_lifx_wire_bitscan64_forward(uint64_t n)
{
    return n ? LGTD_LIFX_DEBRUIJN_SEQUENCE[
        ((n ^ (n - 1)) * LGTD_LIFX_DEBRUIJN_NUMBER) >> 58
    ] : -1;
}

static inline int
lgtd_lifx_wire_next_tag_id(int current_tag_id, uint64_t tags)
{
    // A bitshift >= than the width of the type is undefined behavior in C:
    if (current_tag_id < 63) {
        tags &= ~0ULL << (current_tag_id + 1);
        return lgtd_lifx_wire_bitscan64_forward(tags);
    }
    return -1;
}

#define LGTD_LIFX_WIRE_FOREACH_TAG_ID(tag_id_varname, tags)                         \
    for ((tag_id_varname) = lgtd_lifx_wire_next_tag_id(-1, (tags));                 \
         (tag_id_varname) != -1;                                                    \
         (tag_id_varname) = lgtd_lifx_wire_next_tag_id((tag_id_varname), (tags)))

enum lgtd_lifx_waveform_type lgtd_lifx_wire_waveform_string_id_to_type(const char *, int);
char* lgtd_lifx_wire_print_nsec_timestamp(uint64_t, char *, int);
#define LGTD_LIFX_WIRE_PRINT_NSEC_TIMESTAMP(ts, arr) \
    lgtd_lifx_wire_print_nsec_timestamp((ts), (arr), sizeof((arr)))

const struct lgtd_lifx_packet_info *lgtd_lifx_wire_get_packet_info(enum lgtd_lifx_packet_type);
void lgtd_lifx_wire_load_packet_info_map(void);

const struct lgtd_lifx_packet_info *lgtd_lifx_wire_setup_header(struct lgtd_lifx_packet_header *,
                                                                 enum lgtd_lifx_target_type,
                                                                 union lgtd_lifx_target,
                                                                 const uint8_t *,
                                                                 enum lgtd_lifx_packet_type);
void lgtd_lifx_wire_decode_header(struct lgtd_lifx_packet_header *);

void lgtd_lifx_wire_decode_pan_gateway(struct lgtd_lifx_packet_pan_gateway *);
void lgtd_lifx_wire_encode_pan_gateway(struct lgtd_lifx_packet_pan_gateway *);
void lgtd_lifx_wire_decode_light_status(struct lgtd_lifx_packet_light_status *);
void lgtd_lifx_wire_encode_light_status(struct lgtd_lifx_packet_light_status *);
void lgtd_lifx_wire_decode_power_state(struct lgtd_lifx_packet_power_state *);

void lgtd_lifx_wire_encode_light_color(struct lgtd_lifx_packet_light_color *);
void lgtd_lifx_wire_encode_waveform(struct lgtd_lifx_packet_waveform *);

void lgtd_lifx_wire_encode_tags(struct lgtd_lifx_packet_tags *);
void lgtd_lifx_wire_decode_tags(struct lgtd_lifx_packet_tags *);
void lgtd_lifx_wire_encode_tag_labels(struct lgtd_lifx_packet_tag_labels *);
void lgtd_lifx_wire_decode_tag_labels(struct lgtd_lifx_packet_tag_labels *);

void lgtd_lifx_wire_decode_ip_state(struct lgtd_lifx_packet_ip_state *);
void lgtd_lifx_wire_decode_ip_firmware_info(struct lgtd_lifx_packet_ip_firmware_info *);
void lgtd_lifx_wire_decode_product_info(struct lgtd_lifx_packet_product_info *);
void lgtd_lifx_wire_decode_runtime_info(struct lgtd_lifx_packet_runtime_info *);
