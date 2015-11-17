#include <sys/types.h>

int mock_open(const char *, int, ...);
ssize_t mock_read(int, void *, size_t);
int mock_close(int);

#define open(fp, flags, ...) mock_open(fp, flags, ##__VA_ARGS__)
#define read(fd, buf, sz) mock_read(fd, buf, sz)
#define close(fd) mock_close(fd)

#include "daemon.c"

#include "mock_gateway.h"
#include "mock_pipe.h"
#include "mock_router.h"
#include "mock_log.h"
#include "mock_timer.h"

static const uint32_t MOCK_RANDOM_NUMBER = 0x72616e64;

int mock_open_call_count = 0;

int
mock_open(const char *fp, int flags, ...)
{
    mock_open_call_count++;

    if (strcmp(fp, "/dev/urandom")) {
        errx(1, "got fp %s (expected /dev/urandom)", fp);
    }

    if (flags != O_RDONLY) {
        errx(1, "got flags %#x (expected %#x)", flags, O_RDONLY);
    }

    return 42;
}

int mock_read_call_count = 0;

ssize_t
mock_read(int fd, void *buf, size_t nbytes)
{
    mock_read_call_count++;

    if (fd != 42) {
        errx(1, "got fd %d (expected 42)", fd);
    }

    if (!buf) {
        errx(1, "missing buf");
    }

    if (nbytes != 4) {
        errx(1, "got nbytes %ju (expected 4)", (uintmax_t)nbytes);
    }

    memcpy(buf, &MOCK_RANDOM_NUMBER, sizeof(MOCK_RANDOM_NUMBER));

    return nbytes;
}

int mock_close_call_count = 0;

int
mock_close(int fd)
{
    mock_close_call_count++;

    if (fd != 42) {
        errx(1, "got fd %d (expected 42)", fd);
    }

    return 0;
}

int
main(void)
{
    if (lgtd_daemon_randuint32() != MOCK_RANDOM_NUMBER) {
        errx(1, "got unexpected value from randuint32");
    }
    if (mock_open_call_count != 1) {
        errx(1, "open wasn't once");
    }
    if (mock_read_call_count != 1) {
        errx(1, "read wasn't once");
    }
    if (mock_close_call_count != 1) {
        errx(1, "close wasn't once");
    }

    return 0;
}
