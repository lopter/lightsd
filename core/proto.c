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
#include <endian.h>
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
    lgtd_client_write_string(client, "[");
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
        if (written >= (int)sizeof(buf)) {
            lgtd_warnx(
                "can't send state of bulb %s (%s) to client "
                "[%s]:%hu: output buffer to small",
                bulb->state.label, lgtd_addrtoa(bulb->addr),
                client->ip_addr, client->port
            );
            continue;
        }
        lgtd_client_write_string(client, buf);

        bool comma = false;
        int tag_id;
        LGTD_LIFX_WIRE_FOREACH_TAG_ID(tag_id, bulb->state.tags) {
            if (LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(tag_id) & bulb->gw->tag_ids) {
                lgtd_client_write_string(client, comma ? ",\"" : "\"");
                lgtd_client_write_string(client, bulb->gw->tags[tag_id]->label);
                lgtd_client_write_string(client, "\"");
                comma = true;
            } else {
                lgtd_warnx(
                    "tag_id %d on bulb %.*s (%s) doesn't "
                    "exist on gw [%s]:%hu (site %s)",
                    tag_id, (int)sizeof(bulb->state.label), bulb->state.label,
                    lgtd_addrtoa(bulb->addr), bulb->gw->ip_addr, bulb->gw->port,
                    lgtd_addrtoa(bulb->gw->site.as_array)
                );
            }
        }

        lgtd_client_write_string(
            client, SLIST_NEXT(device, link) ?  "]}," : "]}"
        );
    }
    lgtd_client_write_string(client, "]");
    lgtd_client_end_send_response(client);

    lgtd_router_device_list_free(devices);
}

void
lgtd_proto_tag(struct lgtd_client *client,
               const struct lgtd_proto_target_list *targets,
               const char *tag_label)
{
    assert(client);
    assert(targets);
    assert(tag_label);

    struct lgtd_router_device_list *devices;
    devices = lgtd_router_targets_to_devices(targets);
    if (!devices) {
        goto error_tag_alloc;
    }

    struct lgtd_lifx_tag *tag = lgtd_lifx_tagging_find_tag(tag_label);
    if (!tag) {
        tag = lgtd_lifx_tagging_allocate_tag(tag_label);
        if (!tag) {
            goto error_tag_alloc;
        }
        lgtd_info("created tag [%s]", tag_label);
    }

    struct lgtd_router_device *device;
    struct lgtd_lifx_site *site;

    // Loop over the devices and do allocations first, this makes error
    // handling easier (since you can't rollback enqueued packets) and build
    // the list of affected gateways so we can do SET_TAG_LABELS:
    SLIST_FOREACH(device, devices, link) {
        struct lgtd_lifx_gateway *gw = device->device->gw;
        int tag_id = lgtd_lifx_gateway_get_tag_id(gw, tag);
        if (tag_id == -1) {
            tag_id = lgtd_lifx_gateway_allocate_tag_id(gw, -1, tag_label);
            if (tag_id == -1) {
                goto error_site_alloc;
            }
        }
    }

    // SET_TAG_LABELS, this is idempotent, do it everytime so we can recover
    // from any bad state:
    LIST_FOREACH(site, &tag->sites, link) {
        int tag_id = site->tag_id;
        assert(tag_id > -1 && tag_id < LGTD_LIFX_GATEWAY_MAX_TAGS);
        struct lgtd_lifx_packet_tag_labels pkt = { .tags = 0 };
        pkt.tags = LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(tag_id);
        strncpy(pkt.label, tag_label, sizeof(pkt.label) - 1);
        lgtd_lifx_wire_encode_tag_labels(&pkt);
        bool enqueued = lgtd_lifx_gateway_send_to_site(
            site->gw, LGTD_LIFX_SET_TAG_LABELS, &pkt
        );
        if (!enqueued) {
            goto error_site_alloc;
        }
        lgtd_info(
            "created tag [%s] with id %d on gw [%s]:%hu",
            tag_label, tag_id, site->gw->ip_addr, site->gw->port
        );
    }

    // Finally SET_TAGS on the devices:
    SLIST_FOREACH(device, devices, link) {
        struct lgtd_lifx_bulb *bulb = device->device;
        int tag_id = lgtd_lifx_gateway_get_tag_id(bulb->gw, tag);
        assert(tag_id > -1 && tag_id < LGTD_LIFX_GATEWAY_MAX_TAGS);
        int tag_value = LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(tag_id);
        if (!(bulb->state.tags & tag_value)) {
            struct lgtd_lifx_packet_tags pkt;
            pkt.tags = bulb->state.tags | tag_value;
            lgtd_lifx_wire_encode_tags(&pkt);
            lgtd_router_send_to_device(bulb, LGTD_LIFX_SET_TAGS, &pkt);
        }
    }

    SEND_RESULT(client, true);
    goto fini;

error_site_alloc:
    if (LIST_EMPTY(&tag->sites)) {
        lgtd_lifx_tagging_deallocate_tag(tag);
    } else { // tagging_decref will deallocate the tag for us:
        struct lgtd_lifx_site *next_site;
        LIST_FOREACH_SAFE(site, &tag->sites, link, next_site) {
            lgtd_lifx_gateway_deallocate_tag_id(site->gw, site->tag_id);
        }
    }
error_tag_alloc:
    lgtd_client_send_error(
        client, LGTD_CLIENT_INTERNAL_ERROR, "couldn't allocate new tag"
    );
fini:
    lgtd_router_device_list_free(devices);
    return;
}

void
lgtd_proto_untag(struct lgtd_client *client,
                 const struct lgtd_proto_target_list *targets,
                 const char *tag_label)
{
    assert(client);
    assert(targets);
    assert(tag_label);

    struct lgtd_lifx_tag *tag = lgtd_lifx_tagging_find_tag(tag_label);
    if (!tag) {
        SEND_RESULT(client, true);
        return;
    }

    struct lgtd_router_device_list *devices = NULL;
    devices = lgtd_router_targets_to_devices(targets);
    if (!devices) {
        lgtd_client_send_error(
            client, LGTD_CLIENT_INTERNAL_ERROR, "couldn't allocate memory"
        );
        return;
    }

    struct lgtd_router_device *device;
    SLIST_FOREACH(device, devices, link) {
        struct lgtd_lifx_bulb *bulb = device->device;
        struct lgtd_lifx_gateway *gw = bulb->gw;
        int tag_id = lgtd_lifx_gateway_get_tag_id(gw, tag);
        if (tag_id != -1) {
            int tag_value = LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(tag_id);
            if (bulb->state.tags & tag_value) {
                struct lgtd_lifx_packet_tags pkt;
                pkt.tags = bulb->state.tags & ~tag_value;
                lgtd_lifx_wire_encode_tags(&pkt);
                lgtd_router_send_to_device(bulb, LGTD_LIFX_SET_TAGS, &pkt);
            }
        }
    }

    SEND_RESULT(client, true);

    lgtd_router_device_list_free(devices);
}
