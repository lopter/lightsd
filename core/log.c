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

#include <sys/socket.h>
#include <sys/tree.h>
#include <sys/time.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <assert.h>
#include <endian.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if LGTD_HAVE_LIBBSD
#include <bsd/unistd.h>
#endif

#include <event2/event.h>

#include "lifx/wire_proto.h"
#include "stats.h"
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
    LGTD_TM_TO_ISOTIME(&tm_now, strbuf, bufsz, now.tv_usec);
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

char *
lgtd_iee8023mactoa(const uint8_t *addr, char *buf, int buflen)
{
    assert(addr);
    assert(buf);
    assert(buflen >= 2 * 6 + 5 + 1);

    snprintf(
        buf, buflen, "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
        addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]
    );
    return buf;
}

char *
lgtd_sockaddrtoa(const struct sockaddr *peer, char *buf, int buflen)
{
    assert(peer);
    assert(buf);
    assert(buflen > 1);

    const char *printed = NULL;
    int i = 0;
    switch (peer->sa_family) {
    case AF_INET:
        (void)0;
        const struct sockaddr_in *in_peer = (const struct sockaddr_in *)peer;
        LGTD_SNPRINTF_APPEND(buf, i, buflen, "[::ffff:");
        printed = inet_ntop(AF_INET, &in_peer->sin_addr, &buf[i], buflen - i);
        if (printed) {
            i += strlen(printed);
            LGTD_SNPRINTF_APPEND(
                buf, i, buflen, "]:%hu", ntohs(in_peer->sin_port)
            );
        }
        break;
    case AF_INET6:
        (void)0;
        const struct sockaddr_in6 *in6_peer = (const struct sockaddr_in6 *)peer;
        LGTD_SNPRINTF_APPEND(buf, i, buflen, "[");
        printed = inet_ntop(AF_INET6, &in6_peer->sin6_addr, &buf[i], buflen - i);
        if (printed) {
            i += strlen(printed);
            LGTD_SNPRINTF_APPEND(
                buf, i, buflen, "]:%hu", ntohs(in6_peer->sin6_port)
            );
        }
        break;
    case AF_UNIX:
        (void)0;
        const struct sockaddr_un *un_path = (const struct sockaddr_un *)peer;
        LGTD_SNPRINTF_APPEND(buf, i, buflen, "at %s", un_path->sun_path);
        printed = buf;
        break;
    default:
        break;
    }

    if (!printed) {
        buf[0] = 0;
        lgtd_warnx("not enough space to log an ip address");
#ifndef NDEBUG
        abort();
#endif
    }

    return buf;
}

char *
lgtd_print_duration(uint64_t secs, char *buf, int bufsz)
{
    assert(buf);
    assert(bufsz > 0);

    int days = secs / (60 * 60 * 24);
    int minutes = secs / 60;
    int hours = minutes / 60;
    hours = hours % 24;
    minutes = minutes % 60;

    int i = 0;
    if (days) {
        int n = snprintf(buf, bufsz, "%d days ", days);
        i = LGTD_MIN(i + n, bufsz);
    }
    snprintf(&buf[i], bufsz - i, "%02d:%02d", hours, minutes);
    return buf;
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
