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

#include <sys/queue.h>
#include <sys/tree.h>
#include <arpa/inet.h>
#include <assert.h>
#include <endian.h>
#include <err.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <event2/util.h>

#include "wire_proto.h"
#include "core/time_monotonic.h"
#include "bulb.h"
#include "gateway.h"
#include "core/lightsd.h"

const union lgtd_lifx_target LGTD_LIFX_UNSPEC_TARGET = { .tags = 0 };

const int LGTD_LIFX_DEBRUIJN_SEQUENCE[64] = {
    0, 47,  1, 56, 48, 27,  2, 60,
   57, 49, 41, 37, 28, 16,  3, 61,
   54, 58, 35, 52, 50, 42, 21, 44,
   38, 32, 29, 23, 17, 11,  4, 62,
   46, 55, 26, 59, 40, 36, 15, 53,
   34, 51, 20, 43, 31, 22, 10, 45,
   25, 39, 14, 33, 19, 30,  9, 24,
   13, 18,  8, 12,  7,  6,  5, 63
};

static struct lgtd_lifx_packet_info_map lgtd_lifx_packet_info =
    RB_INITIALIZER(&lgtd_lifx_packets_infos);

RB_GENERATE_STATIC(
    lgtd_lifx_packet_info_map,
    lgtd_lifx_packet_info,
    link,
    lgtd_lifx_packet_info_cmp
);

static void
lgtd_lifx_wire_null_packet_encoder_decoder(void *pkt)
{
    (void)pkt;
}

static void
lgtd_lifx_wire_null_packet_handler(struct lgtd_lifx_gateway *gw,
                                   const struct lgtd_lifx_packet_header *hdr,
                                   const void *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}

static void
lgtd_lifx_wire_enosys_packet_handler(struct lgtd_lifx_gateway *gw,
                                     const struct lgtd_lifx_packet_header *hdr,
                                     const void *pkt)
{
    (void)pkt;

    const struct lgtd_lifx_packet_info *pkt_info;
    pkt_info = lgtd_lifx_wire_get_packet_info(hdr->packet_type);
    bool addressable = hdr->protocol & LGTD_LIFX_PROTOCOL_ADDRESSABLE;
    bool tagged = hdr->protocol & LGTD_LIFX_PROTOCOL_TAGGED;
    unsigned int protocol = hdr->protocol & LGTD_LIFX_PROTOCOL_VERSION_MASK;
    lgtd_info(
        "%s <-- [%s]:%hu - (Unimplemented, header info: "
        "addressable=%d, tagged=%d, protocol=%d)",
        pkt_info->name, gw->ip_addr, gw->port,
        addressable, tagged, protocol
    );
}

