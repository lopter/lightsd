// Copyright (c) 2014, Louis Opter <kalessin@kalessin.fr>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <sys/tree.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <assert.h>
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
#if LGTD_SUSECONDS_T_SIZE == 4
        strbuf, bufsz, "%d-%02d-%02dT%02d:%02d:%02d.%d%c%02ld:%02ld",
#else
        strbuf, bufsz, "%d-%02d-%02dT%02d:%02d:%02d.%ld%c%02ld:%02ld",
#endif
        1900 + tm_now.tm_year, 1 + tm_now.tm_mon, tm_now.tm_mday,
        tm_now.tm_hour, tm_now.tm_min, tm_now.tm_sec,
        now.tv_usec, tm_now.tm_gmtoff >= 0 ? '+' : '-', // %+02ld doesn't work
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
            timestr, loglvl, showprogname ? "lightsd " : ""
        );
        return;
    }
    fprintf(stderr, "[%s] %s", loglvl, showprogname ? "lightsd " : "");
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
