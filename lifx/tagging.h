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

extern struct lgtd_lifx_tag_list lgtd_lifx_tags;

struct lgtd_lifx_site {
    LIST_ENTRY(lgtd_lifx_site)  link;
    int                         tag_id;
    struct lgtd_lifx_gateway    *gw;
};
LIST_HEAD(lgtd_lifx_site_list, lgtd_lifx_site);

struct lgtd_lifx_tag {
    LIST_ENTRY(lgtd_lifx_tag)   link;
    char                        label[LGTD_LIFX_LABEL_SIZE];
    struct lgtd_lifx_site_list  sites;
};
LIST_HEAD(lgtd_lifx_tag_list, lgtd_lifx_tag);

struct lgtd_lifx_tag *lgtd_lifx_tagging_incref(const char *,
                                               struct lgtd_lifx_gateway *,
                                               int);
void lgtd_lifx_tagging_decref(struct lgtd_lifx_tag *, struct lgtd_lifx_gateway *);

struct lgtd_lifx_tag *lgtd_lifx_tagging_find_tag(const char *);
