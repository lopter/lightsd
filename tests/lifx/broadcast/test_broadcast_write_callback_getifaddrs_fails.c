#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>

int mock_getifaddrs(struct ifaddrs **);
void mock_freeifaddrs(struct ifaddrs *);
ssize_t mock_sendto(int, const void *, size_t, int,
                    const struct sockaddr *, socklen_t);

#define getifaddrs(ifap) mock_getifaddrs(ifap)
#define freeifaddrs(ifp) mock_freeifaddrs(ifp)
#define sendto(socket, buffer, length, flags, dest_addr, dest_len) \
    mock_sendto(socket, buffer, length, flags, dest_addr, dest_len)

#include "broadcast.c"

#include "mock_bulb.h"
#define MOCKED_EVENT_DEL
#include "mock_event2.h"
#include "mock_gateway.h"
#include "mock_log.h"
#include "mock_tagging.h"
#define MOCKED_LGTD_LIFX_WIRE_SETUP_HEADER
#include "mock_wire_proto.h"

#include "tests_utils.h"

enum { MOCK_SOCKET_FD = 32 };
static struct event *MOCK_WRITE_EV = (struct event *)0x7061726973;
static const struct sockaddr_in LIFX_BROADCAST_ADDR = {
    .sin_family = AF_INET,
    .sin_addr = { INADDR_BROADCAST },
    .sin_port = LGTD_STATIC_HTONS(LGTD_LIFX_PROTOCOL_PORT),
    .sin_zero = { 0 }
};

static int event_del_call_count = 0;

int
event_del(struct event *ev)
{
    if (ev != MOCK_WRITE_EV) {
        lgtd_errx(
            1, "event_del received invalid event=%p (expected %p)",
            ev, MOCK_WRITE_EV
        );
    }

    event_del_call_count++;

    return 0;
}

int
mock_getifaddrs(struct ifaddrs **ifap)
{
    if (!ifap) {
        lgtd_errx(1, "ifap souldn't be NULL");
    }

    errno = ENOSYS;

    return -1;
}

void
mock_freeifaddrs(struct ifaddrs *ifp)
{
    (void)ifp;

    lgtd_errx(1, "freeifaddrs shouldn't have been called");
}

static int mock_lifx_wire_setup_header_call_count = 0;

const struct lgtd_lifx_packet_info *
lgtd_lifx_wire_setup_header(struct lgtd_lifx_packet_header *hdr,
                            enum lgtd_lifx_target_type target_type,
                            union lgtd_lifx_target target,
                            const uint8_t *site,
                            enum lgtd_lifx_packet_type packet_type)
{
    if (!hdr) {
        lgtd_errx(1, "missing header");
    }

    if (target_type != LGTD_LIFX_TARGET_ALL_DEVICES) {
        lgtd_errx(
            1, "got target type %d (expected %d)",
            target_type, LGTD_LIFX_TARGET_ALL_DEVICES
        );
    }

    if (memcmp(&target, &LGTD_LIFX_UNSPEC_TARGET, sizeof(target))) {
        lgtd_errx(
            1, "got unexpected target (expected LGTD_LIFX_UNSPEC_TARGET)"
        );
    }

    if (site) {
        lgtd_errx(1, "got unexpected site %p (expected NULL)", site);
    }

    if (packet_type != LGTD_LIFX_GET_PAN_GATEWAY) {
        lgtd_errx(
            1, "got unexpected packet type %d (expected %d)",
            packet_type, LGTD_LIFX_GET_PAN_GATEWAY
        );
    }

    mock_lifx_wire_setup_header_call_count++;

    return NULL;
}

enum mock_sendto_calls {
    MOCK_SENDTO_OK = 0,
    MOCK_SENDTO_ERROR,
};

static int mock_sendto_call_count = 0;

ssize_t
mock_sendto(int socket,
            const void *buffer,
            size_t length,
            int flags,
            const struct sockaddr *dest_addr,
            socklen_t dest_len)
{
    if (socket != MOCK_SOCKET_FD) {
        lgtd_errx(
            1, "got socket=%jd (expected %d)", (intmax_t)socket, MOCK_SOCKET_FD
        );
    }

    if (!buffer) {
        lgtd_errx(1, "missing buffer");
    }

    if (length != sizeof(struct lgtd_lifx_packet_header)) {
        lgtd_errx(
            1, "got buffer length=%ju (expected %ju)",
            (uintmax_t)length, (uintmax_t)sizeof(struct lgtd_lifx_packet_header)
        );
    }

    if (flags) {
        lgtd_errx(1, "got flags=%#x (expected 0x0)", flags);
    }

    lgtd_tests_check_sockaddr_in(
        dest_addr, dest_len, AF_INET, INADDR_BROADCAST, LGTD_LIFX_PROTOCOL_PORT
    );

    switch (mock_sendto_call_count++) {
    case MOCK_SENDTO_OK:
        return length;
    case MOCK_SENDTO_ERROR:
        errno = EIO;
        return -1;
    default:
        lgtd_errx(1, "sendto was called too many times");
    }
}

int
main(void)
{
    lgtd_lifx_broadcast_endpoint.socket = MOCK_SOCKET_FD;
    lgtd_lifx_broadcast_endpoint.write_ev = MOCK_WRITE_EV;

    bool ok;

    // getifaddrs fails
    ok = lgtd_lifx_broadcast_handle_write();
    if (!ok) {
        lgtd_errx(1, "write callback returned false (expected true)");
    }
    if (mock_lifx_wire_setup_header_call_count != 1) {
        lgtd_errx(
            1, "mock_lifx_wire_setup_header_call_count=%d (expected 1)",
            mock_lifx_wire_setup_header_call_count
        );
    }
    if (mock_sendto_call_count != 1) {
        lgtd_errx(
            1, "mock_sendto_call_count=%d (expected 1)",
            mock_sendto_call_count
        );
    }
    if (event_del_call_count != 1) {
        lgtd_errx(
            1, "event_del_call_count=%d (expected 1)", event_del_call_count
        );
    }

    // getifaddrs & lgtd_lifx_broadcast_send_packet fail
    ok = lgtd_lifx_broadcast_handle_write();
    if (ok) {
        lgtd_errx(1, "write callback returned true (expected false)");
    }
    if (mock_lifx_wire_setup_header_call_count != 2) {
        lgtd_errx(
            1, "mock_lifx_wire_setup_header_call_count=%d (expected 2)",
            mock_lifx_wire_setup_header_call_count
        );
    }
    if (mock_sendto_call_count != 2) {
        lgtd_errx(
            1, "mock_sendto_call_count=%d (expected 2)",
            mock_sendto_call_count
        );
    }
    if (event_del_call_count != 1) {
        lgtd_errx(
            1, "event_del_call_count=%d (expected 1)", event_del_call_count
        );
    }

    return 0;
}
