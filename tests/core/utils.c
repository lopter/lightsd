#include <stdbool.h>

#include <event2/event.h>

#include "lightsd.h"

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
