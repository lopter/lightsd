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

#ifndef MOCKED_LGTD_LIFX_WIRE_ENOSYS_PACKET_HANDLER
void
lgtd_lifx_wire_enosys_packet_handler(struct lgtd_lifx_gateway *gw,
                                     const struct lgtd_lifx_packet_header *hdr,
                                     const void *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
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
            .name = "GET_DUMMY_PAYLOAD",
            .type = LGTD_LIFX_GET_DUMMY_PAYLOAD
        },
        {
            UNIMPLEMENTED,
            .name = "SET_DUMMY_PAYLOAD",
            .type = LGTD_LIFX_SET_DUMMY_PAYLOAD
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_DUMMY_PAYLOAD",
            .type = LGTD_LIFX_STATE_DUMMY_PAYLOAD
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
            .name = "STATE_FACTORY_TEST_MODE",
            .type = LGTD_LIFX_STATE_FACTORY_TEST_MODE
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_SITE",
            .type = LGTD_LIFX_STATE_SITE
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_REBOOT",
            .type = LGTD_LIFX_STATE_REBOOT
        },
        {
            UNIMPLEMENTED,
            .name = "SET_PAN_GATEWAY",
            .type = LGTD_LIFX_SET_PAN_GATEWAY
        },
        {
            UNIMPLEMENTED,
            .name = "ACK",
            .type = LGTD_LIFX_ACK
        },
        {
            UNIMPLEMENTED,
            .name = "SET_FACTORY_RESET",
            .type = LGTD_LIFX_SET_FACTORY_RESET
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_FACTORY_RESET",
            .type = LGTD_LIFX_STATE_FACTORY_RESET
        },
        {
            UNIMPLEMENTED,
            .name = "GET_LOCATION",
            .type = LGTD_LIFX_GET_LOCATION
        },
        {
            UNIMPLEMENTED,
            .name = "SET_LOCATION",
            .type = LGTD_LIFX_SET_LOCATION
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_LOCATION",
            .type = LGTD_LIFX_STATE_LOCATION
        },
        {
            UNIMPLEMENTED,
            .name = "GET_GROUP",
            .type = LGTD_LIFX_GET_GROUP
        },
        {
            UNIMPLEMENTED,
            .name = "SET_GROUP",
            .type = LGTD_LIFX_SET_GROUP
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_GROUP",
            .type = LGTD_LIFX_STATE_GROUP
        },
        {
            UNIMPLEMENTED,
            .name = "GET_OWNER",
            .type = LGTD_LIFX_GET_OWNER
        },
        {
            UNIMPLEMENTED,
            .name = "SET_OWNER",
            .type = LGTD_LIFX_SET_OWNER
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_OWNER",
            .type = LGTD_LIFX_STATE_OWNER
        },
        {
            UNIMPLEMENTED,
            .name = "GET_FACTORY_TEST_MODE",
            .type = LGTD_LIFX_GET_FACTORY_TEST_MODE
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
            .name = "SET_RGBW",
            .type = LGTD_LIFX_SET_RGBW
        },
        {
            UNIMPLEMENTED,
            .name = "GET_RAIL_VOLTAGE",
            .type = LGTD_LIFX_GET_RAIL_VOLTAGE
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_RAIL_VOLTAGE",
            .type = LGTD_LIFX_STATE_RAIL_VOLTAGE
        },
        {
            UNIMPLEMENTED,
            .name = "GET_TEMPERATURE",
            .type = LGTD_LIFX_GET_TEMPERATURE
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_TEMPERATURE",
            .type = LGTD_LIFX_STATE_TEMPERATURE
        },
        {
            UNIMPLEMENTED,
            .name = "SET_CALIBRATION_COEFFICIENTS",
            .type = LGTD_LIFX_SET_CALIBRATION_COEFFICIENTS
        },
        {
            UNIMPLEMENTED,
            .name = "SET_SIMPLE_EVENT",
            .type = LGTD_LIFX_SET_SIMPLE_EVENT
        },
        {
            UNIMPLEMENTED,
            .name = "GET_SIMPLE_EVENT",
            .type = LGTD_LIFX_GET_SIMPLE_EVENT
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_SIMPLE_EVENT",
            .type = LGTD_LIFX_STATE_SIMPLE_EVENT
        },
        {
            UNIMPLEMENTED,
            .name = "GET_POWER",
            .type = LGTD_LIFX_GET_POWER
        },
        {
            UNIMPLEMENTED,
            .name = "SET_POWER",
            .type = LGTD_LIFX_SET_POWER
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_POWER",
            .type = LGTD_LIFX_STATE_POWER
        },
        {
            UNIMPLEMENTED,
            .name = "SET_WAVEFORM_OPTIONAL",
            .type = LGTD_LIFX_SET_WAVEFORM_OPTIONAL
        },
        {
            UNIMPLEMENTED,
            .name = "CONNECT_PLAIN",
            .type = LGTD_LIFX_CONNECT_PLAIN
        },
        {
            UNIMPLEMENTED,
            .name = "CONNECT_KEY",
            .type = LGTD_LIFX_CONNECT_KEY
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_CONNECT",
            .type = LGTD_LIFX_STATE_CONNECT
        },
        {
            UNIMPLEMENTED,
            .name = "GET_AUTH_KEY",
            .type = LGTD_LIFX_GET_AUTH_KEY
        },
        {
            UNIMPLEMENTED,
            .name = "SET_AUTH_KEY",
            .type = LGTD_LIFX_SET_AUTH_KEY
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_AUTH_KEY",
            .type = LGTD_LIFX_STATE_AUTH_KEY
        },
        {
            UNIMPLEMENTED,
            .name = "SET_KEEP_ALIVE",
            .type = LGTD_LIFX_SET_KEEP_ALIVE
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_KEEP_ALIVE",
            .type = LGTD_LIFX_STATE_KEEP_ALIVE
        },
        {
            UNIMPLEMENTED,
            .name = "SET_HOST",
            .type = LGTD_LIFX_SET_HOST
        },
        {
            UNIMPLEMENTED,
            .name = "GET_HOST",
            .type = LGTD_LIFX_GET_HOST
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_HOST",
            .type = LGTD_LIFX_STATE_HOST
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
            .name = "STATE_ACCESS_POINTS",
            .type = LGTD_LIFX_STATE_ACCESS_POINTS
        },
        {
            UNIMPLEMENTED,
            .name = "GET_ACCESS_POINT",
            .type = LGTD_LIFX_GET_ACCESS_POINT
        },
        {
            UNIMPLEMENTED,
            .name = "STATE_ACCESS_POINT",
            .type = LGTD_LIFX_STATE_ACCESS_POINT
        },
        {
            UNIMPLEMENTED,
            .name = "SET_ACCESS_POINT_BROADCAST",
            .type = LGTD_LIFX_SET_ACCESS_POINT_BROADCAST
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

#ifndef MOCKED_LGTD_LIFX_WIRE_HANDLE_RECEIVE
bool
lgtd_lifx_wire_handle_receive(evutil_socket_t socket,
                              struct lgtd_lifx_gateway *gw)
{
    (void)socket;
    (void)gw;
    return false;
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
    (void)target_type;
    (void)target;
    (void)site;

    const struct lgtd_lifx_packet_info *pkt_info =
        lgtd_lifx_wire_get_packet_info(packet_type);
    hdr->size = pkt_info->size + sizeof(*hdr);
    hdr->packet_type = packet_type;
    if (site) {
        memcpy(hdr->site, site, sizeof(hdr->site));
    }

    return pkt_info;
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