void
lgtd_lifx_wire_load_packet_info_map(void)
{
#define DECODER(x)  ((void (*)(void *))(x))
#define ENCODER(x)  ((void (*)(void *))(x))
#define HANDLER(x)                                      \
    ((void (*)(struct lgtd_lifx_gateway *,              \
               const struct lgtd_lifx_packet_header *,  \
               const void *))(x))
#define NO_PAYLOAD                                          \
    .encode = lgtd_lifx_wire_null_packet_encoder_decoder
#define RESPONSE_ONLY                                       \
    .encode = lgtd_lifx_wire_null_packet_encoder_decoder
#define REQUEST_ONLY                                        \
    .decode = lgtd_lifx_wire_null_packet_encoder_decoder,   \
    .handle = lgtd_lifx_wire_null_packet_handler
#define UNIMPLEMENTED                                       \
    .decode = lgtd_lifx_wire_null_packet_encoder_decoder,   \
    .encode = lgtd_lifx_wire_null_packet_encoder_decoder,   \
    .handle = lgtd_lifx_wire_enosys_packet_handler

    static struct lgtd_lifx_packet_info packet_table[] = {
        // Gateway packets:
        {
            REQUEST_ONLY,
            NO_PAYLOAD,
            .name = "GET_PAN_GATEWAY",
            .type = LGTD_LIFX_GET_PAN_GATEWAY
        },
        {
            .name = "PAN_GATEWAY",
            .type = LGTD_LIFX_PAN_GATEWAY,
            .size = sizeof(struct lgtd_lifx_packet_pan_gateway),
            .decode = DECODER(lgtd_lifx_wire_decode_pan_gateway),
            .encode = ENCODER(lgtd_lifx_wire_encode_pan_gateway),
            .handle = HANDLER(lgtd_lifx_gateway_handle_pan_gateway)
        },
        {
            REQUEST_ONLY,
            .name = "SET_TAG_LABELS",
            .type = LGTD_LIFX_SET_TAG_LABELS,
            .size = sizeof(struct lgtd_lifx_packet_tag_labels),
            .encode = ENCODER(lgtd_lifx_wire_encode_tag_labels)
        },
        {
            REQUEST_ONLY,
            .name = "GET_TAG_LABELS",
            .type = LGTD_LIFX_GET_TAG_LABELS,
            .size = sizeof(struct lgtd_lifx_packet_tags),
            .encode = ENCODER(lgtd_lifx_wire_encode_tags)
        },
        {
            RESPONSE_ONLY,
            .name = "TAG_LABELS",
            .type = LGTD_LIFX_TAG_LABELS,
            .size = sizeof(struct lgtd_lifx_packet_tag_labels),
            .decode = DECODER(lgtd_lifx_wire_decode_tag_labels),
            .handle = HANDLER(lgtd_lifx_gateway_handle_tag_labels)
        },
        // Bulb packets:
        {
            REQUEST_ONLY,
            .name = "SET_LIGHT_COLOR",
            .type = LGTD_LIFX_SET_LIGHT_COLOR,
            .size = sizeof(struct lgtd_lifx_packet_light_color),
            .encode = ENCODER(lgtd_lifx_wire_encode_light_color)
        },
        {
            REQUEST_ONLY,
            .name = "SET_WAVEFORM",
            .type = LGTD_LIFX_SET_WAVEFORM,
            .size = sizeof(struct lgtd_lifx_packet_waveform),
            .encode = ENCODER(lgtd_lifx_wire_encode_waveform)
        },
        {
            REQUEST_ONLY,
            NO_PAYLOAD,
            .name = "GET_LIGHT_STATUS",
            .type = LGTD_LIFX_GET_LIGHT_STATE
        },
        {
            RESPONSE_ONLY,
            .name = "LIGHT_STATUS",
            .type = LGTD_LIFX_LIGHT_STATUS,
            .size = sizeof(struct lgtd_lifx_packet_light_status),
            .decode = DECODER(lgtd_lifx_wire_decode_light_status),
            .handle = HANDLER(lgtd_lifx_gateway_handle_light_status)
        },
        {
            REQUEST_ONLY,
            NO_PAYLOAD, // well it has a payload, but it's just 0 or 1...
            .size = sizeof(struct lgtd_lifx_packet_power_state),
            .name = "SET_POWER_STATE",
            .type = LGTD_LIFX_SET_POWER_STATE,
        },
        {
            RESPONSE_ONLY,
            .name = "POWER_STATE",
            .type = LGTD_LIFX_POWER_STATE,
            .size = sizeof(struct lgtd_lifx_packet_power_state),
            .decode = DECODER(lgtd_lifx_wire_decode_power_state),
            .handle = HANDLER(lgtd_lifx_gateway_handle_power_state)
        },
        {
            REQUEST_ONLY,
            .name = "SET_TAGS",
            .type = LGTD_LIFX_SET_TAGS,
            .size = sizeof(struct lgtd_lifx_packet_tags),
            .encode = ENCODER(lgtd_lifx_wire_encode_tags)
        },
        {
            RESPONSE_ONLY,
            .name = "TAGS",
            .type = LGTD_LIFX_TAGS,
            .size = sizeof(struct lgtd_lifx_packet_tags),
            .decode = DECODER(lgtd_lifx_wire_decode_tags),
            .handle = HANDLER(lgtd_lifx_gateway_handle_tags)
        },
        {
            REQUEST_ONLY,
            NO_PAYLOAD,
            .name = "GET_MESH_INFO",
            .type = LGTD_LIFX_GET_MESH_INFO
        },
        {
            RESPONSE_ONLY,
            .name = "MESH_INFO",
            .type = LGTD_LIFX_MESH_INFO,
            .size = sizeof(struct lgtd_lifx_packet_ip_state),
            .decode = DECODER(lgtd_lifx_wire_decode_ip_state),
            .handle = HANDLER(lgtd_lifx_gateway_handle_ip_state)
        },
        {
            REQUEST_ONLY,
            NO_PAYLOAD,
            .name = "GET_MESH_FIRMWARE",
            .type = LGTD_LIFX_GET_MESH_FIRMWARE
        },
        {
            RESPONSE_ONLY,
            .name = "MESH_FIRMWARE",
            .type = LGTD_LIFX_MESH_FIRMWARE,
            .size = sizeof(struct lgtd_lifx_packet_ip_firmware_info),
            .decode = DECODER(lgtd_lifx_wire_decode_ip_firmware_info),
            .handle = HANDLER(lgtd_lifx_gateway_handle_ip_firmware_info)
        },
        {
            REQUEST_ONLY,
            NO_PAYLOAD,
            .name = "GET_WIFI_INFO",
            .type = LGTD_LIFX_GET_WIFI_INFO,
        },
        {
            RESPONSE_ONLY,
            .name = "WIFI_INFO",
            .type = LGTD_LIFX_WIFI_INFO,
            .size = sizeof(struct lgtd_lifx_packet_ip_state),
            .decode = DECODER(lgtd_lifx_wire_decode_ip_state),
            .handle = HANDLER(lgtd_lifx_gateway_handle_ip_state)
        },
        {
            REQUEST_ONLY,
            NO_PAYLOAD,
            .name = "GET_WIFI_FIRMWARE_STATE",
            .type = LGTD_LIFX_GET_WIFI_FIRMWARE_STATE
        },
        {
            RESPONSE_ONLY,
            .name = "WIFI_FIRMWARE_STATE",
            .type = LGTD_LIFX_WIFI_FIRMWARE_STATE,
            .size = sizeof(struct lgtd_lifx_packet_ip_firmware_info),
            .decode = DECODER(lgtd_lifx_wire_decode_ip_firmware_info),
            .handle = HANDLER(lgtd_lifx_gateway_handle_ip_firmware_info)
        },
        {
            REQUEST_ONLY,
            NO_PAYLOAD,
            .name = "GET_VERSION",
            .type = LGTD_LIFX_GET_VERSION
        },
        {
            RESPONSE_ONLY,
            .name = "VERSION_STATE",
            .type = LGTD_LIFX_VERSION_STATE,
            .size = sizeof(struct lgtd_lifx_packet_product_info),
            .decode = DECODER(lgtd_lifx_wire_decode_product_info),
            .handle = HANDLER(lgtd_lifx_gateway_handle_product_info)
        },
        {
            REQUEST_ONLY,
            NO_PAYLOAD,
            .name = "GET_INFO",
            .type = LGTD_LIFX_GET_INFO
        },
        {
            RESPONSE_ONLY,
            .name = "INFO_STATE",
            .type = LGTD_LIFX_INFO_STATE,
            .size = sizeof(struct lgtd_lifx_packet_runtime_info),
            .decode = DECODER(lgtd_lifx_wire_decode_runtime_info),
            .handle = HANDLER(lgtd_lifx_gateway_handle_runtime_info)
        },
        {
            REQUEST_ONLY,
            .encode = lgtd_lifx_wire_null_packet_encoder_decoder,
            .name = "SET_BULB_LABEL",
            .type = LGTD_LIFX_SET_BULB_LABEL,
            .size = sizeof(struct lgtd_lifx_packet_label)
        },
        {
            RESPONSE_ONLY,
            .name = "BULB_LABEL",
            .type = LGTD_LIFX_BULB_LABEL,
            .size = sizeof(struct lgtd_lifx_packet_label),
            .decode = lgtd_lifx_wire_null_packet_encoder_decoder,
            .handle = HANDLER(lgtd_lifx_gateway_handle_bulb_label)
        },
        {
            REQUEST_ONLY,
            NO_PAYLOAD,
            .name = "GET_AMBIENT_LIGHT",
            .type = LGTD_LIFX_GET_AMBIENT_LIGHT
        },
        {
            RESPONSE_ONLY,
            .name = "STATE_AMBIENT_LIGHT",
            .type = LGTD_LIFX_STATE_AMBIENT_LIGHT,
            .size = sizeof(struct lgtd_lifx_packet_ambient_light),
            .decode = DECODER(lgtd_lifx_wire_decode_ambient_light),
            .handle = HANDLER(lgtd_lifx_gateway_handle_ambient_light)
        },
        // Unimplemented but "known" packets
        {
            UNIMPLEMENTED,
            .name = "GET_TIME",
            .type = LGTD_LIFX_GET_TIME
        },
        {
            UNIMPLEMENTED,
            .name = "SET_TIME",
            .type = LGTD_LIFX_SET_TIME
        },
        {
            UNIMPLEMENTED,
            .name = "TIME_STATE",
            .type = LGTD_LIFX_TIME_STATE
        },
        {
            UNIMPLEMENTED,
            .name = "GET_RESET_SWITCH_STATE",
            .type = LGTD_LIFX_GET_RESET_SWITCH_STATE
        },
        {
            UNIMPLEMENTED,
            .name = "RESET_SWITCH_STATE",
            .type = LGTD_LIFX_RESET_SWITCH_STATE
        },
        {
            UNIMPLEMENTED,
            .name = "GET_BULB_LABEL",
            .type = LGTD_LIFX_GET_BULB_LABEL
        },
        {
            UNIMPLEMENTED,
            .name = "GET_MCU_RAIL_VOLTAGE",
            .type = LGTD_LIFX_GET_MCU_RAIL_VOLTAGE
        },
        {
            UNIMPLEMENTED,
            .name = "MCU_RAIL_VOLTAGE",
            .type = LGTD_LIFX_MCU_RAIL_VOLTAGE
        },
        {
            UNIMPLEMENTED,
            .name = "REBOOT",
            .type = LGTD_LIFX_REBOOT
        },
        {
            UNIMPLEMENTED,
            .name = "SET_FACTORY_TEST_MODE",
            .type = LGTD_LIFX_SET_FACTORY_TEST_MODE
        },
        {
            UNIMPLEMENTED,
            .name = "DISABLE_FACTORY_TEST_MODE",
            .type = LGTD_LIFX_DISABLE_FACTORY_TEST_MODE
        },
        {
            UNIMPLEMENTED,
            .name = "ACK",
            .type = LGTD_LIFX_ACK
        },
        {
            UNIMPLEMENTED,
            .name = "ECHO_REQUEST",
            .type = LGTD_LIFX_ECHO_REQUEST
        },
        {
            UNIMPLEMENTED,
            .name = "ECHO_RESPONSE",
            .type = LGTD_LIFX_ECHO_RESPONSE
        },
        {
            UNIMPLEMENTED,
            .name = "SET_DIM_ABSOLUTE",
            .type = LGTD_LIFX_SET_DIM_ABSOLUTE
        },
        {
            UNIMPLEMENTED,
            .name = "SET_DIM_RELATIVE",
            .type = LGTD_LIFX_SET_DIM_RELATIVE
        },
        {
            UNIMPLEMENTED,
            .name = "GET_WIFI_STATE",
            .type = LGTD_LIFX_GET_WIFI_STATE
        },
        {
            UNIMPLEMENTED,
            .name = "SET_WIFI_STATE",
            .type = LGTD_LIFX_SET_WIFI_STATE
        },
        {
            UNIMPLEMENTED,
            .name = "WIFI_STATE",
            .type = LGTD_LIFX_WIFI_STATE
        },
        {
            UNIMPLEMENTED,
            .name = "GET_ACCESS_POINTS",
            .type = LGTD_LIFX_GET_ACCESS_POINTS
        },
        {
            UNIMPLEMENTED,
            .name = "SET_ACCESS_POINTS",
            .type = LGTD_LIFX_SET_ACCESS_POINTS
        },
        {
            UNIMPLEMENTED,
            .name = "ACCESS_POINT",
            .type = LGTD_LIFX_ACCESS_POINT
        },
        {
            UNIMPLEMENTED,
            .name = "GET_DIMMER_VOLTAGE",
            .type = LGTD_LIFX_GET_DIMMER_VOLTAGE
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_DIMMER_VOLTAGE",
            .type = LGTD_LIFX_STATE_DIMMER_VOLTAGE
        }
    };

    for (int i = 0; i != LGTD_ARRAY_SIZE(packet_table); ++i) {
        RB_INSERT(
            lgtd_lifx_packet_info_map,
            &lgtd_lifx_packet_info,
            &packet_table[i]
        );
    }
}

