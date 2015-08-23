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

#include <sys/queue.h>
#include <sys/tree.h>
#include <arpa/inet.h>
#include <assert.h>
#include <endian.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <strings.h>

#include <event2/event.h>
#include <event2/event_struct.h>

#include "lifx/wire_proto.h"
#include "time_monotonic.h"
#include "lifx/bulb.h"
#include "lifx/gateway.h"
#include "lifx/broadcast.h"
#include "lifx/timer.h"
#include "version.h"
#include "jsmn.h"
#include "jsonrpc.h"
#include "client.h"
#include "pipe.h"
#include "listen.h"
#include "daemon.h"
#include "lightsd.h"

struct lgtd_opts lgtd_opts = {
    .foreground = false,
    .log_timestamps = true,
    .verbosity = LGTD_DEBUG
}; 

struct event_base *lgtd_ev_base = NULL;

void
lgtd_cleanup(void)
{
    lgtd_listen_close_all();
    lgtd_command_pipe_close_all();
    lgtd_client_close_all();
    lgtd_lifx_timer_close();
    lgtd_lifx_broadcast_close();
    lgtd_lifx_gateway_close_all();
    event_base_free(lgtd_ev_base);
#if LIBEVENT_VERSION_NUMBER >= 0x02010100
    libevent_global_shutdown();
#endif
}

short
lgtd_sockaddrport(const struct sockaddr_storage *peer)
{
    assert(peer);

    if (peer->ss_family == AF_INET) {
        const struct sockaddr_in *in_peer = (const struct sockaddr_in *)peer;
        return ntohs(in_peer->sin_port);
    } else {
        const struct sockaddr_in6 *in6_peer = (const struct sockaddr_in6 *)peer;
        return ntohs(in6_peer->sin6_port);
    }
}

static void
lgtd_signal_event_callback(int signum, short events, void *ctx)
{
    assert(ctx);

    lgtd_info(
        "received signal %d (%s), exiting...", signum, strsignal(signum)
    );
    event_del((struct event *)ctx);  // restore default behavior
    event_base_loopbreak(lgtd_ev_base);
    (void)events;
}

static void
lgtd_configure_libevent(void)
{
    event_set_log_callback(lgtd_libevent_log);
    lgtd_ev_base = event_base_new();
}

static void
lgtd_configure_signal_handling(void)
{
    const int signals[] = {SIGINT, SIGTERM, SIGQUIT};
    static struct event sigevs[LGTD_ARRAY_SIZE(signals)];

    for (int i = 0; i != LGTD_ARRAY_SIZE(signals); i++) {
        evsignal_assign(
            &sigevs[i],
            lgtd_ev_base,
            signals[i],
            lgtd_signal_event_callback,
            &sigevs[i]
        );
        evsignal_add(&sigevs[i], NULL);
    }

    struct sigaction act = { .sa_handler = SIG_IGN };
    if (sigaction(SIGPIPE, &act, NULL)) {
        lgtd_err(1, "can't configure signal handling");
    }
}

static void
lgtd_usage(const char *progname)
{
    printf(
        "Usage: %s ...\n\n"
        "  [-l,--listen addr:port [+]]\n"
        "  [-c,--comand-pipe /command/fifo [+]]\n"
        "  [-f,--foreground]\n"
        "  [-t,--no-timestamps]\n"
        "  [-h,--help]\n"
        "  [-V,--version]\n"
        "  [-v,--verbosity debug|info|warning|error]\n",
        progname
    );
    lgtd_cleanup();
    exit(0);
}

int
main(int argc, char *argv[], char *envp[])
{
    lgtd_daemon_setup_proctitle(argc, argv, envp);

    lgtd_configure_libevent();
    lgtd_configure_signal_handling();

    static const struct option long_opts[] = {
        {"listen",          required_argument, NULL, 'l'},
        {"command-pipe",    required_argument, NULL, 'c'},
        {"foreground",      no_argument,       NULL, 'f'},
        {"no-timestamps",   no_argument,       NULL, 't'},
        {"help",            no_argument,       NULL, 'h'},
        {"verbosity",       required_argument, NULL, 'v'},
        {"version",         no_argument,       NULL, 'V'},
        {NULL,              0,                 NULL, 0}
    };
    const char short_opts[] = "l:c:fthv:V";

    if (argc == 1) {
        lgtd_usage(argv[0]);
    }

    for (int rv = getopt_long(argc, argv, short_opts, long_opts, NULL);
         rv != -1;
         rv = getopt_long(argc, argv, short_opts, long_opts, NULL)) {
        switch (rv) {
        case 'l':
            (void)0;
            char *sep = strrchr(optarg, ':');
            if (!sep || !sep[1]) {
                lgtd_usage(argv[0]);
            }
            *sep = '\0';
            if (!lgtd_listen_open(optarg, sep + 1)) {
                exit(1);
            }
            break;
        case 'c':
            if (!lgtd_command_pipe_open(optarg)) {
                exit(1);
            }
            break;
        case 'f':
            lgtd_opts.foreground = true;
            break;
        case 't':
            lgtd_opts.log_timestamps = false;
            break;
        case 'h':
            lgtd_usage(argv[0]);
        case 'v':
            for (int i = 0;;) {
                const char *verbose_levels[] = {
                    "debug", "info", "warning", "error"
                };
                if (!strcasecmp(optarg, verbose_levels[i])) {
                    lgtd_opts.verbosity = i;
                    break;
                }
                if (++i == LGTD_ARRAY_SIZE(verbose_levels)) {
                    lgtd_errx(1, "Unknown verbosity level: %s", optarg);
                }
            }
            break;
        case 'V':
            printf("lightsd v%s\n", LGTD_VERSION);
            return 0;
        default:
            lgtd_usage(argv[0]);
        }
    }

    argc -= optind;
    argv += optind;

    lgtd_lifx_wire_load_packet_info_map();
    if (!lgtd_lifx_timer_setup() || !lgtd_lifx_broadcast_setup()) {
        lgtd_err(1, "can't setup lightsd");
    }

    if (!lgtd_opts.foreground && !lgtd_daemon_unleash()) {
        lgtd_err(1, "can't fork to the background");
    }

    lgtd_lifx_timer_start_discovery();

    event_base_dispatch(lgtd_ev_base);

    lgtd_cleanup();

    return 0;
}
