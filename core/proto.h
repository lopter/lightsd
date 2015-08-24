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

struct lgtd_proto_target {
    SLIST_ENTRY(lgtd_proto_target)  link;
    char                            target[];
};
SLIST_HEAD(lgtd_proto_target_list, lgtd_proto_target);

void lgtd_proto_target_list_clear(struct lgtd_proto_target_list *);
const struct lgtd_proto_target *lgtd_proto_target_list_add(struct lgtd_client *,
                                                           struct lgtd_proto_target_list *,
                                                           const char *, int);

void lgtd_proto_set_light_from_hsbk(struct lgtd_client *,
                                    const struct lgtd_proto_target_list *,
                                    int, int, int, int, int);
void lgtd_proto_set_waveform(struct lgtd_client *,
                             const struct lgtd_proto_target_list *,
                             enum lgtd_lifx_waveform_type,
                             int, int, int, int,
                             int, float, int, bool);
void lgtd_proto_power_on(struct lgtd_client *, const struct lgtd_proto_target_list *);
void lgtd_proto_power_off(struct lgtd_client *, const struct lgtd_proto_target_list *);
void lgtd_proto_power_toggle(struct lgtd_client *, const struct lgtd_proto_target_list *);
void lgtd_proto_get_light_state(struct lgtd_client *, const struct lgtd_proto_target_list *);
void lgtd_proto_tag(struct lgtd_client *, const struct lgtd_proto_target_list *, const char *);
void lgtd_proto_untag(struct lgtd_client *, const struct lgtd_proto_target_list *, const char *);
void lgtd_proto_set_label(struct lgtd_client *, const struct lgtd_proto_target_list *, const char *);
