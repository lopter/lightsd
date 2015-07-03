// Copyright (c) 2015, Louis Opter <kalessin@kalessin.fr>
//
// This file is part of lighstd.
//
// lighstd is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// lighstd is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with lighstd.  If not, see <http://www.gnu.org/licenses/>.

#include <sys/queue.h>
#include <sys/tree.h>
#include <arpa/inet.h>
#include <assert.h>
#include <err.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <event2/bufferevent.h>
#include <event2/util.h>

#include "lifx/wire_proto.h"
#include "time_monotonic.h"
#include "lifx/bulb.h"
#include "lifx/tagging.h"
#include "jsmn.h"
#include "jsonrpc.h"
#include "client.h"
#include "lifx/gateway.h"
#include "proto.h"
#include "router.h"
#include "lightsd.h"

#define SEND_RESULT(client, ok) do {                                \
    lgtd_client_send_response((client), (ok) ? "true" : "false");   \
} while(0)

void
lgtd_proto_target_list_clear(struct lgtd_proto_target_list *targets)
{
    assert(targets);

    while (!SLIST_EMPTY(targets)) {
        struct lgtd_proto_target *target = SLIST_FIRST(targets);
        SLIST_REMOVE_HEAD(targets, link);
        free(target);
    }
}

void
lgtd_proto_power_on(struct lgtd_client *client,
                    const struct lgtd_proto_target_list *targets)
{
    assert(targets);

    struct lgtd_lifx_packet_power_state pkt = { .power = LGTD_LIFX_POWER_ON };
    SEND_RESULT(
        client, lgtd_router_send(targets, LGTD_LIFX_SET_POWER_STATE, &pkt)
    );
}

void
lgtd_proto_power_off(struct lgtd_client *client,
                     const struct lgtd_proto_target_list *targets)
{
    assert(targets);

    struct lgtd_lifx_packet_power_state pkt = { .power = LGTD_LIFX_POWER_OFF };
    SEND_RESULT(
        client, lgtd_router_send(targets, LGTD_LIFX_SET_POWER_STATE, &pkt)
    );
}

void
lgtd_proto_set_light_from_hsbk(struct lgtd_client *client,
                               const struct lgtd_proto_target_list *targets,
                               int hue,
                               int saturation,
                               int brightness,
                               int kelvin,
                               int transition_msecs)
{
    assert(targets);
    assert(hue >= 0 && hue <= UINT16_MAX);
    assert(saturation >= 0 && saturation <= UINT16_MAX);
    assert(brightness >= 0 && brightness <= UINT16_MAX);
    assert(kelvin >= 2500 && kelvin <= 9000);
    assert(transition_msecs >= 0);

    struct lgtd_lifx_packet_light_color pkt = {
        .stream = 0,
        .hue = hue,
        .saturation = saturation,
        .brightness = brightness,
        .kelvin = kelvin,
        .transition = transition_msecs
    };

    lgtd_lifx_wire_encode_light_color(&pkt);
    SEND_RESULT(
        client, lgtd_router_send(targets, LGTD_LIFX_SET_LIGHT_COLOR, &pkt)
    );
}

void
lgtd_proto_set_waveform(struct lgtd_client *client,
                        const struct lgtd_proto_target_list *targets,
                        enum lgtd_lifx_waveform_type waveform,
                        int hue, int saturation,
                        int brightness, int kelvin,
                        int period, float cycles,
                        int skew_ratio, bool transient)
{
    assert(targets);
    assert(hue >= 0 && hue <= UINT16_MAX);
    assert(saturation >= 0 && saturation <= UINT16_MAX);
    assert(brightness >= 0 && brightness <= UINT16_MAX);
    assert(kelvin >= 2500 && kelvin <= 9000);
    assert(waveform <= LGTD_LIFX_WAVEFORM_PULSE);
    assert(skew_ratio >= -32767 && skew_ratio <= 32768);
    assert(period >= 0);
    assert(cycles >= 0);

    struct lgtd_lifx_packet_waveform pkt = {
        .stream = 0,
        .transient = transient,
        .hue = hue,
        .saturation = saturation,
        .brightness = brightness,
        .kelvin = kelvin,
        .period = period,
        .cycles = cycles,
        .skew_ratio = skew_ratio,
        .waveform = waveform
    };

    lgtd_lifx_wire_encode_waveform(&pkt);
    SEND_RESULT(
        client, lgtd_router_send(targets, LGTD_LIFX_SET_WAVEFORM, &pkt)
    );
}

