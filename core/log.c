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

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <event2/event.h>

#include "wire_proto.h"
#include "lifxd.h"

const char *
lifxd_addrtoa(const uint8_t *addr)
{
    assert(addr);

    static char str[LIFXD_ADDR_LENGTH * 2 + LIFXD_ADDR_LENGTH - 1 + 1];
    snprintf(
        str, sizeof(str), "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
        addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]
    );
    return str;
}

void
_lifxd_err(void (*errfn)(int, const char *, ...),
           int eval,
           const char *fmt,
           ...)
{
    int errsave = errno;
    va_list ap;
    va_start(ap, fmt);
    // lifxd_cleanup is probably going to free some of the arguments we got, so
    // let's print to a buffer before we call err.
    char errmsg[LIFXD_ERROR_MSG_BUFSIZE];
    vsnprintf(errmsg, sizeof(errmsg), fmt, ap);
    va_end(ap);
    lifxd_cleanup();
    fputs("[ERR]   ", stderr);
    errno = errsave;
    errfn(eval, errmsg);
}

void
_lifxd_warn(void (*warnfn)(const char *, va_list), const char *fmt, ...)
{
    if (lifxd_opts.verbosity <= LIFXD_WARN) {
        va_list ap;
        va_start(ap, fmt);
        fputs("[WARN]  ", stderr);
        warnfn(fmt, ap);
        va_end(ap);
    }
}

void
lifxd_info(const char *fmt, ...)
{
    if (lifxd_opts.verbosity <= LIFXD_INFO) {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stderr, "[INFO]  lifxd: ");
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        fprintf(stderr, "\n");
    }
}

void
lifxd_debug(const char *fmt, ...)
{
    if (lifxd_opts.verbosity <= LIFXD_DEBUG) {
        va_list ap;
        va_start(ap, fmt);
        fprintf(stderr, "[DEBUG] lifxd: ");
        vfprintf(stderr, fmt, ap);
        va_end(ap);
        fprintf(stderr, "\n");
    }
}

void
lifxd_libevent_log(int severity, const char *msg)
{
    switch (severity) {
    case EVENT_LOG_DEBUG:   lifxd_debug(msg);   break;
    case EVENT_LOG_MSG:     lifxd_info(msg);    break;
    case EVENT_LOG_WARN:    lifxd_warnx(msg)    break;
    case EVENT_LOG_ERR:     lifxd_warnx(msg);   break;
    default:                                    break;
    }
}
