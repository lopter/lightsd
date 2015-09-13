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
lgtd_proto_power_toggle(struct lgtd_client *client,
                        const struct lgtd_proto_target_list *targets)
{
    assert(targets);

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
        struct lgtd_lifx_packet_power_state pkt = {
            .power = ~bulb->state.power
        };
        lgtd_router_send_to_device(bulb, LGTD_LIFX_SET_POWER_STATE, &pkt);
    }

    SEND_RESULT(client, true);

    lgtd_router_device_list_free(devices);
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
    assert(waveform <= LGTD_LIFX_WAVEFORM_SQUARE);
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

    char client_ip_addr[LGTD_SOCKADDR_STRLEN];
    LGTD_SOCKADDRTOA(client->addr, client_ip_addr);

    lgtd_client_start_send_response(client);
    lgtd_client_write_string(client, "[");
    struct lgtd_router_device *device;
    SLIST_FOREACH(device, devices, link) {
        struct lgtd_lifx_bulb *bulb = device->device;

        char buf[2048],
             site_addr[LGTD_LIFX_ADDR_STRLEN],
             bulb_addr[LGTD_LIFX_ADDR_STRLEN];
        int i = 0;

        LGTD_IEEE8023MACTOA(bulb->addr, bulb_addr);
        LGTD_IEEE8023MACTOA(bulb->gw->site.as_array, site_addr);

        LGTD_SNPRINTF_APPEND(
            buf, i, (int)sizeof(buf),
            "{"
                "\"_lifx\":{"
                    "\"addr\":\"%s\","
                    "\"gateway\":{"
                        "\"site\":\"%s\","
                        "\"url\":\"tcp://%s\","
                        "\"latency\":%ju"
                    "}",
            bulb_addr, site_addr, bulb->gw->peeraddr,
            (uintmax_t)lgtd_lifx_gateway_latency(bulb->gw)
        );

#define PRINT_LIFX_FW_TIMESTAMPS(fw_info, built_at_buf, installed_at_buf)       \
    LGTD_LIFX_WIRE_PRINT_NSEC_TIMESTAMP((fw_info)->built_at, (built_at_buf));   \
    LGTD_LIFX_WIRE_PRINT_NSEC_TIMESTAMP(                                        \
        (fw_info)->installed_at, (installed_at_buf)                             \
    )

        for (int ip = 0; ip != LGTD_LIFX_BULB_IP_COUNT; ip++) {
            if (lgtd_opts.verbosity == LGTD_DEBUG) {
                char fw_built_at[64], fw_installed_at[64];
                PRINT_LIFX_FW_TIMESTAMPS(
                    &bulb->ips[ip].fw_info, fw_built_at, fw_installed_at
                );

                LGTD_SNPRINTF_APPEND(
                    buf, i, (int)sizeof(buf),
                    ",\"%s\":{"
                        "\"firmware_built_at\":\"%s\","
                        "\"firmware_installed_at\":\"%s\","
                        "\"firmware_version\":\"%u.%u\","
                        "\"signal_strength\":%f,"
                        "\"tx_bytes\":%u,"
                        "\"rx_bytes\":%u,"
                        "\"temperature\":%u"
                    "}",
                    lgtd_lifx_bulb_ip_names[ip],
                    fw_built_at, fw_installed_at,
                    (bulb->ips[ip].fw_info.version & 0xffff0000) >> 16,
                    bulb->ips[ip].fw_info.version & 0xffff,
                    bulb->ips[ip].state.signal_strength,
                    bulb->ips[ip].state.tx_bytes,
                    bulb->ips[ip].state.rx_bytes,
                    bulb->ips[ip].state.temperature
                );
            } else {
                LGTD_SNPRINTF_APPEND(
                    buf, i, (int)sizeof(buf),
                    ",\"%s\":{\"firmware_version\":\"%u.%u\"}",
                    lgtd_lifx_bulb_ip_names[ip],
                    (bulb->ips[ip].fw_info.version & 0xffff0000) >> 16,
                    bulb->ips[ip].fw_info.version & 0xffff
                );
            }
        }

        if (lgtd_opts.verbosity == LGTD_DEBUG) {
            LGTD_SNPRINTF_APPEND(
                buf, i, (int)sizeof(buf),
                    ",\"product_info\":{"
                        "\"vendor_id\":\"%x\","
                        "\"product_id\":\"%x\","
                        "\"version\":%u"
                    "}",
                bulb->product_info.vendor_id,
                bulb->product_info.product_id,
                bulb->product_info.version
            );

            char bulb_time[64];
            LGTD_SNPRINTF_APPEND(
                buf, i, (int)sizeof(buf),
                    ",\"runtime_info\":{"
                        "\"time\":\"%s\","
                        "\"uptime\":%ju,"
                        "\"downtime\":%ju"
                    "}"
                "}",
                LGTD_LIFX_WIRE_PRINT_NSEC_TIMESTAMP(
                    bulb->runtime_info.time, bulb_time
                ),
                (uintmax_t)LGTD_NSECS_TO_SECS(bulb->runtime_info.uptime),
                (uintmax_t)LGTD_NSECS_TO_SECS(bulb->runtime_info.downtime)
            );
        } else {
            LGTD_SNPRINTF_APPEND(buf, i, (int)sizeof(buf), "}");
        }

#define PRINT_STRING_OR_NULL(buf, i, bufsz, v) do {                 \
    if ((v)) {                                                      \
        LGTD_SNPRINTF_APPEND((buf), (i), (bufsz), "\"%s\"", (v));   \
    } else {                                                        \
        LGTD_SNPRINTF_APPEND((buf), (i), (bufsz), "null");          \
    }                                                               \
} while (0)

    LGTD_SNPRINTF_APPEND(buf, i, (int)sizeof(buf), ",\"_model\":");
    PRINT_STRING_OR_NULL(buf, i, (int)sizeof(buf), bulb->model);
    LGTD_SNPRINTF_APPEND(buf, i, (int)sizeof(buf), ",\"_vendor\":");
    PRINT_STRING_OR_NULL(buf, i, (int)sizeof(buf), bulb->vendor);

