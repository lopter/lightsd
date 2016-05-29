#include "pipe.c"

#include <sys/tree.h>
#include <sys/stat.h>
#include <endian.h>
#include <limits.h>

#include "lifx/wire_proto.h"

#include "mock_daemon.h"
#define MOCKED_EVUTIL_MAKE_SOCKET_NONBLOCKING
#define MOCKED_EVENT_NEW
#define MOCKED_EVBUFFER_NEW
#define MOCKED_EVENT_ADD
#include "mock_event2.h"
#include "mock_gateway.h"
#include "mock_jsonrpc.h"
#include "mock_log.h"
#include "mock_router.h"
#include "mock_timer.h"

#include "tests_utils.h"

char *tmpdir = NULL;

void
cleanup_tmpdir(void)
{
    lgtd_tests_remove_temp_dir(tmpdir);
}

static bool make_socket_nonblocking_call_count = 0;

int 
evutil_make_socket_nonblocking(evutil_socket_t fd)
{
    if (fd <= 0) {
        errx(1, "got invalid fd %d in make_socket_nonblocking", fd);
    }

    make_socket_nonblocking_call_count++;

    return 0;
}

static int event_new_call_count = 0;

struct event *
event_new(struct event_base *base,
          evutil_socket_t fd,
          short events,
          event_callback_fn cb,
          void *ctx)
{
    if (base != lgtd_ev_base) {
        errx(
            1, "unexpected lgtd_ev_base = %p (expected %p)",
            base, lgtd_ev_base
        );
    }
    if (fd <= 0) {
        errx(1, "got invalid fd %d in event_new", fd);
    }
    if (events != (EV_READ|EV_PERSIST)) {
        errx(1, "got events %#x (expected %#x)", events, EV_READ|EV_PERSIST);
    }
    if (cb != lgtd_command_pipe_read_callback) {
        errx(1, "the read callback wasn't set correctly");
    }
    if (!ctx) {
        errx(1, "the callback context wasn't set correctly");
    }

    event_new_call_count++;

    return (void *)1;
}

static int evbuffer_new_call_count = 0;

struct evbuffer *
evbuffer_new(void)
{
    evbuffer_new_call_count++;

    return (void *)2;
}

static int event_add_call_count = 0;

int
event_add(struct event *ev, const struct timeval *timeout)
{
    if (ev != (void *)1) {
        errx(1, "got unexpected event %p (expected %p)", ev, (void*)1);
    }

    if (timeout) {
        errx(1, "a timeout shouldn't have been passed");
    }

    event_add_call_count++;

    return 0;
}

int
main(void)
{
    tmpdir = lgtd_tests_make_temp_dir();
    atexit(cleanup_tmpdir);

    char path[PATH_MAX] = { 0 };
    snprintf(path, sizeof(path), "%s/lightsd.pipe", tmpdir);

    if (mkfifo(path, 0)) {
        errx(1, "can't open fifo");
    }

    if (!lgtd_command_pipe_open(path)) {
        errx(1, "couldn't open pipe");
    }

    if (make_socket_nonblocking_call_count != 1) {
        errx(
            1, "make_socket_nonblocking_call_count = %d",
            make_socket_nonblocking_call_count
        );
    }
    if (event_new_call_count != 1) {
        errx(1, "event_new_call_count = %d", event_new_call_count);
    }
    if (evbuffer_new_call_count != 1) {
        errx(1, "evbuffer_new_call_count = %d", evbuffer_new_call_count);
    }
    if (event_add_call_count != 1) {
        errx(1, "event_add_call_count = %d", event_add_call_count);
    }
    if (SLIST_EMPTY(&lgtd_command_pipes)) {
        errx(1, "the list of command pipes shouldn't be empty");
    }

    struct stat sb;
    if (stat(path, &sb)) {
        errx(1, "can't stat pipe %s", path);
    }

    mode_t expected_mode;
    expected_mode = S_IFIFO|S_IWUSR|S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IWGRP;
    if (sb.st_mode != expected_mode) {
        errx(
            1, "unexpected mode %o (expected %o)",
            sb.st_mode, expected_mode
        );
    }

    // make sure it's idempotent:
    if (!lgtd_command_pipe_open(path)) {
        errx(1, "couldn't open pipe");
    }
    if (event_new_call_count != 1) {
        errx(1, "event_new_call_count = %d", event_new_call_count);
    }

    return 0;
}
