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

#pragma once

// TODO: return that from the functions in there and handle it:
enum lgtd_router_error {
    LGTD_ROUTER_INVALID_TARGET_ERROR,
    LGTD_ROUTER_CANNOT_ENQUEUE_PACKET_ERROR
};

struct lgtd_router_device {
    SLIST_ENTRY(lgtd_router_device) link;
    struct lgtd_lifx_bulb           *device;
};
SLIST_HEAD(lgtd_router_device_list, lgtd_router_device);

bool lgtd_router_send(const struct lgtd_proto_target_list *, enum lgtd_lifx_packet_type, void *);
void lgtd_router_send_to_device(struct lgtd_lifx_bulb *, enum lgtd_lifx_packet_type, void *);
void lgtd_router_send_to_tag(const struct lgtd_lifx_tag *, enum lgtd_lifx_packet_type, void *);
void lgtd_router_send_to_label(const char *, enum lgtd_lifx_packet_type, void *);
void lgtd_router_broadcast(enum lgtd_lifx_packet_type, void *);
struct lgtd_router_device_list *lgtd_router_targets_to_devices(const struct lgtd_proto_target_list *);
void lgtd_router_device_list_free(struct lgtd_router_device_list *);
