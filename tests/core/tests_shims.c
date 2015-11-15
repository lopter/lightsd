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

const int LGTD_LIFX_DEBRUIJN_SEQUENCE[64] = {
    0, 47,  1, 56, 48, 27,  2, 60,
   57, 49, 41, 37, 28, 16,  3, 61,
   54, 58, 35, 52, 50, 42, 21, 44,
   38, 32, 29, 23, 17, 11,  4, 62,
   46, 55, 26, 59, 40, 36, 15, 53,
   34, 51, 20, 43, 31, 22, 10, 45,
   25, 39, 14, 33, 19, 30,  9, 24,
   13, 18,  8, 12,  7,  6,  5, 63
};

void
lgtd_cleanup(void)
{
}
