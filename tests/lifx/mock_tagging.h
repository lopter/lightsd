#pragma once

#include "lifx/tagging.h"

struct lgtd_lifx_tag_list lgtd_lifx_tags =
    LIST_HEAD_INITIALIZER(&lgtd_lifx_tags);

#ifndef MOCKED_LGTD_LIFX_TAGGING_FIND_TAG
struct lgtd_lifx_tag *
lgtd_lifx_tagging_find_tag(const char *tag_label)
{
    (void)tag_label;
    return NULL;
}
#endif

#ifndef MOCKED_LGTD_LIFX_TAGGING_ALLOCATE_TAG
struct lgtd_lifx_tag *
lgtd_lifx_tagging_allocate_tag(const char *tag_label)
{
    (void)tag_label;
    return NULL;
}
#endif

#ifndef MOCKED_LGTD_LIFX_TAGGING_DEALLOCATE_TAG
void
lgtd_lifx_tagging_deallocate_tag(struct lgtd_lifx_tag *tag)
{
    (void)tag;
}
#endif

#ifndef MOCKED_LGTD_LIFX_TAGGING_INCREF
struct lgtd_lifx_tag *
lgtd_lifx_tagging_incref(const char *tag_label,
                         struct lgtd_lifx_gateway *gw,
                         int tag_id)
{
    (void)tag_label;
    (void)gw;
    (void)tag_id;
    return NULL;
}
#endif

#ifndef MOCKED_LGTD_LIFX_TAGGING_DECREF
void
lgtd_lifx_tagging_decref(struct lgtd_lifx_tag *tag,
                         struct lgtd_lifx_gateway *gw)
{
    (void)tag;
    (void)gw;
}
#endif
