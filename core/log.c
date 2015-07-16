// Copyright (c) 2014, Louis Opter <kalessin@kalessin.fr>
//
// This file is part of lighstd.
//
// lighstd is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// lighstd is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with lighstd.  If not, see <http://www.gnu.org/licenses/>.

#include <sys/tree.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <assert.h>
#include <endian.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <event2/event.h>

#include "lifx/wire_proto.h"
#include "lightsd.h"

static void
lgtd_isotime_now(char *strbuf, int bufsz)
{
    assert(strbuf);
    assert(bufsz > 0);

    struct timeval now;
    if (gettimeofday(&now, NULL) == -1) {
        goto error;
    }
    struct tm tm_now;
    if (!localtime_r(&now.tv_sec, &tm_now)) {
        goto error;
    }
    // '2015-01-02T10:13:16.132222+00:00'
    snprintf(
        strbuf, bufsz, "%d-%02d-%02dT%02d:%02d:%02d.%jd%c%02ld:%02ld",
        1900 + tm_now.tm_year, 1 + tm_now.tm_mon, tm_now.tm_mday,
        tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec,
        (intmax_t)now.tv_usec, tm_now.tm_gmtoff >= 0 ? '+' : '-', // %+02ld doesn't work
        LGTD_ABS(tm_now.tm_gmtoff / 60 / 60), tm_now.tm_gmtoff % (60 * 60)
    );
    return;
error:
    strbuf[0] = '\0';
}

static void
lgtd_log_header(const char *loglvl, bool showprogname)
{
    if (lgtd_opts.log_timestamps) {
        char timestr[64];
        lgtd_isotime_now(timestr, sizeof(timestr));
        fprintf(
            stderr, "[%s] [%s] %s",
            timestr, loglvl, showprogname ? "lightsd: " : ""
        );
        return;
    }
    fprintf(stderr, "[%s] %s", loglvl, showprogname ? "lightsd: " : "");
}

const char *
lgtd_addrtoa(const uint8_t *addr)
{
    assert(addr);

    static char str[LGTD_LIFX_ADDR_LENGTH * 2 + LGTD_LIFX_ADDR_LENGTH - 1 + 1];
    snprintf(
        str, sizeof(str), "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
        addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]
    );
    return str;
}

void
lgtd_sockaddrtoa(const struct sockaddr_storage *peer, char *buf, int buflen)
{
    assert(peer);
    assert(buf);
    assert(buflen > 0);

    if (peer->ss_family == AF_INET) {
        const struct sockaddr_in *in_peer = (const struct sockaddr_in *)peer;
        inet_ntop(AF_INET, &in_peer->sin_addr, buf, buflen);
    } else {
        const struct sockaddr_in6 *in6_peer = (const struct sockaddr_in6 *)peer;
        inet_ntop(AF_INET6, &in6_peer->sin6_addr, buf, buflen);
    }
}

void
_lgtd_err(void (*errfn)(int, const char *, ...),
           int eval,
           const char *fmt,
           ...)
{
    int errsave = errno;
    va_list ap;
    va_start(ap, fmt);
    // lgtd_cleanup is probably going to free some of the arguments we got, so
    // let's print to a buffer before we call err.
    char errmsg[LGTD_ERROR_MSG_BUFSIZE];
    vsnprintf(errmsg, sizeof(errmsg), fmt, ap);
    va_end(ap);
    lgtd_cleanup();
    lgtd_log_header("ERR", false);
    errno = errsave;
    errfn(eval, errmsg);
}

void
_lgtd_warn(void (*warnfn)(const char *, va_list), const char *fmt, ...)
{
    if (lgtd_opts.verbosity <= LGTD_WARN) {
        va_list ap;
        va_start(ap, fmt);
        lgtd_log_header("WARN", false);
        warnfn(fmt, ap);
        va_end(ap);
    }
}

void
lgtd_info(const char *fmt, ...)
{
    if (lgtd_opts.verbosity <= LGTD_INFO) {
        va_list ap;
        va_start(ap, fmt);
        lgtd_log_header("INFO", true);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        fprintf(stderr, "\n");
    }
}

void
lgtd_debug(const char *fmt, ...)
{
    if (lgtd_opts.verbosity <= LGTD_DEBUG) {
        va_list ap;
        va_start(ap, fmt);
        lgtd_log_header("DEBUG", true);
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        fprintf(stderr, "\n");
    }
}

void
lgtd_libevent_log(int severity, const char *msg)
{
    switch (severity) {
    case EVENT_LOG_DEBUG:   lgtd_debug("%s", msg); break;
    case EVENT_LOG_MSG:     lgtd_info("%s", msg);  break;
    case EVENT_LOG_WARN:    lgtd_warnx("%s", msg)  break;
    case EVENT_LOG_ERR:     lgtd_warnx("%s", msg); break;
    default:                                        break;
    }
}
