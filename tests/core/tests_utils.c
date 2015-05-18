#include <sys/queue.h>
#include <sys/tree.h>
#include <sys/socket.h>
#include <assert.h>
#include <endian.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <event2/util.h>

#include "lifx/wire_proto.h"
#include "core/time_monotonic.h"
#include "lifx/tagging.h"
#include "core/jsmn.h"
#include "core/jsonrpc.h"
#include "core/client.h"
#include "core/proto.h"
#include "lifx/bulb.h"
#include "lifx/gateway.h"
#include "tests_utils.h"

struct lgtd_lifx_gateway_list lgtd_lifx_gateways =
    LIST_HEAD_INITIALIZER(&lgtd_lifx_gateways);

struct lgtd_lifx_tag_list lgtd_lifx_tags =
    LIST_HEAD_INITIALIZER(&lgtd_lifx_tags);

struct lgtd_lifx_gateway *
lgtd_tests_insert_mock_gateway(int id)
{
    struct lgtd_lifx_gateway *gw = calloc(1, sizeof(*gw));

    gw->socket = id;
    gw->site.as_array[0] = id;

    LIST_INSERT_HEAD(&lgtd_lifx_gateways, gw, link);

    return gw;
}

struct lgtd_lifx_bulb *
lgtd_tests_insert_mock_bulb(struct lgtd_lifx_gateway *gw, uint64_t addr)
{
    assert(gw);

    union {
        uint8_t     as_array[LGTD_LIFX_ADDR_LENGTH];
        uint64_t    as_scalar;
    } bulb_addr = { .as_scalar = htobe64(addr) >> 16 };

    return lgtd_lifx_bulb_open(gw, bulb_addr.as_array);
}

struct lgtd_proto_target_list *
lgtd_tests_build_target_list(const char *target, ...)
{
    struct lgtd_proto_target_list *targets = malloc(sizeof(*targets));
    SLIST_INIT(targets);

    struct lgtd_proto_target *tail = malloc(
        sizeof(*tail) + strlen(target) + 1
    );
    strcpy(tail->target, target);
    SLIST_INSERT_HEAD(targets, tail, link);

    va_list ap;
    va_start(ap, target);
    while ((target = va_arg(ap, const char *))) {
        struct lgtd_proto_target *t = malloc(sizeof(*t) + strlen(target) + 1);
        strcpy(t->target, target);
        SLIST_INSERT_AFTER(tail, t, link);
        tail = t;
    }
    va_end(ap);

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
    gw->tags[tag_id] = tag;
    LIST_INSERT_HEAD(&tag->sites, site, link);
    return site;
}
