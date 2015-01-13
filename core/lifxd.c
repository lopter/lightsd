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

#include <sys/queue.h>
#include <sys/tree.h>
#include <arpa/inet.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <event2/event.h>
#include <event2/event_struct.h>

#include "wire_proto.h"
#include "time_monotonic.h"
#include "bulb.h"
#include "gateway.h"
#include "broadcast.h"
#include "version.h"
#include "timer.h"
#include "lifxd.h"

struct lifxd_opts lifxd_opts = {
    .foreground = false,
    .log_timestamps = true,
    .master_port = 56700,
    .verbosity = LIFXD_DEBUG
}; 

struct event_base *lifxd_ev_base = NULL;

void
lifxd_cleanup(void)
{
    lifxd_timer_close();
    lifxd_broadcast_close();
    lifxd_gateway_close_all();
    event_base_free(lifxd_ev_base);
#if LIBEVENT_VERSION_NUMBER >= 0x02010100
    libevent_global_shutdown();
#endif
}

short
lifxd_sockaddrport(const struct sockaddr_storage *peer)
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
lifxd_signal_event_callback(int signum, short events, void *ctx)
{
    assert(ctx);

    lifxd_info(
        "received signal %d (%s), exiting...", signum, strsignal(signum)
    );
    event_del((struct event *)ctx);  // restore default behavior
    event_base_loopbreak(lifxd_ev_base);
    (void)events;
}

static void
lifxd_configure_libevent(void)
{
    lifxd_gateway_close_all();
    event_set_log_callback(lifxd_libevent_log);
    lifxd_ev_base = event_base_new();
}

static void
lifxd_configure_signal_handling(void)
{
    const int signals[] = {SIGINT, SIGTERM, SIGQUIT};
    static struct event sigevs[LIFXD_ARRAY_SIZE(signals)];

    for (int i = 0; i != LIFXD_ARRAY_SIZE(signals); i++) {
        evsignal_assign(
            &sigevs[i],
            lifxd_ev_base,
            signals[i],
            lifxd_signal_event_callback,
            &sigevs[i]
        );
        evsignal_add(&sigevs[i], NULL);
    }
}

static void
lifxd_usage(const char *progname)
{
    printf(
        "Usage: %s [-p master_bulb_port] "
        "[-v debug|info|warning|error] [-f] [-t] [-h] [-V]\n",
        progname
    );
    exit(0);
}

int
main(int argc, char *argv[])
{
    static const struct option long_opts[] = {
        {"foreground",      no_argument,       NULL, 'f'},
        {"no-timestamps",   no_argument,       NULL, 't'},
        {"help",            no_argument,       NULL, 'h'},
        {"master-port",     required_argument, NULL, 'p'},
        {"verbosity",       required_argument, NULL, 'v'},
        {"version",         no_argument,       NULL, 'V'},
        {NULL,              0,                 NULL, 0}
    };
    const char short_opts[] = "fthp:v:V";

    for (int rv = getopt_long(argc, argv, short_opts, long_opts, NULL);
         rv != -1;
         rv = getopt_long(argc, argv, short_opts, long_opts, NULL)) {
        switch (rv) {
        case 'f':
            lifxd_opts.foreground = true;
            break;
        case 't':
            lifxd_opts.log_timestamps = false;
            break;
        case 'h':
            lifxd_usage(argv[0]);
        case 'p':
            errno = 0;
            long port = strtol(optarg, NULL, 10);
            if (!errno && port <= UINT16_MAX && port > 0) {
                lifxd_opts.master_port = port;
                break;
            }
            lifxd_errx(
                1, "The master port must be between 1 and %d", UINT16_MAX
            );
        case 'v':
            for (int i = 0;;) {
                const char *verbose_levels[] = {
                    "debug", "info", "warning", "error"
                };
                if (!strcasecmp(optarg, verbose_levels[i])) {
                    lifxd_opts.verbosity = i;
                    break;
                }
                if (++i == LIFXD_ARRAY_SIZE(verbose_levels)) {
                    lifxd_errx(1, "Unknown verbosity level: %s", optarg);
                }
            }
            break;
        case 'V':
            printf("%s v%s\n", argv[0], LIFXD_VERSION);
            return 0;
        default:
            lifxd_usage(argv[0]);
        }
    }

    argc -= optind;
    argv += optind;

    lifxd_configure_libevent();
    lifxd_configure_signal_handling();

    lifxd_wire_load_packet_infos_map();
    if (!lifxd_timer_setup() || !lifxd_broadcast_setup()) {
        lifxd_err(1, "can't setup lifxd");
    }

    lifxd_timer_start_discovery();

    event_base_dispatch(lifxd_ev_base);

    lifxd_cleanup();

    return 0;
}