const struct lgtd_lifx_packet_info *
lgtd_lifx_wire_get_packet_info(enum lgtd_lifx_packet_type packet_type)
{
    struct lgtd_lifx_packet_info pkt_info = { .type = packet_type };
    return RB_FIND(lgtd_lifx_packet_info_map, &lgtd_lifx_packet_info, &pkt_info);
}


#define WAVEFORM_ENTRY(e) { .str = e, .len = sizeof(e) - 1 }
const struct lgtd_lifx_waveform_string_id lgtd_lifx_waveform_table[] = {
    WAVEFORM_ENTRY("SAW"),
    WAVEFORM_ENTRY("SINE"),
    WAVEFORM_ENTRY("HALF_SINE"),
    WAVEFORM_ENTRY("TRIANGLE"),
    WAVEFORM_ENTRY("SQUARE"),
    WAVEFORM_ENTRY("INVALID")
};

enum lgtd_lifx_waveform_type
lgtd_lifx_wire_waveform_string_id_to_type(const char *s, int len)
{
    assert(s);
    assert(len >= 0);

    for (int i = 0; i != LGTD_ARRAY_SIZE(lgtd_lifx_waveform_table); i++) {
        const struct lgtd_lifx_waveform_string_id *entry;
        entry = &lgtd_lifx_waveform_table[i];
        if (entry->len == len && !memcmp(entry->str, s, len)) {
            return i;
        }
    }

    return LGTD_LIFX_WAVEFORM_INVALID;
}

