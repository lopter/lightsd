#include <sys/queue.h>
#include <sys/tree.h>
#include <sys/socket.h>
#include <assert.h>
#include <dirent.h>
#include <endian.h>
#include <err.h>
#include <limits.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <event2/util.h>

#include "lifx/wire_proto.h"
#include "core/time_monotonic.h"
#include "lifx/tagging.h"
#include "core/jsmn.h"
#include "core/jsonrpc.h"
#include "core/client.h"
#include "core/proto.h"
#include "core/listen.h"
#include "core/daemon.h"
#include "core/stats.h"
#include "lifx/bulb.h"
#include "lifx/gateway.h"
#include "tests_utils.h"

struct lgtd_listen_list lgtd_listeners =
    SLIST_HEAD_INITIALIZER(&lgtd_listeners);

struct lgtd_lifx_gateway *
lgtd_tests_insert_mock_gateway(int id)
{
    struct lgtd_lifx_gateway *gw = calloc(1, sizeof(*gw));

    gw->socket = id;
    gw->site.as_array[0] = id;

    LIST_INSERT_HEAD(&lgtd_lifx_gateways, gw, link);

    LGTD_STATS_ADD_AND_UPDATE_PROCTITLE(gateways, 1);

    return gw;
}

struct lgtd_lifx_bulb *
lgtd_tests_insert_mock_bulb(struct lgtd_lifx_gateway *gw, uint64_t addr)
{
    assert(gw);

    union {
        uint8_t     as_array[LGTD_LIFX_ADDR_LENGTH];
        uint64_t    as_scalar;
    } bulb_addr = {
        .as_scalar = LGTD_BIG_ENDIAN_SYSTEM ?
            htobe64(addr) << 16 : htobe64(addr) >> 16
    };
    struct lgtd_lifx_bulb *bulb = lgtd_lifx_bulb_open(gw, bulb_addr.as_array);

    SLIST_INSERT_HEAD(&gw->bulbs, bulb, link_by_gw);

    return bulb;
}

struct lgtd_proto_target_list *
lgtd_tests_build_target_list(const char *target, ...)
{
    struct lgtd_proto_target_list *targets = malloc(sizeof(*targets));
    SLIST_INIT(targets);

    if (target) {
        struct lgtd_proto_target *tail = malloc(
            sizeof(*tail) + strlen(target) + 1
        );
        strcpy(tail->target, target);
        SLIST_INSERT_HEAD(targets, tail, link);

        va_list ap;
        va_start(ap, target);
        while ((target = va_arg(ap, const char *))) {
            struct lgtd_proto_target *t = malloc(
                sizeof(*t) + strlen(target) + 1
            );
            strcpy(t->target, target);
            SLIST_INSERT_AFTER(tail, t, link);
            tail = t;
        }
        va_end(ap);
    }

    return targets;
}

struct lgtd_lifx_tag *
lgtd_tests_insert_mock_tag(const char *tag_label)
{
    assert(strlen(tag_label) < LGTD_LIFX_LABEL_SIZE);
    struct lgtd_lifx_tag *tag = calloc(1, sizeof(*tag));
    strcpy(tag->label, tag_label);
    LIST_INSERT_HEAD(&lgtd_lifx_tags, tag, link);
    return tag;
}

struct lgtd_lifx_site *
lgtd_tests_add_tag_to_gw(struct lgtd_lifx_tag *tag,
                         struct lgtd_lifx_gateway *gw,
                         int tag_id)
{
    struct lgtd_lifx_site *site = calloc(1, sizeof(*site));
    site->gw = gw;
    site->tag_id = tag_id;
    LIST_INSERT_HEAD(&tag->sites, site, link);

    gw->tags[tag_id] = tag;
    gw->tag_ids |= LGTD_LIFX_WIRE_TAG_ID_TO_VALUE(tag_id);

    return site;
}

struct lgtd_listen *
lgtd_tests_insert_mock_listener(const char *addr, const char *port)
{
    struct lgtd_listen *listener = calloc(1, sizeof(*listener));
    listener->addr = addr;
    listener->port = port;
    SLIST_INSERT_HEAD(&lgtd_listeners, listener, link);

    return listener;
}

char *
lgtd_tests_make_temp_dir(void)
{
    char buf[PATH_MAX] = { 0 };
    int n = snprintf(buf, sizeof(buf), "%s/lightsd.test.XXXXXXXX", P_tmpdir);
    if (n >= (int)sizeof(buf)) {
        errx(1, "cannot allocate temporary directory");
    }
    return strdup(mkdtemp(buf));
}

void
lgtd_tests_remove_temp_dir(char *path)
{
    DIR *tmpdir = opendir(path);
    if (!tmpdir) {
        return;
    }

    struct dirent db;
    struct dirent *entry = &db;
    while (!readdir_r(tmpdir, entry, &entry) && entry) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            continue;
        }
        char buf[PATH_MAX] = { 0 };
        snprintf(buf, sizeof(buf), "%s/%s", path, entry->d_name);
        unlink(buf);
    }

    closedir(tmpdir);

    if (rmdir(path)) {
        warn("couldn't remove tempdir %s", path);
    }

    free(path);
}
