// Copyright (c) 2015, Louis Opter <kalessin@kalessin.fr>
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
#include <sys/time.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "console.h"
#include "lightsd.h"

static void
lgtd_console_isotime_now(char *strbuf, int bufsz)
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
lgtd_console_log_header(const char *loglvl, bool showprogname)
{
    if (lgtd_opts.log_timestamps) {
        char timestr[64];
        lgtd_console_isotime_now(timestr, sizeof(timestr));
        fprintf(
            stderr, "[%s] [%s] %s",
            timestr, loglvl, showprogname ? "lightsd: " : ""
        );
        return;
    }
    fprintf(stderr, "[%s] %s", loglvl, showprogname ? "lightsd: " : "");
}

void
lgtd_console_err(int eval, const char *fmt, va_list ap)
{
    int errsave = errno;
    va_list aq;
    va_copy(aq, ap);
    // lgtd_cleanup is probably going to free some of the arguments we got, so
    // let's print to a buffer before we call err.
    char errmsg[LGTD_ERROR_MSG_BUFSIZE];
    vsnprintf(errmsg, sizeof(errmsg), fmt, aq);
    va_end(aq);
    lgtd_cleanup();
    lgtd_console_log_header("ERR", false);
    errno = errsave;
    err(eval, "%s", errmsg);
}

void
lgtd_console_errx(int eval, const char *fmt, va_list ap)
{
    va_list aq;
    va_copy(aq, ap);
    // lgtd_cleanup is probably going to free some of the arguments we got, so
    // let's print to a buffer before we call err.
    char errmsg[LGTD_ERROR_MSG_BUFSIZE];
    vsnprintf(errmsg, sizeof(errmsg), fmt, aq);
    va_end(aq);
    lgtd_cleanup();
    lgtd_console_log_header("ERR", false);
    errx(eval, "%s", errmsg);
}

void
lgtd_console_warn(const char *fmt, va_list ap)
{
    va_list aq;
    va_copy(aq, ap);
    lgtd_console_log_header("WARN", false);
    vwarn(fmt, aq);
    va_end(aq);
}

void
lgtd_console_warnx(const char *fmt, va_list ap)
{
    va_list aq;
    va_copy(aq, ap);
    lgtd_console_log_header("WARN", false);
    vwarnx(fmt, aq);
    va_end(aq);
}

void
lgtd_console_info(const char *fmt, va_list ap)
{
    va_list aq;
    va_copy(aq, ap);
    lgtd_console_log_header("INFO", true);
    vfprintf(stderr, fmt, aq);
    va_end(aq);
    fprintf(stderr, "\n");
}

void
lgtd_console_debug(const char *fmt, va_list ap)
{
    va_list aq;
    va_copy(aq, ap);
    lgtd_console_log_header("DEBUG", true);
    vfprintf(stderr, fmt, aq);
    va_end(aq);
    fprintf(stderr, "\n");
}