#define PRINT_COMPONENT(src, dst, start, stop)          \
    lgtd_jsonrpc_uint16_range_to_float_string(          \
        (src), (start), (stop), (dst), sizeof((dst))    \
    )

        char h[16], s[16], b[16];
        PRINT_COMPONENT(bulb->state.hue, h, 0, 360);
        PRINT_COMPONENT(bulb->state.saturation, s, 0, 1);
        PRINT_COMPONENT(bulb->state.brightness, b, 0, 1);

        const char *label;
        int label_size;
        if (bulb->state.label[0]) {
            label = bulb->state.label;
            label_size = (int)sizeof(bulb->state.label);
        } else {
            label = bulb_addr;
            label_size = (int)sizeof(bulb_addr);
        }

        LGTD_SNPRINTF_APPEND(
            buf, i, (int)sizeof(buf),
            ",\"hsbk\":[%s,%s,%s,%hu],"
            "\"power\":%s,"
            "\"label\":\"%.*s\","
            "\"tags\":[",
            h, s, b, bulb->state.kelvin,
            bulb->state.power == LGTD_LIFX_POWER_ON ? "true" : "false",
            label_size, label
        );

        if (i >= (int)sizeof(buf)) {
            lgtd_warnx(
                "can't send state of bulb %s (%s) to client "
                "%s: output buffer to small",
                bulb->state.label, bulb_addr, client_ip_addr
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
                    "exist on gw %s (site %s)",
                    tag_id, (int)sizeof(bulb->state.label), bulb->state.label,
                    bulb_addr, bulb->gw->peeraddr, site_addr
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
            "created tag [%s] with id %d on gw %s",
            tag_label, tag_id, site->gw->peeraddr
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

void
lgtd_proto_set_label(struct lgtd_client *client,
                     const struct lgtd_proto_target_list *targets,
                     const char *label)
{
    assert(client);
    assert(targets);
    assert(label);

    int label_len = strlen(label);
    struct lgtd_lifx_packet_label pkt;
    memcpy(pkt.label, label, LGTD_MIN(label_len, (int)sizeof(pkt.label)));
    // this will go out on the network don't leave garbage in it:
    memset(&pkt.label[label_len], 0, LGTD_MAX(
        (int)sizeof(pkt.label) - label_len, 0
    ));
    SEND_RESULT(
        client, lgtd_router_send(targets, LGTD_LIFX_SET_BULB_LABEL, &pkt)
    );
}
