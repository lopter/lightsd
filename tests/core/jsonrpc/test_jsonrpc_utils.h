#pragma once

#define TEST_REQUEST_INITIALIZER { NULL, NULL, 0, NULL }

static inline int
parse_json(jsmntok_t *tokens, size_t capacity, const char *json , size_t len)
{
    jsmn_parser ctx;
    jsmn_init(&ctx);
    return jsmn_parse(&ctx, json, len, tokens, capacity);
}

static char client_write_buf[4096] = { 0 };
static int client_write_buf_idx = 0;

static inline void
reset_client_write_buf(void)
{
    memset(client_write_buf, 0, sizeof(client_write_buf));
    client_write_buf_idx = 0;
}

int
bufferevent_write(struct bufferevent *bev, const void *data, size_t nbytes)
{
    (void)bev;
    int to_write = LGTD_MIN(nbytes, sizeof(client_write_buf));
    memcpy(&client_write_buf[client_write_buf_idx], data, to_write);
    client_write_buf_idx += to_write;
    return 0;
}

void lgtd_proto_target_list_clear(struct lgtd_proto_target_list *targets)
{
    assert(targets);
}

#ifndef LGTD_TESTING_SET_LIGHT_FROM_HSBK
bool
lgtd_proto_set_light_from_hsbk(const struct lgtd_proto_target_list *targets,
                               int hue,
                               int saturation,
                               int brightness,
                               int kelvin,
                               int transition_msecs)
{
    (void)targets;
    (void)hue;
    (void)saturation;
    (void)brightness;
    (void)kelvin;
    (void)transition_msecs;
    return true;
}
#endif

#ifndef LGTD_TESTING_POWER_ON
bool
lgtd_proto_power_on(const struct lgtd_proto_target_list *targets)
{
    (void)targets;
    return true;
}
#endif

#ifndef LGTD_TESTING_POWER_OFF
bool
lgtd_proto_power_off(const struct lgtd_proto_target_list *targets)
{
    (void)targets;
    return true;
}
#endif

#ifndef LGTD_TESTING_SET_WAVEFORM
bool lgtd_proto_set_waveform(const struct lgtd_proto_target_list *targets,
                             enum lgtd_lifx_waveform_type waveform,
                             int hue, int saturation,
                             int brightness, int kelvin,
                             int period, float cycles,
                             int skew_ratio, bool transient)
{
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
    return true;
}
#endif
