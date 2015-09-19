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

void
lgtd_daemon_update_proctitle(void)
{
}
