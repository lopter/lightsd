#pragma once

const union lgtd_lifx_target LGTD_LIFX_UNSPEC_TARGET = { .tags = 0 };

#ifndef MOCKED_LGTD_WIRE_SETUP
void
lgtd_lifx_wire_setup(void)
{
}
#endif

#ifndef MOCKED_LGTD_LIFX_WIRE_WAVEFORM_STRING_ID_TO_TYPE
enum lgtd_lifx_waveform_type
lgtd_lifx_wire_waveform_string_id_to_type(const char *s, int len)
{
    (void)s;
    (void)len;
    return LGTD_LIFX_WAVEFORM_INVALID;
}
#endif

#ifndef MOCKED_LGTD_LIFX_WIRE_NULL_PACKET_ENCODER_DECODER
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

const struct lgtd_lifx_packet_info *
lgtd_lifx_wire_get_packet_info(enum lgtd_lifx_packet_type packet_type)
{
#define UNIMPLEMENTED                                       \
    .decode = lgtd_lifx_wire_null_packet_encoder_decoder,   \
    .encode = lgtd_lifx_wire_null_packet_encoder_decoder,   \
    .handle = lgtd_lifx_wire_null_packet_handler

    static const struct lgtd_lifx_packet_info packet_table[] = {
        // Gateway packets:
        {
            UNIMPLEMENTED,
            .name = "GET_PAN_GATEWAY",
            .type = LGTD_LIFX_GET_PAN_GATEWAY
        },
        {
            UNIMPLEMENTED,
            .name = "PAN_GATEWAY",
            .type = LGTD_LIFX_PAN_GATEWAY,
            .size = sizeof(struct lgtd_lifx_packet_pan_gateway)
        },
        {
            UNIMPLEMENTED,
            .name = "SET_TAG_LABELS",
            .type = LGTD_LIFX_SET_TAG_LABELS,
            .size = sizeof(struct lgtd_lifx_packet_tag_labels),
        },
        {
            UNIMPLEMENTED,
            .name = "GET_TAG_LABELS",
            .type = LGTD_LIFX_GET_TAG_LABELS,
            .size = sizeof(struct lgtd_lifx_packet_tags),
        },
        {
            UNIMPLEMENTED,
            .name = "TAG_LABELS",
            .type = LGTD_LIFX_TAG_LABELS,
            .size = sizeof(struct lgtd_lifx_packet_tag_labels)
        },
        // Bulb packets:
        {
            UNIMPLEMENTED,
            .name = "SET_LIGHT_COLOR",
            .type = LGTD_LIFX_SET_LIGHT_COLOR,
            .size = sizeof(struct lgtd_lifx_packet_light_color)
        },
        {
            UNIMPLEMENTED,
            .name = "SET_WAVEFORM",
            .type = LGTD_LIFX_SET_WAVEFORM,
            .size = sizeof(struct lgtd_lifx_packet_waveform),
        },
        {
            UNIMPLEMENTED,
            .name = "GET_LIGHT_STATUS",
            .type = LGTD_LIFX_GET_LIGHT_STATE
        },
        {
            UNIMPLEMENTED,
            .name = "LIGHT_STATUS",
            .type = LGTD_LIFX_LIGHT_STATUS,
            .size = sizeof(struct lgtd_lifx_packet_light_status),
        },
        {
            UNIMPLEMENTED,
            .size = sizeof(struct lgtd_lifx_packet_power_state),
            .name = "SET_POWER_STATE",
            .type = LGTD_LIFX_SET_POWER_STATE,
        },
        {
            UNIMPLEMENTED,
            .name = "POWER_STATE",
            .type = LGTD_LIFX_POWER_STATE,
            .size = sizeof(struct lgtd_lifx_packet_power_state),
        },
        {
            UNIMPLEMENTED,
            .name = "SET_TAGS",
            .type = LGTD_LIFX_SET_TAGS,
            .size = sizeof(struct lgtd_lifx_packet_tags),
        },
        {
            UNIMPLEMENTED,
            .name = "TAGS",
            .type = LGTD_LIFX_TAGS,
            .size = sizeof(struct lgtd_lifx_packet_tags),
        },
        {
            UNIMPLEMENTED,
            .name = "GET_MESH_INFO",
            .type = LGTD_LIFX_GET_MESH_INFO
        },
        {
            UNIMPLEMENTED,
            .name = "MESH_INFO",
            .type = LGTD_LIFX_MESH_INFO,
            .size = sizeof(struct lgtd_lifx_packet_ip_state),
        },
        {
            UNIMPLEMENTED,
            .name = "GET_MESH_FIRMWARE",
            .type = LGTD_LIFX_GET_MESH_FIRMWARE
        },
        {
            UNIMPLEMENTED,
            .name = "MESH_FIRMWARE",
            .type = LGTD_LIFX_MESH_FIRMWARE,
            .size = sizeof(struct lgtd_lifx_packet_ip_firmware_info),
        },
        {
            UNIMPLEMENTED,
            .name = "GET_WIFI_INFO",
            .type = LGTD_LIFX_GET_WIFI_INFO,
        },
        {
            UNIMPLEMENTED,
            .name = "WIFI_INFO",
            .type = LGTD_LIFX_WIFI_INFO,
            .size = sizeof(struct lgtd_lifx_packet_ip_state),
        },
        {
            UNIMPLEMENTED,
            .name = "GET_WIFI_FIRMWARE_STATE",
            .type = LGTD_LIFX_GET_WIFI_FIRMWARE_STATE
        },
        {
            UNIMPLEMENTED,
            .name = "WIFI_FIRMWARE_STATE",
            .type = LGTD_LIFX_WIFI_FIRMWARE_STATE,
            .size = sizeof(struct lgtd_lifx_packet_ip_firmware_info),
        },
        {
            UNIMPLEMENTED,
            .name = "GET_VERSION",
            .type = LGTD_LIFX_GET_VERSION
        },
        {
            UNIMPLEMENTED,
            .name = "VERSION_STATE",
            .type = LGTD_LIFX_VERSION_STATE,
            .size = sizeof(struct lgtd_lifx_packet_product_info),
        },
        {
            UNIMPLEMENTED,
            .name = "GET_INFO",
            .type = LGTD_LIFX_GET_INFO
        },
        {
            UNIMPLEMENTED,
            .name = "INFO_STATE",
            .type = LGTD_LIFX_INFO_STATE,
            .size = sizeof(struct lgtd_lifx_packet_runtime_info),
        },
        {
            UNIMPLEMENTED,
            .name = "SET_BULB_LABEL",
            .type = LGTD_LIFX_SET_BULB_LABEL,
            .size = sizeof(struct lgtd_lifx_packet_label)
        },
        {
            UNIMPLEMENTED,
            .name = "BULB_LABEL",
            .type = LGTD_LIFX_BULB_LABEL,
            .size = sizeof(struct lgtd_lifx_packet_label),
        },
        {
            UNIMPLEMENTED,
            .name = "GET_AMBIENT_LIGHT",
            .type = LGTD_LIFX_GET_AMBIENT_LIGHT
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_AMBIENT_LIGHT",
            .type = LGTD_LIFX_STATE_AMBIENT_LIGHT,
            .size = sizeof(struct lgtd_lifx_packet_ambient_light),
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

    for (int i = 0; i != sizeof(packet_table) / sizeof(packet_table[0]); i++) {
        if (packet_table[i].type == packet_type) {
            return &packet_table[i];
        }
    }

    return NULL;
}
#endif

#ifndef MOCKED_LGTD_LIFX_WIRE_SETUP_HEADER
const struct lgtd_lifx_packet_info *
lgtd_lifx_wire_setup_header(struct lgtd_lifx_packet_header *hdr,
                            enum lgtd_lifx_target_type target_type,
                            union lgtd_lifx_target target,
                            const uint8_t *site,
                            enum lgtd_lifx_packet_type packet_type)
{
    (void)hdr;
    (void)target_type;
    (void)target;
    (void)site;
    return lgtd_lifx_wire_get_packet_info(packet_type);
}
#endif

#ifndef MOCKED_LGTD_LIFX_WIRE_DECODE_HEADER
void
lgtd_lifx_wire_decode_header(struct lgtd_lifx_packet_header *hdr)
{
    (void)hdr;
}
#endif

#ifndef MOCKED_LGTD_LIFX_WIRE_ENCODE_LIGHT_COLOR
void
lgtd_lifx_wire_encode_light_color(struct lgtd_lifx_packet_light_color *pkt)
{
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_LIFX_WIRE_ENCODE_TAG_LABELS
void
lgtd_lifx_wire_encode_tag_labels(struct lgtd_lifx_packet_tag_labels *pkt)
{
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_LIFX_WIRE_ENCODE_TAGS
void
lgtd_lifx_wire_encode_tags(struct lgtd_lifx_packet_tags *pkt)
{
    (void)pkt;
}
#endif

#ifndef MOCKED_LGTD_LIFX_WIRE_ENCODE_WAVEFORM
void
lgtd_lifx_wire_encode_waveform(struct lgtd_lifx_packet_waveform *pkt)
{
    (void)pkt;
}
#endif
