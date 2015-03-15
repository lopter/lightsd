#pragma once

void lgtd_lifx_gateway_handle_pan_gateway(struct lgtd_lifx_gateway *gw,
                                          const struct lgtd_lifx_packet_header *hdr,
                                          const struct lgtd_lifx_packet_pan_gateway *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}

void lgtd_lifx_gateway_handle_light_status(struct lgtd_lifx_gateway *gw,
                                           const struct lgtd_lifx_packet_header *hdr,
                                           const struct lgtd_lifx_packet_light_status *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}

void lgtd_lifx_gateway_handle_power_state(struct lgtd_lifx_gateway *gw,
                                          const struct lgtd_lifx_packet_header *hdr,
                                          const struct lgtd_lifx_packet_power_state *pkt)
{
    (void)gw;
    (void)hdr;
    (void)pkt;
}
