#include <sys/queue.h>
#include <sys/tree.h>
#include <sys/socket.h>
#include <assert.h>
#include <endian.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <event2/event.h>
#include <event2/util.h>

#include "lifx/wire_proto.h"
#include "core/time_monotonic.h"
#include "lifx/bulb.h"
#include "lifx/gateway.h"
#include "lifx/tagging.h"
#include "lightsd.h"

struct lgtd_opts lgtd_opts = {
    .foreground = false,
    .log_timestamps = false,
    .verbosity = LGTD_DEBUG
};

#define MOCK_LGTD_EV_BASE ((void *)2222)

struct event_base *lgtd_ev_base = MOCK_LGTD_EV_BASE;

void
lgtd_cleanup(void)
{
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
