#pragma once

#include "mock_daemon.h"

#ifndef MOCKED_CLIENT_OPEN_FROM_PIPE
void
lgtd_client_open_from_pipe(struct lgtd_client *pipe_client)
{
    memset(pipe_client, 0, sizeof(*pipe_client));
    jsmn_init(&pipe_client->jsmn_ctx);
}
#endif
