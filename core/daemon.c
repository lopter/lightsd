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

#include <sys/queue.h>
#include <sys/tree.h>
#include <sys/types.h>
#include <endian.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if LGTD_HAVE_LIBBSD
#include <bsd/bsd.h>
#endif

#include <event2/util.h>

#include "time_monotonic.h"
#include "lifx/wire_proto.h"
#include "lifx/bulb.h"
#include "lifx/gateway.h"
#include "jsmn.h"
#include "jsonrpc.h"
#include "client.h"
#include "listen.h"
#include "daemon.h"
#include "pipe.h"
#include "stats.h"
#include "lightsd.h"

bool
lgtd_daemon_unleash(void)
{
    if (chdir("/")) {
        return false;
    }

    int null = open("/dev/null", O_RDWR);
    if (null == -1) {
        return false;
    }

    const int fds[] = { STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO };
    for (int i = 0; i != LGTD_ARRAY_SIZE(fds); ++i) {
        if (dup2(null, fds[i]) == -1) {
            close(null);
            return false;
        }
    }
    close(null);

#define SUMMON()  do {        \
    switch (fork()) {       \
        case 0:             \
            break;          \
        case -1:            \
            return false;   \
        default:            \
            exit(0);        \
    }                       \
} while (0)

    SUMMON(); // \_o< !
    setsid();

    SUMMON(); // \_o< !!

    return true; // welcome to UNIX!
}

void
lgtd_daemon_setup_proctitle(int argc, char *argv[], char *envp[])
{
#if LGTD_HAVE_LIBBSD
    setproctitle_init(argc, argv, envp);
    lgtd_daemon_update_proctitle();
#else
    (void)argc;
    (void)argv;
    (void)envp;
#endif
}

void
lgtd_daemon_update_proctitle(void)
{
#if LGTD_HAVE_PROCTITLE
    char title[LGTD_DAEMON_TITLE_SIZE] = { 0 };
    int i = 0;

#define TITLE_APPEND(fmt, ...) LGTD_SNPRINTF_APPEND(    \
    title, i, (int)sizeof(title), (fmt), __VA_ARGS__    \
)

#define PREFIX(fmt, ...) TITLE_APPEND(                              \
    "%s" fmt, (i && title[i - 1] == ')' ? "; " : ""), __VA_ARGS__   \
)

#define ADD_ITEM(fmt, ...) TITLE_APPEND(                            \
    "%s" fmt, (i && title[i - 1] != '(' ? ", " : ""), __VA_ARGS__   \
)
#define LOOP(list_type, list, elem_type, prefix, ...) do {    \
    if (!list_type ## _EMPTY(list)) {                         \
        PREFIX("%s(", prefix);                                \
        elem_type *it;                                        \
        list_type ## _FOREACH(it, list, link) {               \
            ADD_ITEM(__VA_ARGS__);                            \
        }                                                     \
        TITLE_APPEND("%s", ")");                              \
    }                                                         \
} while (0)

    LOOP(
        SLIST, &lgtd_listeners, struct lgtd_listen,
        "listening_on", "%s:[%s]", it->addr, it->port
    );

    LOOP(
        SLIST, &lgtd_command_pipes, struct lgtd_command_pipe,
        "command_pipes", "%s", it->path
    );

    if (!LIST_EMPTY(&lgtd_lifx_gateways)) {
        PREFIX("lifx_gateways(found=%d)", LGTD_STATS_GET(gateways));
    }

    PREFIX(
        "bulbs(found=%d, on=%d)",
        LGTD_STATS_GET(bulbs), LGTD_STATS_GET(bulbs_powered_on)
    );

    PREFIX("clients(connected=%d)", LGTD_STATS_GET(clients));

    setproctitle("%s", title);
#endif
}
