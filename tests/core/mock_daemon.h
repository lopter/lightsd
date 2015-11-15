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

#ifndef MOCKED_DAEMON_RANDUINT32
uint32_t
lgtd_daemon_randuint32(void)
{
    return 0x72616e64;
}
#endif
