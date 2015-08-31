#include "client.c"

#include "lifx/wire_proto.h"

#include "mock_daemon.h"
#define MOCKED_EVBUFFER_PULLUP
#define MOCKED_EVBUFFER_DRAIN
#define MOCKED_EVBUFFER_GET_CONTIGUOUS_SPACE
#define MOCKED_BUFFEREVENT_GET_INPUT
#include "mock_event2.h"
#include "mock_gateway.h"
#define MOCKED_JSONRPC_DISPATCH_REQUEST
#include "mock_jsonrpc.h"
#include "mock_router.h"
#include "mock_timer.h"

#include "tests_utils.h"
#include "tests_client_utils.h"

static unsigned char request[] = ("{"
    "\"jsonrpc\": \"2.0\","
    "\"method\": \"get_light_state\","
    "\"params\": [\"*\"],"
    "\"id\": 42"
"}");

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
bufferevent_get_input(struct bufferevent *bufev)
{
    (void)bufev;

    return FAKE_BUFFEREVENT_INPUT_BUF;
}

static int evbuffer_get_contiguous_space_call_count = 0;

size_t
evbuffer_get_contiguous_space(const struct evbuffer *buf)
{
    if (buf != FAKE_BUFFEREVENT_INPUT_BUF) {
        errx(
            1, "evbuffer_get_contiguous_space got buf %p (expected %p)",
            buf, FAKE_BUFFEREVENT_INPUT_BUF
        );
    }

    return get_nbytes_read(evbuffer_get_contiguous_space_call_count++);
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

    if (jsonrpc_dispatch_request_call_count++) {
        errx(1, "jsonrpc_dispatch_request should have been called once");
    }
}

static int evbuffer_drain_call_count = 0;

int
evbuffer_drain(struct evbuffer *buf, size_t len)
{
    if (buf != FAKE_BUFFEREVENT_INPUT_BUF) {
        errx(1, "got unexpected buf %p (expected %p)", buf, (void *)2);
    }

    switch (evbuffer_drain_call_count++) {
    case 0:
        if (len != sizeof(request) - 1) {
            errx(
                1, "trying to drain %ju bytes (expected %ju)",
                (uintmax_t)len, (uintmax_t)sizeof(request) - 1
            );
        }
        break;
    default:
        errx(1, "evbuffer_drain should have been called once");
        break;
    }

    return 0;
}

static int evbuffer_pullup_call_count = 0;

unsigned char *
evbuffer_pullup(struct evbuffer *buf, ev_ssize_t size)
{
    if (buf != FAKE_BUFFEREVENT_INPUT_BUF) {
        errx(1, "got unexpected buf %p (expected %p)", buf, (void *)2);
    }

    int offset;
    switch (evbuffer_pullup_call_count++) {
    case 0:
        if (size != sizeof(request) - 1) {
            errx(
                1, "got unexpected size %jd in pullup (expected %ju)",
                (intmax_t)size, (uintmax_t)sizeof(request) - 1
            );
        }
        offset = 0;
        break;
    case 1:
        if (size != -1) {
            errx(
                1, "got unexpected size %jd in pullup (expected -1)",
                (intmax_t)size
            );
        }
        offset = sizeof(request) - 1;
        break;
    default:
        errx(1, "evbuffer_pullup should have been called twice");
    }

    return &request[offset];
}

int
main(void)
{
    struct lgtd_client client = { .io = FAKE_BUFFEREVENT };

    lgtd_client_read_callback(FAKE_BUFFEREVENT, &client);

    if (!evbuffer_get_contiguous_space_call_count) {
        errx(1, "evbuffer_get_contiguous_space not called");
    }
    if (!jsonrpc_dispatch_request_call_count) {
        errx(1, "jsonrpc_dispatch_request not called");
    }
    if (!evbuffer_drain_call_count) {
        errx(1, "evbuffer_drain not called");
    }
    if (!evbuffer_pullup_call_count) {
        errx(1, "evbuffer_pullup not called");
    }

    return 0;
}
