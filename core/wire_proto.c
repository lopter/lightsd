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

#include <sys/queue.h>
#include <sys/tree.h>
#include <arpa/inet.h>
#include <assert.h>
#include <endian.h>
#include <err.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <event2/util.h>

#include "wire_proto.h"
#include "time_monotonic.h"
#include "bulb.h"
#include "gateway.h"
#include "lifxd.h"

union lifxd_target LIFXD_UNSPEC_TARGET = { .tags = 0 };

static struct lifxd_packet_infos_map lifxd_packet_infos =
    RB_INITIALIZER(&lifxd_packets_infos);

RB_GENERATE_STATIC(
    lifxd_packet_infos_map,
    lifxd_packet_infos,
    link,
    lifxd_packet_infos_cmp
);

static void
lifxd_wire_null_packet_encoder_decoder(void *pkt)
{
    (void)pkt;
}

static void
lifxd_wire_null_packet_handler(struct lifxd_gateway *gw,
                               const struct lifxd_packet_header *hdr,
                               const void *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}

void
lifxd_wire_load_packet_infos_map(void)
{
#define DECODER(x)  ((void (*)(void *))(x))
#define ENCODER(x)  ((void (*)(void *))(x))
#define HANDLER(x)                                  \
    ((void (*)(struct lifxd_gateway *,              \
               const struct lifxd_packet_header *,  \
               const void *))(x))
#define REQUEST_ONLY                                    \
    .decode = lifxd_wire_null_packet_encoder_decoder,   \
    .encode = lifxd_wire_null_packet_encoder_decoder,   \
    .handle = lifxd_wire_null_packet_handler

    static struct lifxd_packet_infos packet_table[] = {
        {
            REQUEST_ONLY,
            .name = "GET_PAN_GATEWAY",
            .type = LIFXD_GET_PAN_GATEWAY
        },
        {
            .name = "PAN_GATEWAY",
            .type = LIFXD_PAN_GATEWAY,
            .size = sizeof(struct lifxd_packet_pan_gateway),
            .decode = DECODER(lifxd_wire_decode_pan_gateway),
            .encode = ENCODER(lifxd_wire_encode_pan_gateway),
            .handle = HANDLER(lifxd_gateway_handle_pan_gateway)
        },
        {
            REQUEST_ONLY,
            .name = "GET_LIGHT_STATUS",
            .type = LIFXD_GET_LIGHT_STATE
        },
        {
            .name = "LIGHT_STATUS",
            .type = LIFXD_LIGHT_STATUS,
            .size = sizeof(struct lifxd_packet_light_status),
            .decode = DECODER(lifxd_wire_decode_light_status),
            .encode = ENCODER(lifxd_wire_encode_light_status),
            .handle = HANDLER(lifxd_gateway_handle_light_status)
        },
        {
            .name = "POWER_STATE",
            .type = LIFXD_POWER_STATE,
            .size = sizeof(struct lifxd_packet_power_state),
            .decode = DECODER(lifxd_wire_decode_power_state),
            .handle = HANDLER(lifxd_gateway_handle_power_state)
        }
    };

    for (int i = 0; i != LIFXD_ARRAY_SIZE(packet_table); ++i) {
        RB_INSERT(
            lifxd_packet_infos_map, &lifxd_packet_infos, &packet_table[i]
        );
    }
}

const struct lifxd_packet_infos *
lifxd_wire_get_packet_infos(enum lifxd_packet_type packet_type)
{
    struct lifxd_packet_infos pkt_infos = { .type = packet_type };
    return RB_FIND(lifxd_packet_infos_map, &lifxd_packet_infos, &pkt_infos);
}

// Convert all the fields in the header to the host endianness.
//
// \return The payload size or -1 if the header is invalid.
void
lifxd_wire_decode_header(struct lifxd_packet_header *hdr)
{
    assert(hdr);

    hdr->size = le16toh(hdr->size);
    hdr->protocol.version = le16toh(hdr->protocol.version);
    if (hdr->protocol.tagged) {
        le64toh(hdr->target.tags);
    }
    hdr->timestamp = le64toh(hdr->timestamp);
    hdr->packet_type = le16toh(hdr->packet_type);
}

const struct lifxd_packet_infos *
lifxd_wire_setup_header(struct lifxd_packet_header *hdr,
                        enum lifxd_target_type target_type,
                        union lifxd_target target,
                        const uint8_t *site,
                        enum lifxd_packet_type packet_type)
{
    assert(hdr);

    const struct lifxd_packet_infos *pkt_infos = lifxd_wire_get_packet_infos(
        packet_type
    );

    memset(hdr, 0, sizeof(*hdr));
    hdr->size = pkt_infos->size + sizeof(*hdr);
    hdr->protocol.version = LIFXD_LIFX_PROTOCOL_V1;
    hdr->packet_type = packet_type;
    if (site) {
        memcpy(hdr->site, site, sizeof(hdr->site));
    } else {
        assert(target_type == LIFXD_TARGET_ALL_DEVICES);
    }

    switch (target_type) {
    case LIFXD_TARGET_SITE:
        hdr->protocol.tagged = true;
        break;
    case LIFXD_TARGET_TAGS:
        hdr->protocol.tagged = true;
        hdr->target.tags = target.tags;
        break;
    case LIFXD_TARGET_DEVICE:
        hdr->protocol.addressable = false;
        memcpy(hdr->target.device_addr, target.addr, LIFXD_ADDR_LENGTH);
        break;
    case LIFXD_TARGET_ALL_DEVICES:
        hdr->protocol.tagged = true;
        break;
    }

    lifxd_wire_encode_header(hdr);

    return pkt_infos;
}

void
lifxd_wire_encode_header(struct lifxd_packet_header *hdr)
{
    assert(hdr);

    hdr->size = htole16(hdr->size);
    hdr->protocol.version = htole16(hdr->protocol.version);
    if (hdr->protocol.tagged) {
        le64toh(hdr->target.tags);
    }
    hdr->timestamp = htole64(hdr->timestamp);
    hdr->packet_type = htole16(hdr->packet_type);
}

void
lifxd_wire_decode_pan_gateway(struct lifxd_packet_pan_gateway *pkt)
{
    assert(pkt);

    pkt->port = le32toh(pkt->port);
}

void
lifxd_wire_encode_pan_gateway(struct lifxd_packet_pan_gateway *pkt)
{
    assert(pkt);

    pkt->port = htole32(pkt->port);
}

void
lifxd_wire_decode_light_status(struct lifxd_packet_light_status *pkt)
{
    assert(pkt);

    pkt->hue = le16toh(pkt->hue);
    pkt->saturation = le16toh(pkt->saturation);
    pkt->brightness = le16toh(pkt->brightness);
    pkt->kelvin = le16toh(pkt->kelvin);
    pkt->dim = le16toh(pkt->dim);
    pkt->power = le16toh(pkt->power);
    pkt->tags = le64toh(pkt->tags);
}

void
lifxd_wire_encode_light_status(struct lifxd_packet_light_status *pkt)
{
    assert(pkt);

    pkt->hue = htole16(pkt->hue);
    pkt->saturation = htole16(pkt->saturation);
    pkt->brightness = htole16(pkt->brightness);
    pkt->kelvin = htole16(pkt->kelvin);
    pkt->dim = htole16(pkt->dim);
    pkt->power = htole16(pkt->power);
    pkt->tags = htole64(pkt->tags);
}

void
lifxd_wire_decode_power_state(struct lifxd_packet_power_state *pkt)
{
    assert(pkt);
}
