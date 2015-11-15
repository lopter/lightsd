#include <pwd.h>
#include <grp.h>
#include <unistd.h>

int mock_open(const char *, int, ...);
int mock_fchown(int, uid_t, gid_t);
int mock_write(int, const void *, size_t);
int mock_close(int);
pid_t mock_getpid(void);

#define open(fp, flags, ...) mock_open(fp, flags, ##__VA_ARGS__)
#define fchown(fd, uid, gid) mock_fchown(fd, uid, gid)
#define write(fd, buf, sz) mock_write(fd, buf, sz)
#define close(fd) mock_close(fd)
#define getpid() mock_getpid()
#include "daemon.c"

#include "mock_gateway.h"
#include "mock_pipe.h"
#include "mock_router.h"
#include "mock_log.h"
#include "mock_timer.h"

#include "tests_utils.h"

int mock_getpid_call_count = 0;

pid_t
mock_getpid(void)
{
    mock_getpid_call_count++;

    return 1234;
}

int mock_open_call_count = 0;

int
mock_open(const char *fp, int flags, ...)
{
    mock_open_call_count++;

    va_list ap;
    va_start(ap, flags);
    mode_t mode = va_arg(ap, int);
    va_end(ap);

    if (strcmp(fp, "/test.pid")) {
        lgtd_errx(1, "got fp %s (expected /test.pid)", fp);
    }

    int expected_flags = O_CREAT|O_WRONLY|O_TRUNC;
    if (flags != expected_flags) {
        lgtd_errx(1, "got flags %#x (expected %#x)", flags, expected_flags);
    }

    mode_t expected_mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH;
    if (mode != expected_mode) {
        lgtd_errx(1, "got mode %#x (expected %#x)", mode, expected_mode);
    }

    return 42;
}

int mock_fchown_call_count = 0;

int
mock_fchown(int fd, uid_t uid, gid_t gid)
{
    mock_fchown_call_count++;

    if (fd != 42) {
        lgtd_errx(1, "god fd %d (expected 42)", fd);
    }

    if (uid != 1000) {
        lgtd_errx(1, "god uid %d (expected 1000)", uid);
    }

    if (gid != 500) {
        lgtd_errx(1, "god gid %d (expected 500)", gid);
    }

    return 0;
}

int mock_write_call_count = 0;

int
mock_write(int fd, const void *buf, size_t nbytes)
{
    mock_write_call_count++;

    if (fd != 42) {
        lgtd_errx(1, "got fd %d (expected 42)", fd);
    }

    if (!buf || strcmp((const char *)buf, "1234")) {
        lgtd_errx(1, "writing pid %s (expected 1234)", (const char *)buf);
    }

    if (nbytes != 4) {
        lgtd_errx(1, "got nbytes %ju (expected 4)", (uintmax_t)nbytes);
    }

    return 0;
}

int mock_close_call_count = 0;

int
mock_close(int fd)
{
    mock_close_call_count++;

    if (fd != 42) {
        lgtd_errx(1, "got fd %d (expected 42)", fd);
    }

    return 0;
}

int
main(void)
{
    struct passwd user_info = { .pw_uid = 1000, .pw_gid = 500 };
    lgtd_user_info = &user_info;
    struct group group_info = { .gr_gid = 500 };
    lgtd_group_info = &group_info;

    lgtd_daemon_write_pidfile("/test.pid");
    if (!mock_open_call_count) {
        lgtd_errx(1, "open wasn't called");
    }
    if (!mock_fchown_call_count) {
        lgtd_errx(1, "fchown wasn't called");
    }
    if (!mock_getpid_call_count) {
        lgtd_errx(1, "getpid wasn't called");
    }
    if (!mock_write_call_count) {
        lgtd_errx(1, "write wasn't called");
    }
    if (!mock_close_call_count) {
        lgtd_errx(1, "close wasn't called");
    }

    lgtd_daemon_write_pidfile("/test.pid");
    if (mock_open_call_count != 2) {
        lgtd_errx(1, "open wasn't called");
    }
    if (mock_fchown_call_count != 2) {
        lgtd_errx(1, "fchown wasn't called");
    }
    if (mock_getpid_call_count != 2) {
        lgtd_errx(1, "getpid wasn't called");
    }
    if (mock_write_call_count != 2) {
        lgtd_errx(1, "write wasn't called");
    }
    if (mock_close_call_count != 2) {
        lgtd_errx(1, "close wasn't called");
    }

    return 0;
}
