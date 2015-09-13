#include "pipe.c"

#include <sys/tree.h>
#include <endian.h>
#include <limits.h>

#include "lifx/wire_proto.h"

#include "mock_daemon.h"
#define MOCKED_EVENT_NEW
#define MOCKED_EVENT_DEL
#define MOCKED_EVBUFFER_NEW
#define MOCKED_EVBUFFER_READ
#define MOCKED_EVBUFFER_PULLUP
#define MOCKED_EVBUFFER_GET_LENGTH
#define MOCKED_EVBUFFER_DRAIN
#include "mock_event2.h"
#include "mock_gateway.h"
#define MOCKED_JSONRPC_DISPATCH_REQUEST
#include "mock_jsonrpc.h"
#include "mock_router.h"
#include "mock_timer.h"

#include "tests_utils.h"

static unsigned char request[] = ("{"
    "\"jsonrpc\": \"2.0\","
    "\"method\": \"get_light_state\","
    "\"params\": [\"*\"],"
    "\"id\": 42"
"}");

static char *tmpdir = NULL;

void
cleanup_tmpdir(void)
{
    lgtd_tests_remove_temp_dir(tmpdir);
}

static int jsonrpc_dispatch_request_call_count = 0;

void
lgtd_jsonrpc_dispatch_request(struct lgtd_client *client, int parsed)
{
    (void)client;
    (void)parsed;

    if (!parsed) {
        errx(1, "number of parsed json tokens not passed in");
    }

    if (memcmp(client->json, request, sizeof(request))) {
        errx(1, "got unexpected json");
    }

    jsonrpc_dispatch_request_call_count++;
}

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

    return (void *)1;
}

static int event_del_call_count = 0;

int
event_del(struct event *ev)
{
    (void)ev;
    event_del_call_count++;
    return 0;
}

static int
get_nbytes_read(int call_count)
{
    switch (call_count) {
    case 0:
        return sizeof(request) - 1; // we don't return the '\0'
    default:
        return 0;
    }
}

struct evbuffer *
evbuffer_new(void)
{
    return (void *)2;
}

static int evbuffer_drain_call_count = 0;

int
evbuffer_drain(struct evbuffer *buf, size_t len)
{
    if (buf != (void *)2) {
        errx(1, "got unexpected buf %p (expected %p)", buf, (void *)2);
    }

    switch (evbuffer_drain_call_count) {
    case 0:
        if (len != sizeof(request) - 1) {
            errx(
                1, "trying to drain %ju bytes (expected %ju)",
                (uintmax_t)len, (uintmax_t)sizeof(request) - 1
            );
        }
        break;
    default:
        break;
    }
    evbuffer_drain_call_count++;

    return 0;
}

static int evbuffer_pullup_call_count = 0;

unsigned char *
evbuffer_pullup(struct evbuffer *buf, ev_ssize_t size)
{
    if (buf != (void *)2) {
        errx(1, "got unexpected buf %p (expected %p)", buf, (void *)2);
    }

    if (size != -1) {
        errx(
            1, "got unexpected size %jd in pullup (expected -1)", (intmax_t)size
        );
    }

    return &request[evbuffer_pullup_call_count++ ? sizeof(request) - 1 : 0];
}

static int evbuffer_get_length_call_count = 0;

size_t
evbuffer_get_length(const struct evbuffer *buf)
{
    if (buf != (void *)2) {
        errx(1, "got unexpected buf %p (expected %p)", buf, (void *)2);
    }

    return get_nbytes_read(evbuffer_get_length_call_count++);
}

static int evbuffer_read_call_count = 0;

int
evbuffer_read(struct evbuffer *buf, evutil_socket_t fd, int howmuch)
{
    if (buf != (void *)2) {
        errx(1, "got unexpected buf %p (expected %p)", buf, (void *)2);
    }

    struct lgtd_command_pipe *pipe = SLIST_FIRST(&lgtd_command_pipes);
    if (fd != pipe->fd) {
        errx(1, "got unexpected fd %d (expected %d)", fd, pipe->fd);
    }

    if (howmuch != -1) {
        errx(
            1, "got unexpected howmuch bytes to read %d (expected -1)", howmuch
        );
    }

    return get_nbytes_read(evbuffer_read_call_count++);
}

int
main(void)
{
    tmpdir = lgtd_tests_make_temp_dir();
    atexit(cleanup_tmpdir);

    char path[PATH_MAX] = { 0 };
    snprintf(path, sizeof(path), "%s/lightsd.pipe", tmpdir);
    if (!lgtd_command_pipe_open(path)) {
        errx(1, "couldn't open pipe");
    }

    struct lgtd_command_pipe *pipe = SLIST_FIRST(&lgtd_command_pipes);
    lgtd_command_pipe_read_callback(pipe->fd, EV_READ, pipe);
    if (event_del_call_count != 1) {
        errx(1, "the pipe wasn't reset");
    }

    jsonrpc_dispatch_request_call_count = 0;
    evbuffer_drain_call_count = 0;
    evbuffer_read_call_count = 0;
    evbuffer_pullup_call_count = 0;
    evbuffer_get_length_call_count = 0;
    event_del_call_count = 0;
    pipe = SLIST_FIRST(&lgtd_command_pipes);
    lgtd_command_pipe_read_callback(pipe->fd, EV_READ, pipe);
    if (event_del_call_count != 1) {
        errx(1, "the pipe wasn't reset");
    }

    return 0;
}
