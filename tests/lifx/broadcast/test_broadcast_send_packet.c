#include <sys/socket.h>

ssize_t mock_sendto(int, const void *, size_t, int,
                    const struct sockaddr *, socklen_t);

#define sendto(socket, buffer, length, flags, dest_addr, dest_len) \
    mock_sendto(socket, buffer, length, flags, dest_addr, dest_len)

#include "broadcast.c"

#include "mock_bulb.h"
#include "mock_event2.h"
#include "mock_gateway.h"
#include "mock_log.h"
#include "mock_tagging.h"
#include "mock_wire_proto.h"

#include "tests_utils.h"

enum { MOCK_SOCKET_FD = 32 };
static const char MOCK_PKT[] = "fake packet";
static const struct sockaddr_in MOCK_ADDR = {
    .sin_family = AF_INET,
    .sin_addr = { INADDR_BROADCAST },
    .sin_port = LGTD_STATIC_HTONS(LGTD_LIFX_PROTOCOL_PORT),
    .sin_zero = { 0 }
};

enum mock_sendto_calls {
    TEST_SENDTO_OK = 0,
    TEST_SENDTO_PARTIAL,
    TEST_SENDTO_EINTR,
    TEST_SENDTO_EINTR_OK,
    TEST_SENDTO_ERROR,
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

    if (length != sizeof(MOCK_PKT)) {
        lgtd_errx(
            1, "got length=%ju (expected %ju)",
            (uintmax_t)length, (uintmax_t)sizeof(MOCK_PKT)
        );
    }

    if (strcmp((const char *)buffer, MOCK_PKT)) {
        lgtd_errx(
            1, "got buffer [%.*s] (expected [%s])",
            (int)length, (const char *)buffer, MOCK_PKT
        );
    }

    if (flags) {
        lgtd_errx(1, "got flags=%#x (expected 0x0)", flags);
    }

    lgtd_tests_check_sockaddr_in(
        dest_addr, dest_len, AF_INET, INADDR_BROADCAST, LGTD_LIFX_PROTOCOL_PORT
    );

    switch (mock_sendto_call_count++) {
    case TEST_SENDTO_OK:
        return length;
    case TEST_SENDTO_PARTIAL:
        return length - 1;
    case TEST_SENDTO_EINTR:
        errno = EINTR;
        return -1;
    case TEST_SENDTO_EINTR_OK:
        return length;
    case TEST_SENDTO_ERROR:
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
    const struct sockaddr *addr = (const struct sockaddr *)&MOCK_ADDR;

    bool ok;

    // ok
    ok = lgtd_lifx_broadcast_send_packet(
        MOCK_PKT, sizeof(MOCK_PKT), addr, sizeof(MOCK_ADDR)
    );
    if (mock_sendto_call_count != 1) {
        lgtd_errx(1, "sendto wasn't called");
    }
    if (!ok) {
        lgtd_errx(1, "broadcast_send_packet returned false (expected true)");
    }

    // partial
    ok = lgtd_lifx_broadcast_send_packet(
        MOCK_PKT, sizeof(MOCK_PKT), addr, sizeof(MOCK_ADDR)
    );
    if (mock_sendto_call_count != 2) {
        lgtd_errx(1, "sendto wasn't called");
    }
    if (ok) {
        lgtd_errx(1, "broadcast_send_packet returned true (expected false)");
    }

    // eintr
    ok = lgtd_lifx_broadcast_send_packet(
        MOCK_PKT, sizeof(MOCK_PKT), addr, sizeof(MOCK_ADDR)
    );
    if (mock_sendto_call_count != 4) {
        lgtd_errx(1, "sendto wasn't called");
    }
    if (!ok) {
        lgtd_errx(1, "broadcast_send_packet returned false (expected true)");
    }

    // error
    ok = lgtd_lifx_broadcast_send_packet(
        MOCK_PKT, sizeof(MOCK_PKT), addr, sizeof(MOCK_ADDR)
    );
    if (mock_sendto_call_count != 5) {
        lgtd_errx(1, "sendto wasn't called");
    }
    if (ok) {
        lgtd_errx(1, "broadcast_send_packet returned true (expected false)");
    }

    return 0;
}
