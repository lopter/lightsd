#pragma once

static char gw_write_buf[4096] = { 0 };
static int gw_write_buf_idx = 0;

static inline void
reset_gw_write_buf(void)
{
    memset(gw_write_buf, 0, sizeof(gw_write_buf));
    gw_write_buf_idx = 0;
}

int
evbuffer_add(struct evbuffer *buf, const void *data, size_t datlen)
{
    (void)buf;
    int to_write = LGTD_MIN(
        datlen, sizeof(gw_write_buf) - gw_write_buf_idx
    );
    memcpy(&gw_write_buf[gw_write_buf_idx], data, to_write);
    gw_write_buf_idx += to_write;
    return 0;
}

struct lgtd_lifx_tag_list lgtd_lifx_tags = LIST_HEAD_INITIALIZER(&lgtd_lifx_tags);

#ifndef MOCKED_EVBUFFER_GET_LENGTH
size_t
evbuffer_get_length(const struct evbuffer *buf)
{
    (void)buf;
    return gw_write_buf_idx + 1;
}
#endif

#ifndef MOCKED_EVBUFFER_WRITE_ATMOST
int
evbuffer_write_atmost(struct evbuffer *buf,
                      evutil_socket_t fd,
                      ev_ssize_t howmuch)
{
    (void)buf;
    (void)fd;
    (void)howmuch;
    return howmuch;
}
#endif

#ifndef MOCKED_LIFX_TAGGING_INCREF
struct lgtd_lifx_tag *
lgtd_lifx_tagging_incref(const char *label, struct lgtd_lifx_gateway *gw)
{
    struct lgtd_lifx_tag *tag = calloc(1, sizeof(*tag));
    strcpy(tag->label, label);
    LIST_INSERT_HEAD(&lgtd_lifx_tags, tag, link);
    struct lgtd_lifx_site *site = calloc(1, sizeof(*site));
    site->gw = gw;
    LIST_INSERT_HEAD(&tag->sites, site, link);
    return tag;
}
#endif

#ifndef MOCKED_LIFX_TAGGING_DECREF
void
lgtd_lifx_tagging_decref(struct lgtd_lifx_tag *tag,
                         struct lgtd_lifx_gateway *gw)
{
    (void)tag;
    (void)gw;
}
#endif

struct evbuffer *
evbuffer_new(void)
{
    return NULL;
}

void
evbuffer_free(struct evbuffer *buf)
{
    (void)buf;
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

static struct event *last_event_passed_to_event_add = NULL;

int
event_add(struct event *ev, const struct timeval *timeout)
{
    (void)timeout;
    last_event_passed_to_event_add = ev;
    return 0;
}

static struct event *last_event_passed_to_event_del = NULL;

int
event_del(struct event *ev)
{
    last_event_passed_to_event_del = ev;
    return 0;
}

void
event_active(struct event *ev, int res, short ncalls)
{
    (void)ev;
    (void)res;
    (void)ncalls;
}

struct event *
event_new(struct event_base *evbase,
          evutil_socket_t sock,
          short events,
          event_callback_fn cb,
          void *ctx)
{
    (void)evbase;
    (void)sock;
    (void)events;
    (void)cb;
    (void)ctx;
    return NULL;
}

void
event_free(struct event *ev)
{
    (void)ev;
}

int
event_pending(const struct event *ev, short events, struct timeval *tv)
{
    (void)ev;
    (void)events;
    (void)tv;
    return 0;
}

int
evutil_closesocket(evutil_socket_t sock)
{
    (void)sock;
    return 0;
}

int
evutil_make_socket_nonblocking(evutil_socket_t sock)
{
    (void)sock;
    return 0;
}

int
evutil_make_listen_socket_reuseable(evutil_socket_t sock)
{
    (void)sock;
    return 0;
}
