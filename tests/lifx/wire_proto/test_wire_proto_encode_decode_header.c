#include <string.h>

#include "wire_proto.c"

#include "mock_daemon.h"
#include "mock_gateway.h"
#include "mock_log.h"

int
main(void)
{
    struct lgtd_lifx_packet_header hdr = {
        .size = 42,
        .target = { .tags = 0xbad },
        .packet_type = LGTD_LIFX_ECHO_REQUEST
    };
    lgtd_lifx_wire_encode_header(&hdr, LGTD_LIFX_ADDRESSABLE|LGTD_LIFX_TAGGED);

    if (htobe16(hdr.protocol) != 0x34) {
        lgtd_errx(1, "protocol = %#hx (expected = 0x34)", hdr.protocol);
    }
    if (le16toh(hdr.size) != 42) {
        lgtd_errx(1, "size = %hu (expected = 42)", le16toh(hdr.size));
    }
    if (le64toh(hdr.target.tags) != 0xbad) {
        lgtd_errx(
            1, "tags = %#jx (expected = 0xbad)",
            (uintmax_t)le64toh(hdr.target.tags)
        );
    }
    if (le16toh(hdr.packet_type) != LGTD_LIFX_ECHO_REQUEST) {
        lgtd_errx(
            1, "packet_type = %hx (expected = %#x)",
            le16toh(hdr.packet_type), LGTD_LIFX_ECHO_REQUEST
        );
    }

    lgtd_lifx_wire_decode_header(&hdr);

    int proto_version = hdr.protocol & LGTD_LIFX_PROTOCOL_VERSION_MASK;
    if (proto_version !=  LGTD_LIFX_PROTOCOL_V1) {
        lgtd_errx(
            1, "protocol version = %d (expected %d)",
            proto_version, LGTD_LIFX_PROTOCOL_V1
        );
    }
    if (!(hdr.protocol & LGTD_LIFX_PROTOCOL_ADDRESSABLE)) {
        lgtd_errx(1, "the protocol addressable bit should be set");
    }
    if (!(hdr.protocol & LGTD_LIFX_PROTOCOL_TAGGED)) {
        lgtd_errx(1, "the protocol tagged bit should be set");
    }
    if (hdr.size != 42) {
        lgtd_errx(1, "size = %hu (expected = 42)", hdr.size);
    }
    if (hdr.target.tags != 0xbad) {
        lgtd_errx(
            1, "tags = %#jx (expected = 0xbad)", (uintmax_t)hdr.target.tags
        );
    }
    if (hdr.packet_type != LGTD_LIFX_ECHO_REQUEST) {
        lgtd_errx(
            1, "packet_type = %hx (expected = %#x)",
            hdr.packet_type, LGTD_LIFX_ECHO_REQUEST
        );
    }

    memset(&hdr, 0, sizeof(hdr));
    hdr.size = 42;
    hdr.target.device_addr[2] = 44;
    hdr.packet_type = LGTD_LIFX_ECHO_REQUEST;
    lgtd_lifx_wire_encode_header(&hdr, LGTD_LIFX_ADDRESSABLE);

    if (htobe16(hdr.protocol) != 0x14) {
        lgtd_errx(1, "protocol = %#hx (expected = 0x14)", hdr.protocol);
    }
    if (le16toh(hdr.size) != 42) {
        lgtd_errx(1, "size = %hu (expected = 42)", le16toh(hdr.size));
    }
    uint8_t expected_addr[LGTD_LIFX_ADDR_LENGTH] = {
        0, 0, 44, 0, 0, 0
    };
    char expected_addr_buf[LGTD_LIFX_ADDR_STRLEN];
    char dev_addr[LGTD_LIFX_ADDR_STRLEN];
    if (memcmp(hdr.target.device_addr, expected_addr, LGTD_LIFX_ADDR_LENGTH)) {
        lgtd_errx(
            1, "device addr = %s (expected = %s)",
            LGTD_IEEE8023MACTOA(hdr.target.device_addr, dev_addr),
            LGTD_IEEE8023MACTOA(expected_addr, expected_addr_buf)
        );
    }
    if (le16toh(hdr.packet_type) != LGTD_LIFX_ECHO_REQUEST) {
        lgtd_errx(
            1, "packet_type = %#hx (expected = %#x)",
            le16toh(hdr.packet_type), LGTD_LIFX_ECHO_REQUEST
        );
    }

    lgtd_lifx_wire_decode_header(&hdr);

    proto_version = hdr.protocol & LGTD_LIFX_PROTOCOL_VERSION_MASK;
    if (proto_version !=  LGTD_LIFX_PROTOCOL_V1) {
        lgtd_errx(
            1, "protocol version = %d (expected %d)",
            proto_version, LGTD_LIFX_PROTOCOL_V1
        );
    }
    if (!(hdr.protocol & LGTD_LIFX_PROTOCOL_ADDRESSABLE)) {
        lgtd_errx(1, "the protocol addressable bit should be set");
    }
    if (hdr.protocol & LGTD_LIFX_PROTOCOL_TAGGED) {
        lgtd_errx(1, "the protocol tagged bit should not be set");
    }
    if (memcmp(hdr.target.device_addr, expected_addr, LGTD_LIFX_ADDR_LENGTH)) {
        lgtd_errx(
            1, "device addr = %s (expected = %s)",
            LGTD_IEEE8023MACTOA(hdr.target.device_addr, dev_addr),
            LGTD_IEEE8023MACTOA(expected_addr, expected_addr_buf)
        );
    }
    if (hdr.size != 42) {
        lgtd_errx(1, "size = %hu (expected = 42)", hdr.size);
    }
    if (hdr.packet_type != LGTD_LIFX_ECHO_REQUEST) {
        lgtd_errx(
            1, "packet_type = %#hx (expected = %#x)",
            hdr.packet_type, LGTD_LIFX_ECHO_REQUEST
        );
    }

    return 0;
}
