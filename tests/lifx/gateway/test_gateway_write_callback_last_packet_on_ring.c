#include "gateway.c"

#define MOCKED_EVBUFFER_WRITE_ATMOST
#define MOCKED_EVBUFFER_GET_LENGTH
#include "test_gateway_utils.h"
#include "mock_log.h"
#include "mock_timer.h"
#include "mock_wire_proto.h"

size_t
evbuffer_get_length(const struct evbuffer *buf)
{
    if (buf != (void *)42) {
        errx(1, "didn't get the expected evbuffer");
    }

    return 0;
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

    int expected = sizeof(struct lgtd_lifx_packet_header);
    expected += sizeof(struct lgtd_lifx_packet_power_state);
    if (howmuch != expected) {
        errx(
            1, "evbuffer_write_atmost expected %d but got %jd",
            expected, (intmax_t)howmuch
        );
    }

    return howmuch;
}

int
main(void)
{
    lgtd_lifx_wire_setup();

    struct lgtd_lifx_gateway gw;
    memset(&gw, 0, sizeof(gw));

    // fake some values:
    gw.socket = 25;
    gw.write_ev = (void *)21;
    gw.write_buf = (void *)42;

    gw.pkt_ring[0].size += sizeof(struct lgtd_lifx_packet_header);
    gw.pkt_ring[0].size += sizeof(struct lgtd_lifx_packet_power_state);
    gw.pkt_ring[0].type = LGTD_LIFX_SET_POWER_STATE;
    gw.pkt_ring_head++;
    gw.pkt_ring_head++;

    lgtd_lifx_gateway_write_callback(-1, EV_WRITE, &gw);

    if (gw.pkt_ring[0].size != 0 || gw.pkt_ring[0].type != 0) {
        errx(1, "the ring entry should have been reset");
    }

    if (gw.pkt_ring_tail != 1) {
        errx(1, "the tail shoud have been moved by one");
    }

    if (last_event_passed_to_event_del != (void *)21) {
        errx(1, "event_del should have ben called");
    }

    return 0;
}
