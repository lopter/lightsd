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

static struct lgtd_lifx_site *
lgtd_lifx_tagging_find_site(struct lgtd_lifx_site_list *sites,
                            struct lgtd_lifx_gateway *gw)
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

struct lgtd_lifx_tag *
lgtd_lifx_tagging_allocate_tag(const char *tag_label)
{
    assert(tag_label);
    assert(strlen(tag_label) < LGTD_LIFX_LABEL_SIZE);

    struct lgtd_lifx_tag *tag = calloc(1, sizeof(*tag));
    if (!tag) {
        return NULL;
    }

    strncpy(tag->label, tag_label, sizeof(tag->label) - 1);
    LIST_INSERT_HEAD(&lgtd_lifx_tags, tag, link);
    return tag;
}

void
lgtd_lifx_tagging_deallocate_tag(struct lgtd_lifx_tag *tag)
{
    assert(tag);
    assert(LIST_EMPTY(&tag->sites));

    LIST_REMOVE(tag, link);
    free(tag);
}

struct lgtd_lifx_tag *
lgtd_lifx_tagging_incref(const char *tag_label,
                         struct lgtd_lifx_gateway *gw,
                         int tag_id)
{
    assert(strlen(tag_label) < LGTD_LIFX_LABEL_SIZE);
    assert(tag_id < LGTD_LIFX_GATEWAY_MAX_TAGS);
    assert(gw);

    bool dealloc_tag = false;
    struct lgtd_lifx_tag *tag = lgtd_lifx_tagging_find_tag(tag_label);
    if (!tag) {
        tag = lgtd_lifx_tagging_allocate_tag(tag_label);
        if (!tag) {
            return NULL;
        }
        dealloc_tag = true;
    }

    struct lgtd_lifx_site *site = lgtd_lifx_tagging_find_site(&tag->sites, gw);
    if (!site) {
        site = calloc(1, sizeof(*site));
        if (!site) {
            if (dealloc_tag) {
                lgtd_lifx_tagging_deallocate_tag(tag);
            }
            errno = ENOMEM;
            return NULL;
        }
        if (dealloc_tag) {
            lgtd_info("discovered tag [%s]", tag_label);
        }
        char site_addr[LGTD_LIFX_ADDR_STRLEN];
        lgtd_info(
            "tag [%s] added to gw [%s]:%hu (site %s) with tag_id %d",
            tag_label, gw->ip_addr, gw->port,
            LGTD_IEEE8023MACTOA(gw->site.as_array, site_addr), tag_id
        );
        site->gw = gw;
        site->tag_id = tag_id;
        LIST_INSERT_HEAD(&tag->sites, site, link);
    }
    assert(site->tag_id == tag_id);

    return tag;
}

void
lgtd_lifx_tagging_decref(struct lgtd_lifx_tag *tag,
                         struct lgtd_lifx_gateway *gw)
{
    assert(tag);
    assert(gw);

    struct lgtd_lifx_site *site;
    site = lgtd_lifx_tagging_find_site(&tag->sites, gw);
    if (site) {
        char site_addr[LGTD_LIFX_ADDR_STRLEN];
        lgtd_debug(
            "tag [%s] removed from gw [%s]:%hu (site %s)",
            tag->label, gw->ip_addr, gw->port,
            LGTD_IEEE8023MACTOA(gw->site.as_array, site_addr)
        );
        LIST_REMOVE(site, link);
        free(site);
    }
    if (LIST_EMPTY(&tag->sites)) {
        lgtd_info("forgetting unused tag [%s]", tag->label);
        lgtd_lifx_tagging_deallocate_tag(tag);
    }
}
