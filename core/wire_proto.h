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

#pragma pack(push, 1)

typedef uint16_t uint16le_t;
typedef uint16_t uint16be_t;
typedef uint32_t uint32le_t;
typedef uint32_t uint32be_t;
typedef uint64_t uint64le_t;
typedef uint64_t uint64be_t;

enum { LIFXD_ADDR_LENGTH = 6 };

struct lifxd_packet_header {
    uint16le_t  size;
    uint16be_t  protocol;
    uint8_t     reserved1[4];
    uint8_t     bulb_addr[LIFXD_ADDR_LENGTH];
    uint8_t     reserved2[2];
    uint8_t     gw_addr[LIFXD_ADDR_LENGTH];
    uint8_t     reserved3[2];
    uint64be_t  timestamp;
    uint16le_t  packet_type;
    uint8_t     reserved4[2];
};

enum { LIFXD_PACKET_HEADER_SIZE = sizeof(struct lifxd_packet_header) };
enum { LIFXD_PROTOCOL_VERSION = 0x54 };

// Let's define a maximum packet size just in case somebody sends us weird
// headers:
enum { LIFXD_MAX_PACKET_SIZE = 4096 };

enum lifxd_packet_type {
    LIFXD_GET_PAN_GATEWAY = 0x02,
    LIFXD_PAN_GATEWAY = 0x03,
    LIFXD_GET_TIME = 0x04,
    LIFXD_SET_TIME = 0x05,
    LIFXD_TIME_STATE = 0x06,
    LIFXD_GET_RESET_SWITCH_STATE = 0x07,
    LIFXD_RESET_SWITCH_STATE = 0x08,
    LIFXD_GET_MESH_INFO = 0x0c,
    LIFXD_MESH_INFO = 0x0d,
    LIFXD_GET_MESH_FIRMWARE = 0x0e,
    LIFXD_MESH_FIRMWARE = 0x0f,
    LIFXD_GET_WIFI_INFO = 0x10,
    LIFXD_WIFI_INFO = 0x11,
    LIFXD_GET_WIFI_FIRMWARE_STATE = 0x12,
    LIFXD_WIFI_FIRMWARE_STATE = 0x13,
    LIFXD_GET_POWER_STATE = 0x14,
    LIFXD_SET_POWER_STATE = 0x15,
    LIFXD_POWER_STATE = 0x16,
    LIFXD_GET_BULB_LABEL = 0x17,
    LIFXD_SET_BULB_LABEL = 0x18,
    LIFXD_BULB_LABEL = 0x19,
    LIFXD_GET_TAGS = 0x1a,
    LIFXD_SET_TAGS = 0x1b,
    LIFXD_TAGS = 0x1c,
    LIFXD_GET_TAG_LABELS = 0x1d,
    LIFXD_SET_TAG_LABELS = 0x1e,
    LIFXD_TAG_LABELS = 0x1f,
    LIFXD_GET_VERSION = 0x20,
    LIFXD_VERSION_STATE = 0x21,
    LIFXD_GET_INFO = 0x22,
    LIFXD_INFO_STATE = 0x23,
    LIFXD_GET_MCU_RAIL_VOLTAGE = 0x24,
    LIFXD_MCU_RAIL_VOLTAGE = 0x25,
    LIFXD_REBOOT = 0x26,
    LIFXD_SET_FACTORY_TEST_MODE = 0x27,
    LIFXD_DISABLE_FACTORY_TEST_MODE = 0x28,
    LIFXD_GET_LIGHT_STATE = 0x65,
    LIFXD_SET_LIGHT_COLOUR = 0x66,
    LIFXD_SET_WAVEFORM = 0x67,
    LIFXD_SET_DIM_ABSOLUTE = 0x68,
    LIFXD_SET_DIM_RELATIVE = 0x69,
    LIFXD_LIGHT_STATUS = 0x6b,
    LIFXD_GET_WIFI_STATE = 0x12d,
    LIFXD_SET_WIFI_STATE = 0x12e,
    LIFXD_WIFI_STATE = 0x12f,
    LIFXD_GET_ACCESS_POINTS = 0x130,
    LIFXD_SET_ACCESS_POINTS = 0x131,
    LIFXD_ACCESS_POINT = 0x132,
};

enum { LIFXD_LABEL_SIZE = 32 };

struct lifxd_packet_light_status {
    uint16le_t  hue;
    uint16le_t  saturation;
    uint16le_t  brightness;
    uint16le_t  kelvin;
    uint16le_t  dim;
    uint16le_t  power;
    uint8_t     label[LIFXD_LABEL_SIZE];
    uint64be_t  tags;
};

enum lifxd_power_state {
    LIFXD_POWER_OFF = 0,
    LIFXD_POWER_ON = 0xffff
};

struct lifxd_packet_power_state {
    uint16_t    power; // see enum lifxd_power_state
};

enum lifxd_service_type {
    LIFXD_SERVICE_TCP = 1,
    LIFXD_SERVICE_UDP = 2
};

struct lifxd_packet_pan_gateway {
    uint8_t     service_type; // see enum lifxd_service_type
    uint32le_t  port;
};

void lifxd_wire_decode_header(struct lifxd_packet_header *);
void lifxd_wire_encode_header(struct lifxd_packet_header *);
void lifxd_wire_dump_header(const struct lifxd_packet_header *);
void lifxd_wire_encode_packet(void *, enum lifxd_packet_type);

void lifxd_wire_decode_pan_gateway(struct lifxd_packet_pan_gateway *);
void lifxd_wire_encode_pan_gateway(struct lifxd_packet_pan_gateway *);
void lifxd_wire_decode_light_status(struct lifxd_packet_light_status *);
void lifxd_wire_encode_light_status(struct lifxd_packet_light_status *);
void lifxd_wire_decode_power_state(struct lifxd_packet_power_state *);

#pragma pack(pop)
