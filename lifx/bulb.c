// Copyright (c) 2014, Louis Opter <kalessin@kalessin.fr>
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
#include <assert.h>
#include <endian.h>
#include <err.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <event2/event.h>
#include <event2/util.h>

#include "wire_proto.h"
#include "core/time_monotonic.h"
#include "bulb.h"
#include "gateway.h"
#include "core/daemon.h"
#include "core/timer.h"
#include "core/stats.h"
#include "core/jsmn.h"
#include "core/jsonrpc.h"
#include "core/client.h"
#include "core/proto.h"
#include "core/router.h"
#include "core/lightsd.h"

struct lgtd_lifx_bulb_map lgtd_lifx_bulbs_table =
    RB_INITIALIZER(&lgtd_lifx_bulbs_table);

const char * const lgtd_lifx_bulb_ip_names[] = { "mcu", "wifi" };

static const char *
lgtd_lifx_bulb_get_model_name(uint32_t vendor_id, uint32_t product_id)
{
    if (vendor_id != LGTD_LIFX_VENDOR_ID) {
        return "Unknown";
    }

    switch (product_id) {
    case 0x1:
    case 0x2:
        return "A21 (Original)";
    case 0x3:
        return "GU10 (Color 650)";
    case 0xa:
        return "A19 (White 800)";
    default:
        return "Unknown";
    }
}

static const char *
lgtd_lifx_bulb_get_vendor_name(uint32_t vendor_id)
{
    switch (vendor_id) {
    case LGTD_LIFX_VENDOR_ID:
        return "LIFX";
    default:
        return "Unknown";
    }
}

static void
lgtd_lifx_bulb_fetch_hardware_info(struct lgtd_timer *timer,
                                   union lgtd_timer_ctx ctx)
{
    assert(timer);
    assert(ctx.as_uint);

    // Get the bulb again, it might have been closed while we were waiting:
    const uint8_t *bulb_addr = (const uint8_t *)&ctx.as_uint;
    struct lgtd_lifx_bulb *bulb = lgtd_lifx_bulb_get(bulb_addr);
    if (!bulb) {
        lgtd_timer_stop(timer);
        return;
    }

#define RESEND_IF(test, pkt_type) do {                      \
    if ((test)) {                                           \
        stop = false;                                       \
        lgtd_router_send_to_device(bulb, (pkt_type), NULL); \
    }                                                       \
} while (0)

    bool stop = true;
    RESEND_IF(!bulb->product_info.vendor_id, LGTD_LIFX_GET_VERSION);
    RESEND_IF(
        !bulb->ips[LGTD_LIFX_BULB_MCU_IP].fw_info.version,
        LGTD_LIFX_GET_MESH_FIRMWARE
    );
    lgtd_time_mono_t state_updated_at;
    state_updated_at = bulb->ips[LGTD_LIFX_BULB_MCU_IP].state_updated_at;
    lgtd_time_mono_t timeout = LGTD_LIFX_BULB_FETCH_WIFI_FW_INFO_TIMEOUT_MSECS;
    RESEND_IF(
        (
            !bulb->ips[LGTD_LIFX_BULB_WIFI_IP].fw_info.version
            && (lgtd_time_monotonic_msecs() - state_updated_at < timeout)
        ),
        LGTD_LIFX_GET_WIFI_FIRMWARE_STATE
    );
    if (stop) {
        lgtd_timer_stop(timer);
    }
}

struct lgtd_lifx_bulb *
lgtd_lifx_bulb_get(const uint8_t *addr)
{
    assert(addr);

    struct lgtd_lifx_bulb bulb;
    memcpy(bulb.addr, addr, sizeof(bulb.addr));
    return RB_FIND(lgtd_lifx_bulb_map, &lgtd_lifx_bulbs_table, &bulb);
}

struct lgtd_lifx_bulb *
lgtd_lifx_bulb_open(struct lgtd_lifx_gateway *gw, const uint8_t *addr)
{
    assert(gw);
    assert(addr);

    struct lgtd_lifx_bulb *bulb = calloc(1, sizeof(*bulb));
    if (!bulb) {
        lgtd_warn("can't allocate a new bulb");
        return NULL;
    }

    bulb->gw = gw;
    memcpy(bulb->addr, addr, sizeof(bulb->addr));
    RB_INSERT(lgtd_lifx_bulb_map, &lgtd_lifx_bulbs_table, bulb);
    LGTD_STATS_ADD_AND_UPDATE_PROCTITLE(bulbs, 1);

    bulb->last_light_state_at = lgtd_time_monotonic_msecs();

    union lgtd_timer_ctx ctx = { .as_uint = 0 };
    memcpy(&ctx.as_uint, addr, LGTD_LIFX_ADDR_LENGTH);
    struct lgtd_timer *timer = lgtd_timer_start(
        LGTD_TIMER_ACTIVATE_NOW|LGTD_TIMER_PERSISTENT,
        LGTD_LIFX_BULB_FETCH_HARDWARE_INFO_TIMER_MSECS,
        lgtd_lifx_bulb_fetch_hardware_info,
        ctx
    );
    if (!timer) {
        lgtd_warn("can't start a timer to fetch the bulb hardware specs");
    }

    return bulb;
}

