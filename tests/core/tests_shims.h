#pragma once

struct lgtd_opts lgtd_opts = {
    .foreground = false,
    .log_timestamps = false,
    .verbosity = LGTD_DEBUG
};

#define MOCK_LGTD_EV_BASE ((void *)2222)

struct event_base *lgtd_ev_base = MOCK_LGTD_EV_BASE;

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
