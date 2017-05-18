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

// reallocarray has been copied from OpenBSD under the following license:
/*
 * Copyright (c) 2008 Otto Moerbeek <otto@drijf.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#ifndef __attribute__
# define __attribute__(e)
#endif

#define LGTD_ABS(v) ((v) >= 0 ? (v) : (v) * -1)
#define LGTD_MIN(a, b) ((a) < (b) ? (a) : (b))
#define LGTD_MAX(a, b) ((a) > (b) ? (a) : (b))

#define LGTD_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define LGTD_SWAP(type, a, b) do { type _tmp = a; a = b; b = _tmp; } while (0)

#define LGTD_MSECS_TO_TIMEVAL(v) {  \
    .tv_sec = (v) / 1000,           \
    .tv_usec = ((v) % 1000) * 1000  \
}
#define LGTD_MSECS_TO_TIMESPEC(v) {     \
    .tv_sec = (v) / 1000,               \
    .tv_nsec = ((v) % 1000) * 1000000   \
}
#define LGTD_NSECS_TO_USECS(v) ((v) / (unsigned int)1E6)
#define LGTD_NSECS_TO_SECS(v) ((v) / (unsigned int)1E9)
#define LGTD_SECS_TO_NSECS(v) ((v) * (unsigned int)1E9)

#define LGTD_TM_TO_ISOTIME(tm, sbuf, bufsz, usec) do {                          \
    /* '2015-01-02T10:13:16.132222+00:00' */                                    \
    if ((usec)) {                                                               \
        snprintf(                                                               \
            (sbuf), (bufsz), "%d-%02d-%02dT%02d:%02d:%02d.%jd%c%02ld:%02ld",    \
            1900 + (tm)->tm_year, 1 + (tm)->tm_mon, (tm)->tm_mday,              \
            (tm)->tm_hour, (tm)->tm_min, (tm)->tm_sec, (intmax_t)usec,          \
            (tm)->tm_gmtoff >= 0 ? '+' : '-', /* %+02ld doesn't work */         \
            LGTD_ABS((tm)->tm_gmtoff / 60 / 60), (tm)->tm_gmtoff % (60 * 60)    \
        );                                                                      \
    } else {                                                                    \
        snprintf(                                                               \
            (sbuf), (bufsz), "%d-%02d-%02dT%02d:%02d:%02d%c%02ld:%02ld",        \
            1900 + (tm)->tm_year, 1 + (tm)->tm_mon, (tm)->tm_mday,              \
            (tm)->tm_hour, (tm)->tm_min, (tm)->tm_sec,                          \
            (tm)->tm_gmtoff >= 0 ? '+' : '-', /* %+02ld doesn't work */         \
            LGTD_ABS((tm)->tm_gmtoff / 60 / 60), (tm)->tm_gmtoff % (60 * 60)    \
        );                                                                      \
    }                                                                           \
} while (0)

#define LGTD_SNPRINTF_APPEND(buf, i, bufsz, ...) do {       \
    int n = snprintf(&(buf)[(i)], bufsz - i, __VA_ARGS__);  \
    (i) = LGTD_MIN((i) + n, bufsz);                         \
} while (0)

#if LGTD_BIG_ENDIAN_SYSTEM
# define LGTD_STATIC_HTONS(s) (s)
# define LGTD_STATIC_HTONL(l) (l)
#else
# define LGTD_STATIC_HTONS(s) ((((s) << 8) & 0xff00) | (((s) >> 8) & 0xff))
# define LGTD_STATIC_HTONL(l) (                                 \
    (((l) & 0xff000000) >> 24) | (((l) & 0x00ff0000) >> 8)      \
    | (((l) & 0x0000ff00) << 8) | (((l) & 0x000000ff) << 24)    \
)
#endif


enum lgtd_verbosity {
    LGTD_DEBUG = 0,
    LGTD_INFO,
    LGTD_WARN,
    LGTD_ERR
};

enum { LGTD_ERROR_MSG_BUFSIZE = 2048 };

// FIXME: introspect sizeof(sockaddr_un.sun_path) with CMake to generate a
// reasonable value for that:
enum { LGTD_SOCKADDR_STRLEN = 128 };

#if !LGTD_HAVE_REALLOCARRAY
# define MUL_NO_OVERFLOW ((size_t)1 << (sizeof(size_t) * 4))
static inline void *
reallocarray(void *optr, size_t nmemb, size_t size)
{
    if ((nmemb >= MUL_NO_OVERFLOW || size >= MUL_NO_OVERFLOW) &&
        nmemb > 0 && SIZE_MAX / nmemb < size) {
# ifndef NDEBUG
        abort();
# endif
        return NULL;
    }
    return realloc(optr, size * nmemb);
}
#endif

struct sockaddr;

struct lgtd_opts {
    bool                foreground;
    bool                log_timestamps;
    enum lgtd_verbosity verbosity;
    const char          *user;
    const char          *group;
    bool                syslog;
    int                 syslog_facility;
    const char          *syslog_ident;
    const char          *pidfile;
};

extern struct lgtd_opts lgtd_opts;
extern struct event_base *lgtd_ev_base;
extern const char *lgtd_progname;

char *lgtd_iee8023mactoa(const uint8_t *addr, char *buf, int buflen);
#define LGTD_IEEE8023MACTOA(addr, buf) \
    lgtd_iee8023mactoa((addr), (buf), sizeof(buf))
char *lgtd_sockaddrtoa(const struct sockaddr *, char *buf, int buflen);
#define LGTD_SOCKADDRTOA(addr, buf) \
    lgtd_sockaddrtoa((addr), (buf), sizeof(buf))

char *lgtd_print_duration(uint64_t, char *, int);
#define LGTD_PRINT_DURATION(secs, arr) \
    lgtd_print_duration((secs), (arr), sizeof((arr)))
char* lgtd_print_nsec_timestamp(uint64_t, char *, int);
#define LGTD_LIFX_WIRE_PRINT_NSEC_TIMESTAMP(ts, arr) \
    lgtd_print_nsec_timestamp((ts), (arr), sizeof((arr)))

void lgtd_log_setup(void);

void lgtd_err(int, const char *, ...)
    __attribute__((format(printf, 2, 3), noreturn));
void lgtd_errx(int, const char *, ...)
    __attribute__((format(printf, 2, 3), noreturn));
void lgtd_warn(const char *, ...) __attribute__((format(printf, 1, 2)));
void lgtd_warnx(const char *, ...) __attribute__((format(printf, 1, 2)));
void lgtd_info(const char *, ...) __attribute__((format(printf, 1, 2)));
void lgtd_debug(const char *, ...) __attribute__((format(printf, 1, 2)));

void lgtd_cleanup(void);
