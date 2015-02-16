// Copyright (c) 2015, Louis Opter <kalessin@kalessin.fr>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

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

#include "lifx/wire_proto.h"
#include "router.h"
#include "lightsd.h"

bool
lgtd_proto_power_on(const char *target)
{
    assert(target);

    struct lgtd_lifx_packet_power_state pkt = { .power = LGTD_LIFX_POWER_ON };
    return lgtd_router_send(target, LGTD_LIFX_SET_POWER_STATE, &pkt);
}

bool
lgtd_proto_power_off(const char *target)
{
    assert(target);

    struct lgtd_lifx_packet_power_state pkt = { .power = LGTD_LIFX_POWER_OFF };
    return lgtd_router_send(target, LGTD_LIFX_SET_POWER_STATE, &pkt);
}

bool
lgtd_proto_set_light_from_hsbk(const char *target,
                               int hue,
                               int saturation,
                               int brightness,
                               int kelvin,
                               int transition_msecs)
{
    assert(target);
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
    return lgtd_router_send(target, LGTD_LIFX_SET_LIGHT_COLOR, &pkt);
}
