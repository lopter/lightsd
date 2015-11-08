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

    struct lgtd_lifx_site *site_gw1 = calloc(1, sizeof(*site_gw1));
    site_gw1->gw = &gw1;
    struct lgtd_lifx_site *site_gw2 = calloc(1, sizeof(*site_gw2));
    site_gw2->gw = &gw2;

    const char *rawr = "rawr";
    struct lgtd_lifx_tag *tag = calloc(1, sizeof(*tag));
    strcpy(tag->label, rawr);
    LIST_INSERT_HEAD(&tag->sites, site_gw1, link);
    LIST_INSERT_HEAD(&tag->sites, site_gw2, link);
    LIST_INSERT_HEAD(&lgtd_lifx_tags, tag, link);

    for (int i = 0; i != 2; i++) {
        lgtd_lifx_tagging_decref(tag, &gw2);
        if (count_site(&tag->sites, &gw2) != 0) {
            errx(1, "gw2 shouldn't be in the sites list");
        }
        if (count_site(&tag->sites, &gw1) != 1) {
            errx(1, "gw1 wasn't found once in the sites list");
        }
        if (count_tag(rawr) != 1) {
            errx(1, "%s wasn't found once in the tags list", rawr);
        }
    }

    lgtd_lifx_tagging_decref(tag, &gw1);
    if (count_tag(rawr)) {
        errx(1, "the tags list should be empty");
    }

    return 0;
}
