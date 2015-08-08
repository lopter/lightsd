#pragma once

#ifndef MOCKED_CLIENT_OPEN_FROM_PIPE
void
lgtd_client_open_from_pipe(struct lgtd_client *pipe_client)
{
    memset(pipe_client, 0, sizeof(*pipe_client));
    jsmn_init(&pipe_client->jsmn_ctx);
}
#endif

#ifndef MOCKED_JSONRPC_DISPATCH_REQUEST
void
lgtd_jsonrpc_dispatch_request(struct lgtd_client *client, int parsed)
{
    (void)client;
    (void)parsed;
}
#endif
