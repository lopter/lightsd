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

// /24
#define TEST_IPV4_NETMASK_ADDR_24 LGTD_STATIC_HTONL(0xffffff00)
static struct sockaddr_in TEST_IPV4_NETMASK_SOCKADDR_24 = {
    .sin_family = AF_INET, .sin_addr = { TEST_IPV4_NETMASK_ADDR_24 }
};

// /16
#define TEST_IPV4_NETMASK_ADDR_16 LGTD_STATIC_HTONL(0xffff0000)
static struct sockaddr_in TEST_IPV4_NETMASK_SOCKADDR_16 = {
    .sin_family = AF_INET, .sin_addr = { TEST_IPV4_NETMASK_ADDR_16 }
};

// 192.168.42.255
#define TEST_IPV4_BROADCAST_ADDR_CLASS_C LGTD_STATIC_HTONL(0xc0a82aff)
static struct sockaddr_in TEST_IPV4_BROADCAST_SOCKADDR_CLASS_C = {
    .sin_family = AF_INET, .sin_addr = { TEST_IPV4_BROADCAST_ADDR_CLASS_C }
};

// 10.10.255.255
#define TEST_IPV4_BROADCAST_ADDR_CLASS_A LGTD_STATIC_HTONL(0x0a0affff)
static struct sockaddr_in TEST_IPV4_BROADCAST_SOCKADDR_CLASS_A = {
    .sin_family = AF_INET, .sin_addr = { TEST_IPV4_BROADCAST_ADDR_CLASS_A }
};

// 82.66.148.158
#define TEST_IPV4_UNICAST_ADDR_ROUTABLE LGTD_STATIC_HTONL(0x5242949e)
static struct sockaddr_in TEST_IPV4_UNICAST_SOCKADDR_ROUTABLE = {
    .sin_family = AF_INET, .sin_addr = { TEST_IPV4_UNICAST_ADDR_ROUTABLE }
};

static struct sockaddr_in6 TEST_IPV6_NETMASK_SOCKADDR_64 = {
    .sin6_family = AF_INET6
};
static struct sockaddr_in6 TEST_IPV6_UNICAST_SOCKADDR = {
    .sin6_family = AF_INET6
};

