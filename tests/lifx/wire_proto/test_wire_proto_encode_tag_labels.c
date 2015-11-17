#include <string.h>

#include "wire_proto.c"
#include "mock_daemon.h"
#include "mock_gateway.h"
#include "mock_log.h"

int
main(void)
{
    struct lgtd_lifx_packet_tag_labels pkt = { .tags = 0x2a, .label = "42" };

    lgtd_lifx_wire_encode_tag_labels(&pkt);

    if (pkt.tags != le64toh(0x2a)) {
        errx(
            1, "got tags = %#jx (expected %#jx)",
            (uintmax_t)pkt.tags, (uintmax_t)le64toh(0x2a)
        );
    }
    if (strcmp(pkt.label, "42")) {
        errx(
            1, "got label = %.*s (expected 42)",
            (int)sizeof(pkt.label), pkt.label
        );
    }

    return 0;
}