char *
lgtd_lifx_wire_print_nsec_timestamp(uint64_t nsec_ts, char *buf, int bufsz)
{
    assert(buf);
    assert(bufsz > 0);

    time_t ts = LGTD_NSECS_TO_SECS(nsec_ts);

    struct tm tm_utc;
    if (gmtime_r(&ts, &tm_utc)) {
        int64_t usecs = LGTD_NSECS_TO_USECS(nsec_ts - LGTD_SECS_TO_NSECS(ts));
        LGTD_TM_TO_ISOTIME(&tm_utc, buf, bufsz, usecs);
    } else {
        buf[0] = '\0';
    }

    return buf;
}

static void
lgtd_lifx_wire_encode_header(struct lgtd_lifx_packet_header *hdr, int flags)
{
    assert(hdr);

    hdr->size = htole16(hdr->size);
    hdr->protocol = htole16(LGTD_LIFX_PROTOCOL_V1);
    if (flags & LGTD_LIFX_ADDRESSABLE) {
        hdr->protocol |= LGTD_LIFX_PROTOCOL_ADDRESSABLE;
    }
    if (flags & LGTD_LIFX_TAGGED) {
        hdr->protocol |= LGTD_LIFX_PROTOCOL_TAGGED;
        hdr->target.tags = htole64(hdr->target.tags);
    }
    if (flags & LGTD_LIFX_ACK_REQUIRED) {
        hdr->flags |= LGTD_LIFX_FLAG_ACK_REQUIRED;
    }
    if (flags & LGTD_LIFX_RES_REQUIRED) {
        hdr->flags |= LGTD_LIFX_FLAG_RES_REQUIRED;
    }
    hdr->at_time = htole64(hdr->at_time);
    hdr->packet_type = htole16(hdr->packet_type);
}

