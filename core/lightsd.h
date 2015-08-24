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

#pragma once

#ifndef __attribute__
# define __atttribute__(e)
#endif

#define LGTD_ABS(v) ((v) >= 0 ? (v) : (v) * -1)
#define LGTD_MIN(a, b) ((a) < (b) ? (a) : (b))
#define LGTD_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define LGTD_MSECS_TO_TIMEVAL(v) {  \
    .tv_sec = (v) / 1000,           \
    .tv_usec = ((v) % 1000) * 1000  \
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

enum lgtd_verbosity {
    LGTD_DEBUG = 0,
    LGTD_INFO,
    LGTD_WARN,
    LGTD_ERR
};

enum { LGTD_ERROR_MSG_BUFSIZE = 2048 };

struct lgtd_opts {
    bool                foreground;
    bool                log_timestamps;
    enum lgtd_verbosity verbosity;
};

extern struct lgtd_opts lgtd_opts;
extern struct event_base *lgtd_ev_base;

char *lgtd_iee8023mactoa(const uint8_t *addr, char *buf, int buflen);
#define LGTD_IEEE8023MACTOA(addr, buf) \
    lgtd_iee8023mactoa((addr), (buf), sizeof(buf))
void lgtd_sockaddrtoa(const struct sockaddr_storage *, char *buf, int buflen);
short lgtd_sockaddrport(const struct sockaddr_storage *);

char *lgtd_print_duration(uint64_t, char *, int);
#define LGTD_PRINT_DURATION(secs, arr) \
    lgtd_print_duration((secs), (arr), sizeof((arr)))

void _lgtd_err(void (*)(int, const char *, ...), int, const char *, ...)
    __attribute__((format(printf, 3, 4)));
#define lgtd_err(eval, fmt, ...) _lgtd_err(err, (eval), (fmt), ##__VA_ARGS__);
#define lgtd_errx(eval, fmt, ...) _lgtd_err(errx, (eval), (fmt), ##__VA_ARGS__);
void _lgtd_warn(void (*)(const char *, va_list), const char *, ...)
    __attribute__((format(printf, 2, 3)));
#define lgtd_warn(fmt, ...) _lgtd_warn(vwarn, (fmt), ##__VA_ARGS__);
#define lgtd_warnx(fmt, ...) _lgtd_warn(vwarnx, (fmt), ##__VA_ARGS__);
void lgtd_info(const char *, ...) __attribute__((format(printf, 1, 2)));
void lgtd_debug(const char *, ...) __attribute__((format(printf, 1, 2)));
void lgtd_libevent_log(int, const char *);

void lgtd_cleanup(void);
