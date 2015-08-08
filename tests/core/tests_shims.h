#pragma once

struct lgtd_opts lgtd_opts = {
    .foreground = false,
    .log_timestamps = false,
    .verbosity = LGTD_DEBUG
};

struct event_base *lgtd_ev_base = NULL;

const char *lgtd_binds = NULL;

void
lgtd_cleanup(void)
{
}

#ifndef MOCKED_DAEMON_UPDATE_PROCTITLE
void
lgtd_daemon_update_proctitle(void)
{
}
#endif