// Convert all the fields in the header to the host endianness.
//
// \return The payload size or -1 if the header is invalid.
void
lgtd_lifx_wire_decode_header(struct lgtd_lifx_packet_header *hdr)
{
    assert(hdr);

    hdr->size = le16toh(hdr->size);
    hdr->protocol = (
        le16toh(hdr->protocol & LGTD_LIFX_PROTOCOL_VERSION_MASK)
        | (hdr->protocol & LGTD_LIFX_PROTOCOL_FLAGS_MASK)
    );
    if (hdr->protocol & LGTD_LIFX_PROTOCOL_TAGGED) {
        hdr->target.tags = le64toh(hdr->target.tags);
    }
    hdr->at_time = le64toh(hdr->at_time);
    hdr->packet_type = le16toh(hdr->packet_type);
}

const struct lgtd_lifx_packet_info *
lgtd_lifx_wire_setup_header(struct lgtd_lifx_packet_header *hdr,
                            enum lgtd_lifx_target_type target_type,
                            union lgtd_lifx_target target,
                            const uint8_t *site,
                            enum lgtd_lifx_packet_type packet_type)
{
    assert(hdr);

    const struct lgtd_lifx_packet_info *pkt_info =
        lgtd_lifx_wire_get_packet_info(packet_type);

