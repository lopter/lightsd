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

#include <event2/util.h>

#include "lifx/wire_proto.h"
#include "time_monotonic.h"
#include "lifx/bulb.h"
#include "jsmn.h"
#include "jsonrpc.h"
#include "client.h"
#include "proto.h"
#include "router.h"
#include "lightsd.h"

#define SEND_RESULT(client, ok) do {                                \
    lgtd_jsonrpc_send_response((client), (ok) ? "true" : "false");  \
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
        client, lgtd_router_send(targets, LGTD_LIFX_SET_LIGHT_COLOR, &pkt))
    ;
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
