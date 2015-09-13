#include "client.c"

#include "lifx/wire_proto.h"

#include "mock_daemon.h"
#define MOCKED_EVBUFFER_PULLUP
#define MOCKED_EVBUFFER_DRAIN
#define MOCKED_EVBUFFER_GET_CONTIGUOUS_SPACE
#define MOCKED_EVBUFFER_GET_LENGTH
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
    "\"id\": \"verylongidyooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo\","
    "\"jsonrpc\": \"2.0\","
    "\"method\": \"get_light_state\","
    "\"params\": [\"*\"]"
"}");

#define REQUEST_LEN (sizeof(request) - 1)
#define PART_1_LEN ((LGTD_CLIENT_MAX_REQUEST_BUF_SIZE) + 1)
#define PART_2_LEN ((REQUEST_LEN) - (PART_1_LEN))

static int
get_nbytes_read(int call_count)
{
    switch (call_count) {
    case 0:
        return PART_1_LEN;
    case 1:
        return PART_2_LEN;
    default:
        return 0;
    }
}

static int evbuffer_get_length_call_count = 0;

size_t
evbuffer_get_length(const struct evbuffer *buf)
{
    if (buf != FAKE_BUFFEREVENT_INPUT_BUF) {
        errx(
            1, "evbuffer_get_length got buf %p (expected %p)",
            buf, FAKE_BUFFEREVENT_INPUT_BUF
        );
    }

    if (evbuffer_get_length_call_count) {
        errx(1, "evbuffer_get_length should have been called once");
    }

    return get_nbytes_read(evbuffer_get_length_call_count++);
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

void
lgtd_jsonrpc_dispatch_request(struct lgtd_client *client, int parsed)
{
    (void)client;
    (void)parsed;

    errx(1, "No request should have been dispatched");
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
        if (len != PART_1_LEN) {
            errx(
                1, "trying to drain %ju bytes (expected %ju)",
                (uintmax_t)len, (uintmax_t)PART_1_LEN
            );
        }
        break;
    case 1:
        if (len != PART_2_LEN) {
            errx(
                1, "trying to drain %ju bytes (expected %ju)",
                (uintmax_t)len, (uintmax_t)PART_2_LEN
            );
        }
        break;
    default:
        errx(
            1, "evbuffer_drain shouldn't have been called %d times",
            evbuffer_drain_call_count
        );
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
        if (size != PART_1_LEN) {
            errx(
                1, "trying to pullup %ju bytes (expected %ju)",
                (intmax_t)size, (uintmax_t)PART_1_LEN
            );
        }
        offset = 0;
        break;
    case 1:
        if (size != -1) {
            errx(
                1, "trying to pullup %ju bytes (expected %ju)",
                (intmax_t)size, (uintmax_t)-1
            );
        }
        offset = PART_1_LEN;
        break;
    case 2:
        if (size != -1) {
            errx(1, "trying to pullup %ju bytes (expected -1)", (intmax_t)size);
        }
        offset = REQUEST_LEN;
        break;
    default:
        errx(
            1, "evbuffer_pullup shouldn't have been called %d times",
            evbuffer_pullup_call_count
        );
    }

    return &request[offset];
}

int
main(void)
{
    if (LGTD_CLIENT_MAX_REQUEST_BUF_SIZE >= REQUEST_LEN) {
        errx(
            1, "Please adjust this test to copy with the new value of "
            "LGTD_CLIENT_MAX_REQUEST_BUF_SIZE"
        );
    }

    struct lgtd_client *client;
    client = lgtd_tests_insert_mock_client(FAKE_BUFFEREVENT);

    lgtd_client_read_callback(FAKE_BUFFEREVENT, client);

    if (!evbuffer_get_contiguous_space_call_count) {
        errx(1, "evbuffer_get_contiguous_space not called");
    }
    if (!evbuffer_drain_call_count) {
        errx(1, "evbuffer_drain not called");
    }
    if (!evbuffer_pullup_call_count) {
        errx(1, "evbuffer_pullup not called");
    }
    if (!evbuffer_get_length_call_count) {
        errx(1, "evbuffer_get_length not called");
    }

    return 0;
}
