#pragma once

#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/event.h>

#define MOCK_EVENT_NEW_EVENT_PTR ((void *)0xdadadada)

#ifndef MOCKED_EVBUFFER_DRAIN
int
evbuffer_drain(struct evbuffer *buf, size_t len)
{
    (void)buf;
    (void)len;
    return 0;
}
#endif

#ifndef MOCKED_EVBUFFER_NEW
struct evbuffer *
evbuffer_new(void)
{
    return NULL;
}
#endif

#ifndef MOCKED_EVENT_FREE
void
evbuffer_free(struct evbuffer *buf)
{
    (void)buf;
}
#endif

#ifndef MOCKED_EVBUFFER_GET_LENGTH
size_t
evbuffer_get_length(const struct evbuffer *buf)
{
    (void)buf;
    return 0;
}
#endif

#ifndef MOCKED_EVBUFFER_PULLUP
unsigned char *
evbuffer_pullup(struct evbuffer *buf, ev_ssize_t size)
{
    (void)buf;
    (void)size;
    return NULL;
}
#endif

#ifndef MOCKED_EVBUFFER_READ
int
evbuffer_read(struct evbuffer *buffer, evutil_socket_t fd, int howmuch)
{
    (void)buffer;
    (void)fd;
    return howmuch;
}
#endif

#ifndef MOCKED_EVBUFFER_GET_CONTIGUOUS_SPACE
size_t
evbuffer_get_contiguous_space(const struct evbuffer *buf)
{
    (void)buf;
    return 0;
}
#endif

#ifndef MOCKED_EVENT_ADD
int
event_add(struct event *ev, const struct timeval *timeout)
{
    (void)ev;
    (void)timeout;
    return 0;
}
#endif

#ifndef MOCKED_EVENT_DEL
int
event_del(struct event *ev)
{
    (void)ev;
    return 0;
}
#endif

#ifndef MOCKED_EVENT_FREE
void
event_free(struct event *ev)
{
    (void)ev;
}
#endif

#ifndef MOCKED_EVENT_NEW
struct event *
event_new(struct event_base *base,
          evutil_socket_t fd,
          short events,
          event_callback_fn cb,
          void *ctx)
{
    (void)base;
    (void)fd;
    (void)events;
    (void)cb;
    (void)ctx;
    return MOCK_EVENT_NEW_EVENT_PTR;
}
#endif

#ifndef MOCKED_EVENT_ACTIVE
void
event_active(struct event *ev, int res, short ncalls)
{
    (void)ev;
    (void)res;
    (void)ncalls;
}
#endif

#ifndef MOCKED_EVENT_PENDING
int
event_pending(const struct event *ev, short events, struct timeval *tv)
{
    (void)ev;
    (void)events;
    (void)tv;
    return 0;
}
#endif

#ifndef MOCKED_EVUTIL_MAKE_SOCKET_NONBLOCKING
int
evutil_make_socket_nonblocking(evutil_socket_t fd)
{
    (void)fd;
    return 0;
}
#endif

#ifndef MOCKED_BUFFEREVENT_GET_INPUT
struct evbuffer *
bufferevent_get_input(struct bufferevent *bufev)
{
    (void)bufev;
    return NULL;
}
#endif

#ifndef MOCKED_BUFFEREVENT_ENABLE
int
bufferevent_enable(struct bufferevent *bufev, short event)
{
    (void)bufev;
    (void)event;
    return 0;
}
#endif

#ifndef MOCKED_BUFFEREVENT_FREE
void
bufferevent_free(struct bufferevent *bufev)
{
    (void)bufev;
}
#endif

#ifndef MOCKED_BUFFEREVENT_SETCB
void
bufferevent_setcb(struct bufferevent *bufev,
                  bufferevent_data_cb readcb,
                  bufferevent_data_cb writecb,
                  bufferevent_event_cb eventcb,
                  void *cbarg)
{
    (void)bufev;
    (void)readcb;
    (void)writecb;
    (void)eventcb;
    (void)cbarg;
}
#endif

#ifndef MOCKED_BUFFEREVENT_SOCKET_NEW
struct bufferevent *
bufferevent_socket_new(struct event_base *base, evutil_socket_t fd, int options)
{
    (void)base;
    (void)fd;
    (void)options;
    return NULL;
}
#endif

#ifndef MOCKED_BUFFEREVENT_WRITE
int
bufferevent_write(struct bufferevent *bufev,
                  const void *data,
                  size_t size)
{
    (void)bufev;
    (void)data;
    (void)size;
    return 0;
}
#endif
