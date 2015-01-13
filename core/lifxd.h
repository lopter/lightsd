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

#define LIFXD_ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#define LIFXD_MSECS_TO_TIMEVAL(v) { \
    .tv_sec = (v) / 1000,           \
    .tv_usec = ((v) % 1000) * 1000  \
}

enum lifxd_verbosity {
    LIFXD_DEBUG = 0,
    LIFXD_INFO,
    LIFXD_WARN,
    LIFXD_ERR
};

enum { LIFXD_ERROR_MSG_BUFSIZE = 2048 };

struct lifxd_opts {
    bool                    foreground;
    bool                    log_timestamps;
    uint16_t                master_port;
    enum lifxd_verbosity    verbosity;
};

extern struct lifxd_opts lifxd_opts;
extern struct event_base *lifxd_ev_base;

const char *lifxd_addrtoa(const uint8_t *);
void lifxd_sockaddrtoa(const struct sockaddr_storage *, char *buf, int buflen);
short lifxd_sockaddrport(const struct sockaddr_storage *);

void _lifxd_err(void (*)(int, const char *, ...), int, const char *, ...);
#define lifxd_err(eval, fmt, ...) _lifxd_err(err, (eval), (fmt), ##__VA_ARGS__);
#define lifxd_errx(eval, fmt, ...) _lifxd_err(errx, (eval), (fmt), ##__VA_ARGS__);
void _lifxd_warn(void (*)(const char *, va_list), const char *, ...);
#define lifxd_warn(fmt, ...) _lifxd_warn(vwarn, (fmt), ##__VA_ARGS__);
#define lifxd_warnx(fmt, ...) _lifxd_warn(vwarnx, (fmt), ##__VA_ARGS__);
void lifxd_info(const char *, ...);
void lifxd_debug(const char *, ...);
void lifxd_libevent_log(int, const char *);

void lifxd_cleanup(void);