void
lgtd_lifx_bulb_close(struct lgtd_lifx_bulb *bulb)
{
    assert(bulb);
    assert(bulb->gw);

#ifndef NDEBUG
    // FIXME: Yeah, so an unit test lgtd_lifx_gateway_remove_and_close_bulb
    // would be better because it can be automated, but this looks so much
    // easier to do and this code path is often exercised:
    int tag_id;
    LGTD_LIFX_WIRE_FOREACH_TAG_ID(tag_id, bulb->state.tags) {
        int n = 0;
        struct lgtd_lifx_bulb *gw_bulb;
        SLIST_FOREACH(gw_bulb, &bulb->gw->bulbs, link_by_gw) {
            assert(gw_bulb != bulb);
            if (LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(tag_id) & gw_bulb->state.tags) {
                n++;
            }
        }
        assert(bulb->gw->tag_refcounts[tag_id] == n);
    }
#endif

    LGTD_STATS_ADD_AND_UPDATE_PROCTITLE(bulbs, -1);
    if (bulb->state.power == LGTD_LIFX_POWER_ON) {
        LGTD_STATS_ADD_AND_UPDATE_PROCTITLE(bulbs_powered_on, -1);
    }
    RB_REMOVE(lgtd_lifx_bulb_map, &lgtd_lifx_bulbs_table, bulb);
    char addr[LGTD_LIFX_ADDR_STRLEN];
    lgtd_info(
        "closed bulb \"%.*s\" (%s) on [%s]:%hu",
        LGTD_LIFX_LABEL_SIZE,
        bulb->state.label,
        LGTD_IEEE8023MACTOA(bulb->addr, addr),
        bulb->gw->ip_addr,
        bulb->gw->port
    );
    free(bulb);
}

void
lgtd_lifx_bulb_set_light_state(struct lgtd_lifx_bulb *bulb,
                               const struct lgtd_lifx_light_state *state,
                               lgtd_time_mono_t received_at)
{
    assert(bulb);
    assert(state);

    if (state->power != bulb->state.power) {
        LGTD_STATS_ADD_AND_UPDATE_PROCTITLE(
            bulbs_powered_on, state->power == LGTD_LIFX_POWER_ON ? 1 : -1
        );
        lgtd_router_send_to_device(bulb, LGTD_LIFX_GET_INFO, NULL);
    }

    lgtd_lifx_gateway_update_tag_refcounts(bulb->gw, bulb->state.tags, state->tags);

    bulb->last_light_state_at = received_at;
    memcpy(&bulb->state, state, sizeof(bulb->state));
}

void
lgtd_lifx_bulb_set_power_state(struct lgtd_lifx_bulb *bulb, uint16_t power)
{
    assert(bulb);

    if (power != bulb->state.power) {
        LGTD_STATS_ADD_AND_UPDATE_PROCTITLE(
            bulbs_powered_on, power == LGTD_LIFX_POWER_ON ? 1 : -1
        );
        lgtd_router_send_to_device(bulb, LGTD_LIFX_GET_INFO, NULL);
    }

    bulb->state.power = power;
}

void
lgtd_lifx_bulb_set_tags(struct lgtd_lifx_bulb *bulb, uint64_t tags)
{
    assert(bulb);

    lgtd_lifx_gateway_update_tag_refcounts(bulb->gw, bulb->state.tags, tags);

    bulb->state.tags = tags;
}

void
lgtd_lifx_bulb_set_ip_state(struct lgtd_lifx_bulb *bulb,
                      enum lgtd_lifx_bulb_ips ip_id,
                      const struct lgtd_lifx_ip_state *state,
                      lgtd_time_mono_t received_at)
{
    assert(bulb);
    assert(state);

    struct lgtd_lifx_bulb_ip *ip = &bulb->ips[ip_id];
    ip->state_updated_at = received_at;
    memcpy(&ip->state, state, sizeof(ip->state));
}

void
lgtd_lifx_bulb_set_ip_firmware_info(struct lgtd_lifx_bulb *bulb,
                                    enum lgtd_lifx_bulb_ips ip_id,
                                    const struct lgtd_lifx_ip_firmware_info *info,
                                    lgtd_time_mono_t received_at)
{
    assert(bulb);
    assert(info);

    struct lgtd_lifx_bulb_ip *ip = &bulb->ips[ip_id];
    ip->fw_info_updated_at = received_at;
    memcpy(&ip->fw_info, info, sizeof(ip->fw_info));
}

void
lgtd_lifx_bulb_set_product_info(struct lgtd_lifx_bulb *bulb,
                                const struct lgtd_lifx_product_info *info)
{
    assert(bulb);
    assert(info);

    memcpy(&bulb->product_info, info, sizeof(bulb->product_info));
    bulb->vendor = lgtd_lifx_bulb_get_vendor_name(info->vendor_id);
    bulb->model = lgtd_lifx_bulb_get_model_name(
        info->vendor_id, info->product_id
    );
}

void
lgtd_lifx_bulb_set_runtime_info(struct lgtd_lifx_bulb *bulb,
                                const struct lgtd_lifx_runtime_info *info,
                                lgtd_time_mono_t received_at)
{
    assert(bulb);
    assert(info);

    bulb->runtime_info_updated_at = received_at;
    memcpy(&bulb->runtime_info, info, sizeof(bulb->runtime_info));
}

void
lgtd_lifx_bulb_set_label(struct lgtd_lifx_bulb *bulb,
                         const char label[LGTD_LIFX_LABEL_SIZE])
{
    assert(bulb);

    memcpy(bulb->state.label, label, LGTD_LIFX_LABEL_SIZE);
}