    assert(pkt_info);

    memset(hdr, 0, sizeof(*hdr));
    hdr->size = pkt_info->size + sizeof(*hdr);
    hdr->packet_type = packet_type;
    if (site) {
        memcpy(hdr->site, site, sizeof(hdr->site));
    } else {
        assert(target_type == LGTD_LIFX_TARGET_ALL_DEVICES);
    }

    int flags = LGTD_LIFX_ADDRESSABLE;
    switch (target_type) {
    case LGTD_LIFX_TARGET_SITE:
    case LGTD_LIFX_TARGET_ALL_DEVICES:
        flags |= LGTD_LIFX_TAGGED;
        break;
    case LGTD_LIFX_TARGET_TAGS:
        flags |= LGTD_LIFX_TAGGED;
        hdr->target.tags = target.tags;
        break;
    case LGTD_LIFX_TARGET_DEVICE:
        memcpy(hdr->target.device_addr, target.addr, LGTD_LIFX_ADDR_LENGTH);
        break;
    }

    lgtd_lifx_wire_encode_header(hdr, flags);

    return pkt_info;
}

void
lgtd_lifx_wire_decode_pan_gateway(struct lgtd_lifx_packet_pan_gateway *pkt)
{
    assert(pkt);

    pkt->port = le32toh(pkt->port);
}

void
lgtd_lifx_wire_encode_pan_gateway(struct lgtd_lifx_packet_pan_gateway *pkt)
{
    assert(pkt);

    pkt->port = htole32(pkt->port);
}

void
lgtd_lifx_wire_decode_light_status(struct lgtd_lifx_packet_light_status *pkt)
{
    assert(pkt);

    pkt->hue = le16toh(pkt->hue);
    pkt->saturation = le16toh(pkt->saturation);
    pkt->brightness = le16toh(pkt->brightness);
    pkt->kelvin = le16toh(pkt->kelvin);
    pkt->dim = le16toh(pkt->dim);
    // The bulbs actually return power values between 0 and 0xffff, not sure
    // what the intermediate values mean, let's pull them down to 0:
    if (pkt->power != LGTD_LIFX_POWER_ON) {
        pkt->power = LGTD_LIFX_POWER_OFF;
    }
    pkt->tags = le64toh(pkt->tags);
}

