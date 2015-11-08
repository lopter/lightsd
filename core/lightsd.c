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
#include <syslog.h>
#include <unistd.h>

#include <event2/event.h>
#include <event2/event_struct.h>

#include "lifx/wire_proto.h"
#include "time_monotonic.h"
#include "lifx/bulb.h"
#include "lifx/gateway.h"
#include "lifx/broadcast.h"
#include "lifx/discovery.h"
#include "version.h"
#include "jsmn.h"
#include "jsonrpc.h"
#include "client.h"
#include "pipe.h"
#include "timer.h"
#include "listen.h"
#include "daemon.h"
#include "lightsd.h"

struct lgtd_opts lgtd_opts = {
    .foreground = true,
    .log_timestamps = true,
#ifndef NDEBUG
    .verbosity = LGTD_INFO,
#else
    .verbosity = LGTD_WARN,
#endif
    .user = NULL,
    .group = NULL,
    .syslog = false,
    .syslog_facility = LOG_DAEMON,
    .syslog_ident = "lightsd",
    .pidfile = NULL
}; 

struct event_base *lgtd_ev_base = NULL;

static int lgtd_last_signal_received = 0;

void
lgtd_cleanup(void)
{
    lgtd_lifx_discovery_close();
    lgtd_listen_close_all();
    lgtd_command_pipe_close_all();
    lgtd_client_close_all();
    lgtd_lifx_broadcast_close();
    lgtd_lifx_gateway_close_all();
    lgtd_timer_stop_all();
    event_base_free(lgtd_ev_base);
#if LIBEVENT_VERSION_NUMBER >= 0x02010100
    libevent_global_shutdown();
#endif
    if (lgtd_opts.pidfile) {
        unlink(lgtd_opts.pidfile);
    }
}

static void
lgtd_signal_event_callback(int signum, short events, void *ctx)
{
    assert(ctx);

    // NOTE: syslog isn't signal safe, don't log anything in this function.

    lgtd_last_signal_received = signum;
    event_del((struct event *)ctx);  // restore default behavior
    event_base_loopbreak(lgtd_ev_base);
    (void)events;
}

static void
lgtd_libevent_log(int severity, const char *msg)
{
    switch (severity) {
    case EVENT_LOG_DEBUG:   lgtd_debug("%s", msg); break;
    case EVENT_LOG_MSG:     lgtd_info("%s", msg);  break;
    case EVENT_LOG_WARN:    lgtd_warnx("%s", msg); break;
    case EVENT_LOG_ERR:     lgtd_warnx("%s", msg); break;
    default:                                       break;
    }
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
"  [-l,--listen addr:port]              Listen for JSON-RPC commands over TCP at\n"
"                                       this address (can be repeated).\n"
"  [-c,--command-pipe /command/fifo]    Open an unidirectional JSON-RPC\n"
"                                       command pipe at this location (can be\n"
"                                       repeated).\n"
"  [-s,--socket /unix/socket]           Open an Unix socket at this location\n"
"                                       (can be repeated).\n"
"  [-d,--daemonize]                     Fork in the background.\n"
"  [-p,--pidfile /path/to/pid.file]     Write lightsd's pid in the given file.\n"
"  [-u,--user user]                     Drop privileges to this user (and the\n"
"                                       group of this user if -g is missing).\n"
"  [-g,--group group]                   Drop privileges to this group (-g requires\n"
"                                       the -u option to be used).\n"
"  [-S,--syslog]                        Divert logging from the console to syslog.\n"
"  [-F,--syslog-facility]               Facility to use with syslog (defaults to\n"
"                                       daemon, other possible values are user and\n"
"                                       local0-7, see syslog(3)).\n"
"  [-I,--syslog-ident]                  Identifier to use with syslog (defaults to\n"
"                                       lightsd).\n"
"  [-t,--no-timestamps]                 Disable timestamps in the console logs.\n"
"  [-h,--help]                          Display this.\n"
"  [-V,--version]                       Display version and build information.\n"
"  [-v,--verbosity debug|info|warning|error]\n"
"\nor,\n\n"
"  --prefix                             Display the install prefix for lightsd.\n"
"\nor,\n\n"
"  --rundir                             Display the runtime directory for lightsd.\n",
        progname
    );
    lgtd_cleanup();
    exit(0);
}

