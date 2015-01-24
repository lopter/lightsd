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

#pragma once

#ifndef __attribute__
# define __atttribute__(e)
#endif

#define LGTD_ABS(v) ((v) >= 0 ? (v) : (v) * -1)
#define LGTD_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define LGTD_MSECS_TO_TIMEVAL(v) { \
    .tv_sec = (v) / 1000,           \
    .tv_usec = ((v) % 1000) * 1000  \
}

enum lgtd_verbosity {
    LGTD_DEBUG = 0,
    LGTD_INFO,
    LGTD_WARN,
    LGTD_ERR
};

enum { LGTD_ERROR_MSG_BUFSIZE = 2048 };

struct lgtd_opts {
    bool                    foreground;
    bool                    log_timestamps;
    enum lgtd_verbosity    verbosity;
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
