#include "pipe.c"

#include <sys/tree.h>
#include <endian.h>
#include <limits.h>

#include "lifx/wire_proto.h"

#include "mock_daemon.h"
#define MOCKED_EVENT_NEW
#define MOCKED_EVBUFFER_NEW
#define MOCKED_EVBUFFER_READ
#define MOCKED_EVBUFFER_PULLUP
#define MOCKED_EVBUFFER_GET_LENGTH
#define MOCKED_EVBUFFER_DRAIN
#include "mock_event2.h"
#include "mock_gateway.h"
#define MOCKED_JSONRPC_DISPATCH_REQUEST
#include "mock_jsonrpc.h"
#include "mock_log.h"
#include "mock_router.h"
#include "mock_timer.h"

#include "tests_utils.h"

#define REQUEST_1 "{"                   \
    "\"jsonrpc\": \"2.0\","             \
    "\"method\": \"get_light_state\","  \
    "\"params\": [\"*\"],"              \
    "\"id\": 42"                        \
"}"

#define REQUEST_2 "{"           \
    "\"jsonrpc\": \"2.0\","     \
    "\"method\": \"power_on\"," \
    "\"params\": [\"*\"],"      \
    "\"id\": 43"                \
"}"

static unsigned char request[] = (
    REQUEST_1
    REQUEST_2
);

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

    switch (jsonrpc_dispatch_request_call_count) {
    case 0:
        if (memcmp(client->json, request, sizeof(request) - 1)) {
            errx(
                1, "got unexpected json %s (expected %s)",
                client->json, request
            );
        }
        break;
    case 1:
        if (memcmp(client->json, REQUEST_2, sizeof(REQUEST_2) - 1)) {
            errx(
                1, "got unexpected json %s (expected %s)",
                client->json, REQUEST_2
            );
        }
        break;
    default:
        errx(
            1, "jsonrpc_dispatch_request_call_count = %d",
            jsonrpc_dispatch_request_call_count
        );
        break;
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
        if (len != sizeof(REQUEST_1) - 1) {
            errx(
                1, "trying to drain %ju bytes (expected %ju)",
                (uintmax_t)len, (uintmax_t)sizeof(REQUEST_1) - 1
            );
        }
        break;
    case 1:
        if (len != sizeof(REQUEST_2) - 1) {
            errx(
                1, "trying to drain %ju bytes (expected %ju)",
                (uintmax_t)len, (uintmax_t)sizeof(REQUEST_2) - 1
            );
        }
        break;
    default:
        errx(1, "evbuffer_drain_call_count = %d", evbuffer_drain_call_count);
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

    int offset;
    switch (evbuffer_pullup_call_count) {
    case 0:
        offset = 0;
        break;
    case 1:
        offset = sizeof(REQUEST_1) - 1;
        break;
    default:
        offset = sizeof(request);
        break;
    }
    evbuffer_pullup_call_count++;

    return &request[offset];
}

static int evbuffer_get_length_call_count = 0;

size_t
evbuffer_get_length(const struct evbuffer *buf)
{
    if (buf != (void *)2) {
        errx(1, "got unexpected buf %p (expected %p)", buf, (void *)2);
    }

    size_t len;
    switch (evbuffer_get_length_call_count) {
    case 0:
        len = sizeof(request) - 1;
        break;
    case 1:
        len = sizeof(request) - sizeof(REQUEST_1);
        break;
    default:
        len = 0;
        break;
    }
    evbuffer_get_length_call_count++;

    return len;
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

    int rv = 0;
    switch (evbuffer_read_call_count) {
    case 0:
        rv = sizeof(request) - 1;
    default:
        break;
    }
    evbuffer_read_call_count++;

    return rv;
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

    return 0;
}