void
lgtd_proto_list_tags(struct lgtd_client *client)
{
    lgtd_client_start_send_response(client);

    LGTD_CLIENT_WRITE_STRING(client, "[");
    struct lgtd_lifx_tag *tag;
    LIST_FOREACH(tag, &lgtd_lifx_tags, link) {
        LGTD_CLIENT_WRITE_STRING(client, "\"");
        LGTD_CLIENT_WRITE_STRING(client, tag->label);
        LGTD_CLIENT_WRITE_STRING(client, LIST_NEXT(tag, link) ? "\"," : "\"");
    }
    LGTD_CLIENT_WRITE_STRING(client, "]");

    lgtd_client_end_send_response(client);
}

void
lgtd_proto_get_light_state(struct lgtd_client *client,
                           const struct lgtd_proto_target_list *targets)
{
    assert(targets);

    struct lgtd_router_device_list *devices;
    devices = lgtd_router_targets_to_devices(targets);
    if (!devices) {
        lgtd_client_send_error(
            client, LGTD_CLIENT_INTERNAL_ERROR, "couldn't allocate device list"
        );
        return;
    }

    static const char *state_fmt = ("{"
        "\"hsbk\":[%s,%s,%s,%hu],"
        "\"power\":%s,"
        "\"label\":\"%s\","
        "\"tags\":[");

#define PRINT_COMPONENT(src, dst, start, stop)          \
    lgtd_jsonrpc_uint16_range_to_float_string(          \
        (src), (start), (stop), (dst), sizeof((dst))    \
    )

    lgtd_client_start_send_response(client);
    LGTD_CLIENT_WRITE_STRING(client, "[");
    struct lgtd_router_device *device;
    SLIST_FOREACH(device, devices, link) {
        struct lgtd_lifx_bulb *bulb = device->device;

        char h[16], s[16], b[16];
        PRINT_COMPONENT(bulb->state.hue, h, 0, 360);
        PRINT_COMPONENT(bulb->state.saturation, s, 0, 1);
        PRINT_COMPONENT(bulb->state.brightness, b, 0, 1);

        char buf[3072];
        int written = snprintf(
            buf, sizeof(buf), state_fmt,
            h, s, b, bulb->state.kelvin,
            bulb->state.power == LGTD_LIFX_POWER_ON ? "true" : "false",
            bulb->state.label
        );
        if (written == sizeof(buf)) {
            lgtd_warnx(
                "can't send state of bulb %s (%s) to client "
                "[%s]:%hu: output buffer to small",
                bulb->state.label, lgtd_addrtoa(bulb->addr),
                client->ip_addr, client->port
            );
            continue;
        }
        LGTD_CLIENT_WRITE_STRING(client, buf);

        bool comma = false;
        int tag_id;
        LGTD_LIFX_WIRE_FOREACH_TAG_ID(tag_id, bulb->state.tags) {
            LGTD_CLIENT_WRITE_STRING(client, comma ? ",\"" : "\"");
            LGTD_CLIENT_WRITE_STRING(client, bulb->gw->tags[tag_id]->label);
            LGTD_CLIENT_WRITE_STRING(client, "\"");
            comma = true;
        }

        LGTD_CLIENT_WRITE_STRING(
            client, SLIST_NEXT(device, link) ?  "]}," : "]}"
        );
    }
    LGTD_CLIENT_WRITE_STRING(client, "]");
    lgtd_client_end_send_response(client);

    lgtd_router_device_list_free(devices);
}