void
lgtd_lifx_wire_decode_power_state(struct lgtd_lifx_packet_power_state *pkt)
{
    assert(pkt);

    if (pkt->power != LGTD_LIFX_POWER_ON) {
        pkt->power = LGTD_LIFX_POWER_OFF;
    }
}

void
lgtd_lifx_wire_encode_light_color(struct lgtd_lifx_packet_light_color *pkt)
{
    assert(pkt);

    pkt->hue = htole16(pkt->hue);
    pkt->saturation = htole16(pkt->saturation);
    pkt->brightness = htole16(pkt->brightness);
    pkt->kelvin = htole16(pkt->kelvin);
    pkt->transition = htole32(pkt->transition);
}

void
lgtd_lifx_wire_encode_waveform(struct lgtd_lifx_packet_waveform *pkt)
{
    assert(pkt);

    pkt->hue = htole16(pkt->hue);
    pkt->saturation = htole16(pkt->saturation);
    pkt->brightness = htole16(pkt->brightness);
    pkt->kelvin = htole16(pkt->kelvin);
    pkt->period = htole32(pkt->period);
    pkt->cycles = lgtd_lifx_wire_htolefloat(pkt->cycles);
    pkt->skew_ratio = htole16(pkt->skew_ratio);
}

void
lgtd_lifx_wire_encode_tag_labels(struct lgtd_lifx_packet_tag_labels *pkt)
{
    assert(pkt);

    pkt->tags = htole64(pkt->tags);
}

void
lgtd_lifx_wire_decode_tag_labels(struct lgtd_lifx_packet_tag_labels *pkt)
{
    assert(pkt);

    pkt->label[sizeof(pkt->label) - 1] = '\0';
    pkt->tags = le64toh(pkt->tags);
}

void
lgtd_lifx_wire_encode_tags(struct lgtd_lifx_packet_tags *pkt)
{
    assert(pkt);

    pkt->tags = htole64(pkt->tags);
}

void
lgtd_lifx_wire_decode_tags(struct lgtd_lifx_packet_tags *pkt)
{
    assert(pkt);

    pkt->tags = le64toh(pkt->tags);
}

void
lgtd_lifx_wire_decode_ip_state(struct lgtd_lifx_packet_ip_state *pkt)
{
    assert(pkt);

    pkt->signal_strength = lgtd_lifx_wire_lefloattoh(pkt->signal_strength);
    pkt->tx_bytes = le32toh(pkt->tx_bytes);
    pkt->rx_bytes = le32toh(pkt->rx_bytes);
    pkt->temperature = le16toh(pkt->temperature);
}

void
lgtd_lifx_wire_decode_ip_firmware_info(struct lgtd_lifx_packet_ip_firmware_info *pkt)
{
    assert(pkt);

    pkt->built_at = le64toh(pkt->built_at);
    pkt->installed_at = le64toh(pkt->installed_at);
    pkt->version = le32toh(pkt->version);
}

void
lgtd_lifx_wire_decode_product_info(struct lgtd_lifx_packet_product_info *pkt)
{
    assert(pkt);

    pkt->vendor_id = le32toh(pkt->vendor_id);
    pkt->product_id = le32toh(pkt->product_id);
    pkt->version = le32toh(pkt->version);
}

void
lgtd_lifx_wire_decode_runtime_info(struct lgtd_lifx_packet_runtime_info *pkt)
{
    assert(pkt);

    pkt->time = le64toh(pkt->time);
    pkt->uptime = le64toh(pkt->uptime);
    pkt->downtime = le64toh(pkt->downtime);
}

void
lgtd_lifx_wire_decode_ambient_light(struct lgtd_lifx_packet_ambient_light *pkt)
{
    assert(pkt);

    pkt->illuminance = lgtd_lifx_wire_lefloattoh(pkt->illuminance);
}