static struct ifaddrs MOCK_IFSOCKADDR_IPV4_BROADCAST_SOCKADDR_CLASS_C = {
    .ifa_flags = IFF_BROADCAST,
    .ifa_broadaddr = (struct sockaddr *)&TEST_IPV4_BROADCAST_SOCKADDR_CLASS_C,
    .ifa_netmask = (struct sockaddr *)&TEST_IPV4_NETMASK_SOCKADDR_24,
    .ifa_next = NULL
};
static struct ifaddrs MOCK_IFSOCKADDR_IPV4_BROADCAST_SOCKADDR_CLASS_A = {
    .ifa_flags = IFF_BROADCAST,
    .ifa_broadaddr = (struct sockaddr *)&TEST_IPV4_BROADCAST_SOCKADDR_CLASS_A,
    .ifa_netmask = (struct sockaddr *)&TEST_IPV4_NETMASK_SOCKADDR_16,
    .ifa_next = &MOCK_IFSOCKADDR_IPV4_BROADCAST_SOCKADDR_CLASS_C
};
static struct ifaddrs MOCK_IFSOCKADDR_IPV4_UNICAST_SOCKADDR_ROUTABLE = {
    .ifa_dstaddr = (struct sockaddr *)&TEST_IPV4_UNICAST_SOCKADDR_ROUTABLE,
    .ifa_netmask = (struct sockaddr *)&TEST_IPV4_NETMASK_SOCKADDR_24,
    .ifa_next = &MOCK_IFSOCKADDR_IPV4_BROADCAST_SOCKADDR_CLASS_A
};
static struct ifaddrs MOCK_IFSOCKADDR_IPV6_UNICAST_SOCKADDR = {
    .ifa_dstaddr = (struct sockaddr *)&TEST_IPV6_UNICAST_SOCKADDR,
    .ifa_netmask = (struct sockaddr *)&TEST_IPV6_NETMASK_SOCKADDR_64,
    .ifa_next = &MOCK_IFSOCKADDR_IPV4_UNICAST_SOCKADDR_ROUTABLE
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

static int mock_getifaddrs_call_count = 0;

int
mock_getifaddrs(struct ifaddrs **ifap)
{
    if (!ifap) {
        lgtd_errx(1, "ifap souldn't be NULL");
    }

    *ifap = &MOCK_IFSOCKADDR_IPV6_UNICAST_SOCKADDR;

    mock_getifaddrs_call_count++;

    return 0;
}

static int mock_freeifaddrs_call_count = 0;

void
mock_freeifaddrs(struct ifaddrs *ifp)
{
    if (ifp != &MOCK_IFSOCKADDR_IPV6_UNICAST_SOCKADDR) {
        lgtd_errx(
            1, "got ifp = %p (expected %p)",
            ifp, &MOCK_IFSOCKADDR_IPV6_UNICAST_SOCKADDR
        );
    }

    mock_freeifaddrs_call_count++;
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

static int mock_sendto_call_count = 0;

enum mock_sendto_calls {
    // 1st test case everything ok
    TEST_OK_SENDTO_OK_ADDR_CLASS_A = 0,
    TEST_OK_SENDTO_OK_ADDR_CLASS_C,
    // 2nd test case one failure
    TEST_PARTIAL_SENDTO_OK_ADDR_CLASS_A,
    TEST_PARTIAL_SENDTO_ERROR_ADDR_CLASS_C,
    // 3rd test case all failures
    TEST_FAIL_SENDTO_ERROR_ADDR_CLASS_A,
    TEST_FAIL_SENDTO_ERROR_ADDR_CLASS_C
};

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

    switch (mock_sendto_call_count++) {
    case TEST_PARTIAL_SENDTO_OK_ADDR_CLASS_A:
    case TEST_OK_SENDTO_OK_ADDR_CLASS_A:
        lgtd_tests_check_sockaddr_in(
            dest_addr,
            dest_len,
            AF_INET,
            TEST_IPV4_BROADCAST_ADDR_CLASS_A,
            LGTD_LIFX_PROTOCOL_PORT
        );
        return length;
    case TEST_OK_SENDTO_OK_ADDR_CLASS_C:
        lgtd_tests_check_sockaddr_in(
            dest_addr,
            dest_len,
            AF_INET,
            TEST_IPV4_BROADCAST_ADDR_CLASS_C,
            LGTD_LIFX_PROTOCOL_PORT
        );
        return length;
    case TEST_PARTIAL_SENDTO_ERROR_ADDR_CLASS_C:
    case TEST_FAIL_SENDTO_ERROR_ADDR_CLASS_C:
        lgtd_tests_check_sockaddr_in(
            dest_addr,
            dest_len,
            AF_INET,
            TEST_IPV4_BROADCAST_ADDR_CLASS_C,
            LGTD_LIFX_PROTOCOL_PORT
        );
        return length - 1;
    case TEST_FAIL_SENDTO_ERROR_ADDR_CLASS_A:
        lgtd_tests_check_sockaddr_in(
            dest_addr,
            dest_len,
            AF_INET,
            TEST_IPV4_BROADCAST_ADDR_CLASS_A,
            LGTD_LIFX_PROTOCOL_PORT
        );
        return length - 1;
    default:
        lgtd_errx(1, "mock_sendto called too many times");
    }
}

int
main(void)
{
    lgtd_lifx_broadcast_endpoint.socket = MOCK_SOCKET_FD;
    lgtd_lifx_broadcast_endpoint.write_ev = MOCK_WRITE_EV;

    bool ok;

    // getifaddrs ok
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
    if (mock_freeifaddrs_call_count != 1) {
        lgtd_errx(
            1, "freeifaddrs_call_count=%d (expected 1)",
            mock_freeifaddrs_call_count
        );
    }

    lgtd_tests_hr();

    // getifaddrs ok, one send fails
    ok = lgtd_lifx_broadcast_handle_write();
    if (!ok) {
        lgtd_errx(1, "write callback returned false (expected true)");
    }
    if (mock_lifx_wire_setup_header_call_count != 2) {
        lgtd_errx(
            1, "mock_lifx_wire_setup_header_call_count=%d (expected 2)",
            mock_lifx_wire_setup_header_call_count
        );
    }
    if (mock_sendto_call_count != 4) {
        lgtd_errx(
            1, "mock_sendto_call_count=%d (expected 4)",
            mock_sendto_call_count
        );
    }
    if (event_del_call_count != 2) {
        lgtd_errx(
            1, "event_del_call_count=%d (expected 2)", event_del_call_count
        );
    }
    if (mock_freeifaddrs_call_count != 2) {
        lgtd_errx(
            1, "freeifaddrs_call_count=%d (expected 2)",
            mock_freeifaddrs_call_count
        );
    }

    lgtd_tests_hr();

    // getifaddrs ok, all sends fail
    ok = lgtd_lifx_broadcast_handle_write();
    if (ok) {
        lgtd_errx(1, "write callback returned true (expected false)");
    }
    if (mock_lifx_wire_setup_header_call_count != 3) {
        lgtd_errx(
            1, "mock_lifx_wire_setup_header_call_count=%d (expected 2)",
            mock_lifx_wire_setup_header_call_count
        );
    }
    if (mock_sendto_call_count != 6) {
        lgtd_errx(
            1, "mock_sendto_call_count=%d (expected 6)",
            mock_sendto_call_count
        );
    }
    if (event_del_call_count != 2) {
        lgtd_errx(
            1, "event_del_call_count=%d (expected 2)", event_del_call_count
        );
    }
    if (mock_freeifaddrs_call_count != 3) {
        lgtd_errx(
            1, "freeifaddrs_call_count=%d (expected 3)",
            mock_freeifaddrs_call_count
        );
    }

    return 0;
}
