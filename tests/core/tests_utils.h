#pragma once

static inline bool
lgtd_tests_lifx_header_has_flags(const struct lgtd_lifx_packet_header *hdr,
                                 int flags)
{
    int expected_protocol_flags = 0;
    if (flags & LGTD_LIFX_ADDRESSABLE) {
        expected_protocol_flags |= LGTD_LIFX_PROTOCOL_ADDRESSABLE;
    }
    if (flags & LGTD_LIFX_TAGGED) {
        expected_protocol_flags |= LGTD_LIFX_PROTOCOL_TAGGED;
    }
    int protocol_flags = hdr->protocol & LGTD_LIFX_PROTOCOL_FLAGS_MASK;
    if (protocol_flags != expected_protocol_flags) {
        return false;
    }

    int expected_flags = 0;
    if (flags & LGTD_LIFX_ACK_REQUIRED) {
        expected_flags |= LGTD_LIFX_FLAG_ACK_REQUIRED;
    }
    if (flags & LGTD_LIFX_RES_REQUIRED) {
        expected_flags |= LGTD_LIFX_FLAG_ACK_REQUIRED;
    }
    if (hdr->flags != expected_flags) {
        return false;
    }

    return true;
}

struct lgtd_lifx_gateway *lgtd_tests_insert_mock_gateway(int);
struct lgtd_lifx_bulb *lgtd_tests_insert_mock_bulb(struct lgtd_lifx_gateway *, uint64_t);
struct lgtd_proto_target_list *lgtd_tests_build_target_list(const char *, ...);
struct lgtd_lifx_tag *lgtd_tests_insert_mock_tag(const char *);
struct lgtd_lifx_site *lgtd_tests_add_tag_to_gw(struct lgtd_lifx_tag *,
                                                struct lgtd_lifx_gateway *,
                                                int);
