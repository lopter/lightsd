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
#include <endian.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <event2/util.h>

#include "lifx/wire_proto.h"
#include "stats.h"
#include "console.h"
#include "daemon.h"
#include "lightsd.h"

void
lgtd_log_setup(void)
{
    if (lgtd_opts.syslog) {
        lgtd_daemon_syslog_open(
            lgtd_opts.syslog_ident,
            lgtd_opts.verbosity,
            lgtd_opts.syslog_facility
        );
    }
}

#define ERRFN(fn)                               \
void                                            \
lgtd_##fn(int eval, const char *fmt, ...)       \
{                                               \
    va_list ap;                                 \
    va_start(ap, fmt);                          \
    if (lgtd_opts.syslog) {                     \
        lgtd_daemon_syslog_##fn(eval, fmt, ap); \
    } else {                                    \
        lgtd_console_##fn(eval, fmt, ap);       \
    }                                           \
    /* not reached */                           \
}

ERRFN(err);
ERRFN(errx);

#define LOGFN(level ,fn)                    \
void                                        \
lgtd_##fn(const char *fmt, ...)             \
{                                           \
    if (lgtd_opts.verbosity > (level)) {    \
        return;                             \
    }                                       \
                                            \
    va_list ap;                             \
    va_start(ap, fmt);                      \
    if (lgtd_opts.syslog) {                 \
        lgtd_daemon_syslog_##fn(fmt, ap);   \
    } else {                                \
        lgtd_console_##fn(fmt, ap);         \
    }                                       \
    va_end(ap);                             \
}

LOGFN(LGTD_WARN, warn);
LOGFN(LGTD_WARN, warnx);
LOGFN(LGTD_INFO, info);
LOGFN(LGTD_DEBUG, debug);
