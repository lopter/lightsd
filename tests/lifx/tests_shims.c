#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "core/lightsd.h"

struct lgtd_opts lgtd_opts = {
    .foreground = false,
    .log_timestamps = false,
    .verbosity = LGTD_DEBUG
};

struct event_base *lgtd_ev_base = NULL;

void
lgtd_cleanup(void)
{
}

short
lgtd_sockaddrport(const struct sockaddr_storage *peer)
{
    if (!peer) {
        return -1;
    }

    if (peer->ss_family == AF_INET) {
        const struct sockaddr_in *in_peer = (const struct sockaddr_in *)peer;
        return ntohs(in_peer->sin_port);
    } else {
        const struct sockaddr_in6 *in6_peer = (const struct sockaddr_in6 *)peer;
        return ntohs(in6_peer->sin6_port);
    }
}

void
lgtd_daemon_update_proctitle(void)
{
}
