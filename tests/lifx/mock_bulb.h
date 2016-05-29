#pragma once

#ifndef MOCKED_LGTD_LIFX_BULB_GET
struct lgtd_lifx_bulb *
lgtd_lifx_bulb_get(const uint8_t *addr)
{
    (void)addr;
    return NULL;
}
#endif

#ifndef MOCKED_LGTD_LIFX_BULB_OPEN
struct lgtd_lifx_bulb *
lgtd_lifx_bulb_open(struct lgtd_lifx_gateway *gw, const uint8_t *addr)
{
    (void)gw;
    (void)addr;
    return NULL;
}
#endif

#ifndef MOCKED_LGTD_LIFX_BULB_CLOSE
void
lgtd_lifx_bulb_close(struct lgtd_lifx_bulb *bulb)
{
    (void)bulb;
}
#endif

#ifndef MOCKED_LGTD_LIFX_BULB_HAS_LABEL
bool
lgtd_lifx_bulb_has_label(const struct lgtd_lifx_bulb *bulb,
                         const char *label)
{
    (void)bulb;
    (void)label;
    return false;
}
#endif

#ifndef MOCKED_LGTD_LIFX_BULB_SET_LIGHT_STATE
void
lgtd_lifx_bulb_set_light_state(struct lgtd_lifx_bulb *bulb,
                               const struct lgtd_lifx_light_state *state,
                               lgtd_time_mono_t received_at)
{
    (void)bulb;
    (void)state;
    (void)received_at;
}
#endif

#ifndef MOCKED_LGTD_LIFX_BULB_SET_POWER_STATE
void
lgtd_lifx_bulb_set_power_state(struct lgtd_lifx_bulb *bulb, uint16_t power)
{
    (void)bulb;
    (void)power;
}
#endif

#ifndef MOCKED_LGTD_LIFX_BULB_SET_TAGS
void
lgtd_lifx_bulb_set_tags(struct lgtd_lifx_bulb *bulb, uint64_t tags)
{
    (void)bulb;
    (void)tags;
}
#endif

#ifndef MOCKED_LGTD_LIFX_BULB_SET_IP_STATE
void
lgtd_lifx_bulb_set_ip_state(struct lgtd_lifx_bulb *bulb,
                            enum lgtd_lifx_bulb_ips ip_id,
                            const struct lgtd_lifx_ip_state *state,
                            lgtd_time_mono_t received_at)
{
    (void)bulb;
    (void)ip_id;
    (void)state;
    (void)received_at;
}
#endif

#ifndef MOCKED_LGTD_LIFX_BULB_SET_IP_FIRMWARE_INFO
void
lgtd_lifx_bulb_set_ip_firmware_info(struct lgtd_lifx_bulb *bulb,
                                    enum lgtd_lifx_bulb_ips ip_id,
                                    const struct lgtd_lifx_ip_firmware_info *info,
                                    lgtd_time_mono_t received_at)
{
    (void)bulb;
    (void)ip_id;
    (void)info;
    (void)received_at;
}
#endif

#ifndef MOCKED_LGTD_LIFX_BULB_SET_PRODUCT_INFO
void
lgtd_lifx_bulb_set_product_info(struct lgtd_lifx_bulb *bulb,
                                const struct lgtd_lifx_product_info *info)
{
    (void)bulb;
    (void)info;
}
#endif

#ifndef MOCKED_LGTD_LIFX_BULB_SET_RUNTIME_INFO
void
lgtd_lifx_bulb_set_runtime_info(struct lgtd_lifx_bulb *bulb,
                                const struct lgtd_lifx_runtime_info *info,
                                lgtd_time_mono_t received_at)
{
    (void)bulb;
    (void)info;
    (void)received_at;
}
#endif

#ifndef MOCKED_LGTD_LIFX_BULB_SET_LABEL
void
lgtd_lifx_bulb_set_label(struct lgtd_lifx_bulb *bulb,
                         const char label[LGTD_LIFX_LABEL_SIZE])
{
    (void)bulb;
    (void)label;
}
#endif

#ifndef MOCKED_LGTD_LIFX_BULB_SET_AMBIENT_LIGHT
void
lgtd_lifx_bulb_set_ambient_light(struct lgtd_lifx_bulb *bulb, float illuminance)
{
    (void)bulb;
    (void)illuminance;
}
#endif
