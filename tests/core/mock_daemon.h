#pragma once

#ifndef MOCKED_DAEMON_UPDATE_PROCTITLE
void
lgtd_daemon_update_proctitle(void)
{
}
#endif

#ifndef MOCKED_DAEMON_MAKEDIRS
bool
lgtd_daemon_makedirs(const char *fp)
{
    (void)fp;
    return true;
}
#endif
