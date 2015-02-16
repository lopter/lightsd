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

#include "lifx/wire_proto.h"
#include "time_monotonic.h"
#include "lifx/bulb.h"
#include "lifx/gateway.h"
#include "lifx/broadcast.h"
#include "lifx/timer.h"
#include "version.h"
#include "jsmn.h"
#include "client.h"
#include "listen.h"
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
}

static void
lgtd_usage(const char *progname)
{
    printf(
        "Usage: %s -l addr:port [-l ...] [-f] [-t] [-h] [-V] "
        "[-v debug|info|warning|error]\n",
        progname
    );
    exit(0);
}

int
main(int argc, char *argv[])
{
    lgtd_configure_libevent();
    lgtd_configure_signal_handling();

    static const struct option long_opts[] = {
        {"listen",          required_argument, NULL, 'l'},
        {"foreground",      no_argument,       NULL, 'f'},
        {"no-timestamps",   no_argument,       NULL, 't'},
        {"help",            no_argument,       NULL, 'h'},
        {"verbosity",       required_argument, NULL, 'v'},
        {"version",         no_argument,       NULL, 'V'},
        {NULL,              0,                 NULL, 0}
    };
    const char short_opts[] = "l:fthv:V";

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
            printf("%s v%s\n", argv[0], LGTD_VERSION);
            return 0;
        default:
            lgtd_usage(argv[0]);
        }
    }

    argc -= optind;
    argv += optind;

    lgtd_lifx_wire_load_packet_infos_map();
    if (!lgtd_lifx_timer_setup() || !lgtd_lifx_broadcast_setup()) {
        lgtd_err(1, "can't setup lightsd");
    }

    lgtd_lifx_timer_start_discovery();

    event_base_dispatch(lgtd_ev_base);

    lgtd_cleanup();

    return 0;
}
