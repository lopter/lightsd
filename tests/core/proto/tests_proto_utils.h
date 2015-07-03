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

#ifndef MOCKED_CLIENT_SEND_ERROR
void
lgtd_client_send_error(struct lgtd_client *client,
                       enum lgtd_client_error_code error,
                       const char *msg)
{
    (void)client;
    (void)error;
    (void)msg;
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

#ifndef MOCKED_ROUTER_TARGETS_TO_DEVICES
struct lgtd_router_device_list *
lgtd_router_targets_to_devices(const struct lgtd_proto_target_list *targets)
{
    (void)targets;
    return NULL;
}
#endif

#ifndef MOCKED_ROUTER_DEVICE_LIST_FREE
void
lgtd_router_device_list_free(struct lgtd_router_device_list *devices)
{
    (void)devices;
}
#endif
