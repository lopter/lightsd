#pragma once

void
lgtd_client_start_send_response(struct lgtd_client *client)
{
    (void)client;
}

void
lgtd_client_end_send_response(struct lgtd_client *client)
{
    (void)client;
}

#ifndef MOCKED_CLIENT_SEND_RESPONSE
void
lgtd_client_send_response(struct lgtd_client *client, const char *msg)
{
    LGTD_CLIENT_WRITE_STRING(client, msg);
}
#endif

#ifndef MOCKED_ROUTER_SEND
bool
lgtd_router_send(const struct lgtd_proto_target_list *targets,
                 enum lgtd_lifx_packet_type pkt_type,
                 void *pkt)
{
    (void)targets;
    (void)pkt_type;
    (void)pkt;
    return true;
}
#endif
