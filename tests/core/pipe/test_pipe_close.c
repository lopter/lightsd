#include "pipe.c"

#include <sys/tree.h>
#include <endian.h>
#include <limits.h>

#include "lifx/wire_proto.h"

#define MOCKED_EVENT_NEW
#define MOCKED_EVBUFFER_NEW
#define MOCKED_EVENT_DEL
#define MOCKED_EVBUFFER_FREE
#define MOCKED_EVENT_FREE
#include "mock_event2.h"
#include "mock_gateway.h"

#include "tests_utils.h"
#include "tests_pipe_utils.h"

char *tmpdir = NULL;

void
cleanup_tmpdir(void)
{
    lgtd_tests_remove_temp_dir(tmpdir);
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

static int event_del_call_count = 0;

int
event_del(struct event *ev)
{
    (void)ev;

    event_del_call_count++;

    return 0;
}

static int evbuffer_free_call_count = 0;

void
evbuffer_free(struct evbuffer *buf)
{
    (void)buf;

    evbuffer_free_call_count++;
}

static int event_free_call_count = 0;

void
event_free(struct event *ev)
{
    (void)ev;

    event_free_call_count++;
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

    int pipe_fd = SLIST_FIRST(&lgtd_command_pipes)->fd;

    lgtd_command_pipe_close_all();

    if (event_del_call_count != 1) {
        errx(1, "event_del_call_count = %d", event_del_call_count);
    }
    if (evbuffer_free_call_count != 1) {
        errx(1, "evbuffer_free_call_count = %d", evbuffer_free_call_count);
    }
    if (event_free_call_count != 1) {
        errx(1, "event_free_call_count = %d", event_free_call_count);
    }
    struct stat sb;
    if (fstat(pipe_fd, &sb) != -1 && errno != EBADF) {
        errx(1, "the pipe file descriptor wasn't closed correctly");
    }
    if (stat(path, &sb) != -1 && errno != ENOENT) {
        errx(1, "the pipe wasn't removed correctly");
    }

    return 0;
}
