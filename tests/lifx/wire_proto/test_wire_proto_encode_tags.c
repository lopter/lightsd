#include <string.h>

#include "wire_proto.c"
#include "mock_daemon.h"
#include "mock_gateway.h"
#include "mock_log.h"

int
main(void)
{
    struct lgtd_lifx_packet_tags pkt = { .tags = 0x2a };

    lgtd_lifx_wire_encode_tags(&pkt);

    if (pkt.tags != le64toh(0x2a)) {
        errx(
            1, "got tags = %#jx (expected 0x2a)",
            (uintmax_t)le64toh(pkt.tags)
        );
    }

    return 0;
}
