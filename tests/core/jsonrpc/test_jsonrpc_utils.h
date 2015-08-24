#pragma once

#include "mock_gateway.h"

#define TEST_REQUEST_INITIALIZER { NULL, NULL, 0, NULL }

static inline int
parse_json(jsmntok_t *tokens, size_t capacity, const char *json , size_t len)
{
    jsmn_parser ctx;
    jsmn_init(&ctx);
    return jsmn_parse(&ctx, json, len, tokens, capacity);
}

void
lgtd_proto_target_list_clear(struct lgtd_proto_target_list *targets)
{
    (void)targets;
}

void
lgtd_proto_list_tags(struct lgtd_client *client)
{
    (void)client;
}

#ifndef LGTD_TESTING_SET_LIGHT_FROM_HSBK
void
lgtd_proto_set_light_from_hsbk(struct lgtd_client *client,
                               const struct lgtd_proto_target_list *targets,
                               int hue,
                               int saturation,
                               int brightness,
                               int kelvin,
                               int transition_msecs)
{
    (void)client;
    (void)targets;
    (void)hue;
    (void)saturation;
    (void)brightness;
    (void)kelvin;
    (void)transition_msecs;
}
#endif

#ifndef LGTD_TESTING_POWER_ON
void
lgtd_proto_power_on(struct lgtd_client *client,
                    const struct lgtd_proto_target_list *targets)
{
    (void)client;
    (void)targets;
}
#endif

#ifndef LGTD_TESTING_POWER_OFF
void
lgtd_proto_power_off(struct lgtd_client *client,
                     const struct lgtd_proto_target_list *targets)
{
    (void)client;
    (void)targets;
}
#endif

#ifndef LGTD_TESTING_SET_WAVEFORM
void
lgtd_proto_set_waveform(struct lgtd_client *client,
                        const struct lgtd_proto_target_list *targets,
                        enum lgtd_lifx_waveform_type waveform,
                        int hue, int saturation,
                        int brightness, int kelvin,
                        int period, float cycles,
                        int skew_ratio, bool transient)
{
    (void)client;
    (void)targets;
    (void)waveform;
    (void)hue;
    (void)saturation;
    (void)brightness;
    (void)kelvin;
    (void)period;
    (void)cycles;
    (void)skew_ratio;
    (void)transient;
}
#endif

#ifndef MOCKED_LGTD_GET_LIGHT_STATE
void
lgtd_proto_get_light_state(struct lgtd_client *client,
                           const struct lgtd_proto_target_list *targets)
{
    (void)client;
    (void)targets;
}
#endif

#ifndef MOCKED_LGTD_TAG
void
lgtd_proto_tag(struct lgtd_client *client,
               const struct lgtd_proto_target_list *targets,
               const char *tag_label)
{
    (void)client;
    (void)targets;
    (void)tag_label;
}
#endif

#ifndef MOCKED_LGTD_UNTAG
void
lgtd_proto_untag(struct lgtd_client *client,
                 const struct lgtd_proto_target_list *targets,
                 const char *tag_label)
{
    (void)client;
    (void)targets;
    (void)tag_label;
}
#endif

#ifndef MOCKED_LGTD_PROTO_POWER_TOGGLE
void
lgtd_proto_power_toggle(struct lgtd_client *client,
                        const struct lgtd_proto_target_list *targets)
{
    (void)client;
    (void)targets;
}
#endif

#ifndef MOCKED_LGTD_PROTO_SET_LABEL
void
lgtd_proto_set_label(struct lgtd_client *client,
                     const struct lgtd_proto_target_list *targets,
                     const char *label)
{
    (void)client;
    (void)targets;
    (void)label;
}
#endif
