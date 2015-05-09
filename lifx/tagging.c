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
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <event2/util.h>

#include "wire_proto.h"
#include "core/time_monotonic.h"
#include "bulb.h"
#include "gateway.h"
#include "tagging.h"
#include "core/lightsd.h"

struct lgtd_lifx_tag_list lgtd_lifx_tags =
    LIST_HEAD_INITIALIZER(&lgtd_lifx_tags);

static struct lgtd_lifx_tag *
lgtd_lifx_tagging_find_tag(const char *tag_label)
{
    struct lgtd_lifx_tag *tag = NULL;
    LIST_FOREACH(tag, &lgtd_lifx_tags, link) {
        if (!strcmp(tag->label, tag_label)) {
            break;
        }
    }
    return tag;
}

static struct lgtd_lifx_site *
lgtd_lifx_tagging_find_site(struct lgtd_lifx_site_list *sites,
                            const struct lgtd_lifx_gateway *gw)
{
    struct lgtd_lifx_site *site = NULL;
    LIST_FOREACH(site, sites, link) {
        if (site->gw == gw) {
            break;
        }
    }
    return site;
}

struct lgtd_lifx_tag *
lgtd_lifx_tagging_incref(const char *tag_label,
                         const struct lgtd_lifx_gateway *gw)
{
    assert(strlen(tag_label) < LGTD_LIFX_LABEL_SIZE);
    assert(gw);

    bool dealloc_tag = false;
    struct lgtd_lifx_tag *tag = lgtd_lifx_tagging_find_tag(tag_label);
    if (!tag) {
        tag = calloc(1, sizeof(*tag));
        if (!tag) {
            return NULL;
        }
        strncpy(tag->label, tag_label, sizeof(tag->label) - 1);
        LIST_INSERT_HEAD(&lgtd_lifx_tags, tag, link);
        dealloc_tag = true;
    }

    struct lgtd_lifx_site *site = lgtd_lifx_tagging_find_site(&tag->sites, gw);
    if (!site) {
        site = calloc(1, sizeof(*site));
        if (!site) {
            if (dealloc_tag) {
                LIST_REMOVE(tag, link);
                free(tag);
            }
            errno = ENOMEM;
            return NULL;
        }
        if (dealloc_tag) {
            lgtd_info("discovered tag [%s]", tag_label);
        }
        lgtd_debug(
            "tag [%s] added to gw [%s]:%hu (site %s)",
            tag_label, gw->ip_addr, gw->port, lgtd_addrtoa(gw->site.as_array)
        );
        site->gw = gw;
        LIST_INSERT_HEAD(&tag->sites, site, link);
    }

    return tag;
}

void
lgtd_lifx_tagging_decref(struct lgtd_lifx_tag *tag,
                         const struct lgtd_lifx_gateway *gw)
{
    assert(tag);
    assert(gw);

    struct lgtd_lifx_site *site;
    site = lgtd_lifx_tagging_find_site(&tag->sites, gw);
    if (site) {
        lgtd_debug(
            "tag [%s] removed from gw [%s]:%hu (site %s)",
            tag->label, gw->ip_addr, gw->port,
            lgtd_addrtoa(gw->site.as_array)
        );
        LIST_REMOVE(site, link);
        free(site);
    }
    if (LIST_EMPTY(&tag->sites)) {
        LIST_REMOVE(tag, link);
        lgtd_info("forgetting unused tag [%s]", tag->label);
        free(tag);
    }
}
