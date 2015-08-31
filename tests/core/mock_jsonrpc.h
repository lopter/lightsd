#pragma once

#ifndef MOCKED_JSONRPC_DISPATCH_REQUEST
void
lgtd_jsonrpc_dispatch_request(struct lgtd_client *client, int parsed)
{
    (void)client;
    (void)parsed;
}
#endif

#ifndef MOCKED_JSONRPC_SEND_ERROR
void
lgtd_jsonrpc_send_error(struct lgtd_client *client,
                        enum lgtd_jsonrpc_error_code code,
                        const char *msg)
{
    (void)client;
    (void)code;
    (void)msg;
}
#endif

#ifndef MOCKED_JSONRPC_SEND_RESPONSE
void
lgtd_jsonrpc_send_response(struct lgtd_client *client, const char *msg)
{
    (void)client;
    (void)msg;
}
#endif

#ifndef MOCKED_JSONRPC_START_SEND_RESPONSE
void
lgtd_jsonrpc_start_send_response(struct lgtd_client *client)
{
    (void)client;
}
#endif

#ifndef MOCKED_JSONRPC_END_SEND_RESPONSE
void
lgtd_jsonrpc_end_send_response(struct lgtd_client *client)
{
    (void)client;
}
#endif
