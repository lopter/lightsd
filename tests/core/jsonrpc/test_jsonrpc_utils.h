#pragma once

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
    assert(targets);
}

void
lgtd_proto_list_tags(struct lgtd_client *client)
{
    assert(client);
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
