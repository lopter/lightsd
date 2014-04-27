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

#include <assert.h>
#include <endian.h>
#include <err.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "wire_proto.h"
#include "lifxd.h"

// Convert all the fields in the header to the host endianness.
//
// :returns: The payload size or -1 if the header is invalid.
void
lifxd_wire_decode_header(struct lifxd_packet_header *hdr)
{
    assert(hdr);

    hdr->size = le16toh(hdr->size);
    hdr->protocol = be16toh(hdr->protocol);
    hdr->timestamp = be64toh(hdr->timestamp);
    hdr->packet_type = le16toh(hdr->packet_type);
}

void
lifxd_wire_encode_header(struct lifxd_packet_header *hdr)
{
    assert(hdr);

    hdr->size = htole16(hdr->size);
    hdr->protocol = htobe16(hdr->protocol);
    hdr->timestamp = htobe64(hdr->timestamp);
    hdr->packet_type = htole16(hdr->packet_type);
}

void
lifxd_wire_dump_header(const struct lifxd_packet_header *hdr)
{
    assert(hdr);
    lifxd_debug(
        "header @%p: size=%hu, protocol=%hu, timestamp=%lu, packet_type=%hx",
        hdr, hdr->size, hdr->protocol, hdr->timestamp, hdr->packet_type
    );
}

void
lifxd_wire_decode_pan_gateway(struct lifxd_packet_pan_gateway *pkt)
{
    pkt->port = le32toh(pkt->port);
}

void
lifxd_wire_encode_pan_gateway(struct lifxd_packet_pan_gateway *pkt)
{
    pkt->port = htole32(pkt->port);
}

void
lifxd_wire_decode_light_status(struct lifxd_packet_light_status *pkt)
{
    pkt->hue = le16toh(pkt->hue);
    pkt->saturation = le16toh(pkt->saturation);
    pkt->brightness = le16toh(pkt->brightness);
    pkt->kelvin = le16toh(pkt->kelvin);
    pkt->dim = le16toh(pkt->dim);
    pkt->power = le16toh(pkt->power);
    pkt->tags = be64toh(pkt->tags);
}

void
lifxd_wire_encode_light_status(struct lifxd_packet_light_status *pkt)
{
    pkt->hue = htole16(pkt->hue);
    pkt->saturation = htole16(pkt->saturation);
    pkt->brightness = htole16(pkt->brightness);
    pkt->kelvin = htole16(pkt->kelvin);
    pkt->dim = htole16(pkt->dim);
    pkt->power = htole16(pkt->power);
    pkt->tags = htobe64(pkt->tags);
}

void
lifxd_wire_decode_power_state(struct lifxd_packet_power_state *pkt)
{
    (void)pkt;
}
