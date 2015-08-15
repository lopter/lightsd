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
#define LGTD_MSECS_TO_TIMEVAL(v) { \
    .tv_sec = (v) / 1000,           \
    .tv_usec = ((v) % 1000) * 1000  \
}
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

const char *lgtd_addrtoa(const uint8_t *);
void lgtd_sockaddrtoa(const struct sockaddr_storage *, char *buf, int buflen);
short lgtd_sockaddrport(const struct sockaddr_storage *);

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
