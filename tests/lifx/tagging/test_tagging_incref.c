#include "tagging.c"

#include "mock_log.h"

static int
count_tag(const char *tag_label)
{
    int count = 0;
    struct lgtd_lifx_tag *tag;
    LIST_FOREACH(tag, &lgtd_lifx_tags, link) {
        if (!strcmp(tag_label, tag->label)) {
            count++;
        }
    }
    return count;
}

static int
count_site(struct lgtd_lifx_site_list *list, const struct lgtd_lifx_gateway *gw)
{
    int count = 0;
    struct lgtd_lifx_site *site;
    LIST_FOREACH(site, list, link) {
        if (site->gw == gw) {
            count++;
        }
    }
    return count;
}

int
main(void)
{
    struct lgtd_lifx_gateway gw1, gw2;
    memset(&gw1, 0, sizeof(gw1));
    memset(&gw2, 0, sizeof(gw2));

    const char *rawr = "rawr";
    const char *awww = "awww";

    for (int i = 0; i != 2; i++) {
        lgtd_lifx_tagging_incref(rawr, &gw1, 1);
        if (count_tag(rawr) != 1) {
            errx(1, "%s wasn't found once the list of tags", rawr);
        }
        if (count_site(&LIST_FIRST(&lgtd_lifx_tags)->sites, &gw1) != 1) {
            errx(1, "site %p wasn't found once in the list of sites", &gw1);
        }
    }

    lgtd_lifx_tagging_incref(rawr, &gw2, 1);
    struct lgtd_lifx_tag *tag = lgtd_lifx_tagging_find_tag(rawr);
    if (count_site(&tag->sites, &gw2) != 1) {
        errx(1, "gw2 wasn't found once in the sites of tag %s", tag->label);
    }

    lgtd_lifx_tagging_incref(awww, &gw1, 1);
    if (count_tag(awww) != 1) {
        errx(1, "%s wasn't found once in the list of tags", awww);
    }
    tag = lgtd_lifx_tagging_find_tag(awww);
    if (count_site(&tag->sites, &gw1) != 1) {
        errx(1, "gw1 wasn't found once in the sites of tag %s", awww);
    }

    LIST_FOREACH(tag, &lgtd_lifx_tags, link) {
        struct lgtd_lifx_site *site;
        LIST_FOREACH(site, &tag->sites, link) {
            if (site->tag_id != 1) {
                lgtd_errx(1, "site->tag_id = %d (expected 1)", site->tag_id);
            }
        }
    }

    return 0;
}