int
main(int argc, char *argv[], char *envp[])
{
    char progname[32] = { 0 };
    memcpy(progname, argv[0], LGTD_MIN(sizeof(progname) - 1, strlen(argv[0])));

    lgtd_daemon_setup_proctitle(argc, argv, envp);

    lgtd_configure_libevent();
    lgtd_configure_signal_handling();

    static const struct option long_opts[] = {
        {"listen",          required_argument, NULL, 'l'},
        {"command-pipe",    required_argument, NULL, 'c'},
        {"socket",          required_argument, NULL, 's'},
        {"foreground",      no_argument,       NULL, 'f'},
        {"daemonize",       no_argument,       NULL, 'd'},
        {"pidfile",         required_argument, NULL, 'p'},
        {"user",            required_argument, NULL, 'u'},
        {"group",           required_argument, NULL, 'g'},
        {"syslog",          no_argument,       NULL, 'S'},
        {"syslog-facility", required_argument, NULL, 'F'},
        {"syslog-ident",    required_argument, NULL, 'I'},
        {"no-timestamps",   no_argument,       NULL, 't'},
        {"help",            no_argument,       NULL, 'h'},
        {"verbosity",       required_argument, NULL, 'v'},
        {"version",         no_argument,       NULL, 'V'},
        {"prefix",          no_argument,       NULL, 'P'},
        {"rundir",          no_argument,       NULL, 'r'},
        {NULL,              0,                 NULL, 0}
    };
    const char short_opts[] = "l:c:s:fdp:u:g:SF:I:thv:V";

    if (argc == 1) {
        lgtd_usage(progname);
    }

    for (int rv = getopt_long(argc, argv, short_opts, long_opts, NULL);
         rv != -1;
         rv = getopt_long(argc, argv, short_opts, long_opts, NULL)) {
        switch (rv) {
        case 'l':
            (void)0;
            char *sep = strrchr(optarg, ':');
            if (!sep || !sep[1]) {
                lgtd_usage(progname);
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
        case 's':
            if (!lgtd_listen_unix_open(optarg)) {
                exit(1);
            }
            break;
        case 'f':
            lgtd_opts.foreground = true;
            break;
        case 'd':
            lgtd_opts.foreground = false;
            break;
        case 'p':
            lgtd_opts.pidfile = optarg;
            break;
        case 'u':
            lgtd_opts.user = optarg;
            break;
        case 'g':
            lgtd_opts.group = optarg;
            break;
        case 'S':
            lgtd_opts.syslog = true;
            break;
        case 'F':
            lgtd_opts.syslog_facility = lgtd_daemon_syslog_facilitytoi(optarg);
            break;
        case 'I':
            lgtd_opts.syslog_ident = optarg;
            break;
        case 't':
            lgtd_opts.log_timestamps = false;
            break;
        case 'h':
            lgtd_usage(progname);
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
            printf("%s %s\n", progname, LGTD_VERSION);
            lgtd_cleanup();
            return 0;
        case 'P':
            printf(
                "%s%s\n", LGTD_INSTALL_PREFIX, LGTD_INSTALL_PREFIX[
                    LGTD_ARRAY_SIZE(LGTD_INSTALL_PREFIX) - 1
                ] != '/' ?  "/" : ""
            );
            return 0;
        case 'r':
            printf(
                "%s%s\n", LGTD_RUNTIME_DIRECTORY, LGTD_RUNTIME_DIRECTORY[
                    LGTD_ARRAY_SIZE(LGTD_RUNTIME_DIRECTORY) - 1
                ] != '/' ?  "/" : ""
            );
            return 0;
        default:
            lgtd_usage(progname);
        }
    }

    argc -= optind;
    argv += optind;

    // Ideally we should parse the syslog relation options first and call that
    // before anything can be logged:
    lgtd_log_setup();

    if (lgtd_opts.user) {
        lgtd_daemon_set_user(lgtd_opts.user);
        lgtd_daemon_set_group(lgtd_opts.group);
        // create the pidfile before we drop privileges:
        if (lgtd_opts.pidfile
            && !lgtd_daemon_write_pidfile(lgtd_opts.pidfile)) {
            lgtd_warn("couldn't write pidfile at %s", lgtd_opts.pidfile);
        }
        lgtd_daemon_drop_privileges();
    } else if (lgtd_opts.group) {
        lgtd_errx(1, "please, specify an user with the -u option");
    }

    lgtd_daemon_die_if_running_as_root_unless_requested(lgtd_opts.user);

    lgtd_lifx_wire_load_packet_info_map();
    if (!lgtd_lifx_discovery_setup() || !lgtd_lifx_broadcast_setup()) {
        lgtd_err(1, "can't setup lightsd");
    }

    if (!lgtd_opts.foreground) {
        lgtd_info("forking into the background now...");
        if (!lgtd_daemon_unleash()) {
            lgtd_err(1, "can't fork to the background");
        }
    }

    // update the pidfile after we've forked:
    if (lgtd_opts.pidfile && !lgtd_daemon_write_pidfile(lgtd_opts.pidfile)) {
        lgtd_warn("couldn't write pidfile at %s", lgtd_opts.pidfile);
    }

    lgtd_lifx_discovery_start();

    // update at least once: so that if no bulbs are discovered we still get a
    // clear status line.
    lgtd_daemon_update_proctitle();

    event_base_dispatch(lgtd_ev_base);

    if (lgtd_last_signal_received) {
        lgtd_info(
            "received signal %d (%s), exiting...",
            lgtd_last_signal_received, strsignal(lgtd_last_signal_received)
        );
    }

    lgtd_cleanup();

    return 0;
}
