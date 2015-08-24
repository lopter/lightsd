#include "gateway.c"

#define MOCKED_EVBUFFER_WRITE_ATMOST
#define MOCKED_EVBUFFER_GET_LENGTH
#include "test_gateway_utils.h"
#include "mock_timer.h"

size_t
evbuffer_get_length(const struct evbuffer *buf)
{
    if (buf != (void *)42) {
        errx(1, "didn't get the expected evbuffer");
    }

    // fake another packet to write:
    return sizeof(struct lgtd_lifx_packet_header);
}

int
evbuffer_write_atmost(struct evbuffer *buf,
                      evutil_socket_t fd,
                      ev_ssize_t howmuch)
{
    if (fd != 25) {
        errx(1, "evbuffer_write_atmost didn't get the expected socket");
    }

    if (buf != (void *)42) {
        errx(1, "evbuffer_write_atmost didn't get the expected evbuffer");
    }

    static int expected = (
        sizeof(struct lgtd_lifx_packet_header)
        + sizeof(struct lgtd_lifx_packet_power_state)
    );
    if (howmuch != expected) {
        errx(
            1, "evbuffer_write_atmost expected %d but got %jd",
            expected, (intmax_t)howmuch
        );
    }

    if (expected != sizeof(struct lgtd_lifx_packet_power_state)) {
        expected -= sizeof(struct lgtd_lifx_packet_header);
        return sizeof(struct lgtd_lifx_packet_header);
    }

    return howmuch;
}

int
main(void)
{
    lgtd_lifx_wire_load_packet_info_map();

    struct lgtd_lifx_gateway gw;
    memset(&gw, 0, sizeof(gw));

    // fake some values:
    gw.socket = 25;
    gw.write_buf = (void *)42;

    gw.pkt_ring[0].size += sizeof(struct lgtd_lifx_packet_header);
    gw.pkt_ring[0].size += sizeof(struct lgtd_lifx_packet_power_state);
    gw.pkt_ring[0].type = LGTD_LIFX_SET_POWER_STATE;
    gw.pkt_ring_head++;
    gw.pkt_ring_head++;

    lgtd_lifx_gateway_write_callback(-1, EV_WRITE, &gw);

    if (gw.pkt_ring[0].type != LGTD_LIFX_SET_POWER_STATE) {
        errx(1, "the ring entry doesn't have the right packet type");
    }

    if (gw.pkt_ring[0].size != sizeof(struct lgtd_lifx_packet_power_state)) {
        errx(1, "the ring entry doesn't have the right size value");
    }

    if (gw.pkt_ring_tail != 0) {
        errx(1, "the tail shoudn't have been moved by one");
    }

    if (last_event_passed_to_event_del != NULL) {
        errx(1, "event_del shouldn't have ben called");
    }

    lgtd_lifx_gateway_write_callback(-1, EV_WRITE, &gw);

    if (gw.pkt_ring[0].size != 0 || gw.pkt_ring[0].type != 0) {
        errx(1, "the ring entry should have been reset");
    }

    if (gw.pkt_ring_tail != 1) {
        errx(1, "the tail shoud have been moved by one");
    }

    if (last_event_passed_to_event_del != NULL) {
        errx(1, "event_del shouldn't have ben called");
    }

    return 0;
}
