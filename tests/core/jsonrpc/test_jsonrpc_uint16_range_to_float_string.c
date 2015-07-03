#include "jsonrpc.c"

#include "mock_client_buf.h"
#include "test_jsonrpc_utils.h"

int
main(void)
{
    char buf[32];
    int bufsz = sizeof(buf);

    lgtd_jsonrpc_uint16_range_to_float_string(UINT16_MAX, 0, 1, buf, bufsz);
    if (strcmp(buf, "1")) {
        lgtd_errx(
            1, "UINT16_MAX converted to %.*s (expected %s)",
            bufsz, buf, "1"
        );
    }

    lgtd_jsonrpc_uint16_range_to_float_string(UINT16_MAX / 2, 0, 1, buf, bufsz);
    if (strcmp(buf, "0.499992")) {
        lgtd_errx(
            1, "UINT16_MAX / 2 converted to %.*s (expected %s)",
            bufsz, buf, "0.499992"
        );
    }

    lgtd_jsonrpc_uint16_range_to_float_string(0, 0, 1, buf, bufsz);
    if (strcmp(buf, "0")) {
        lgtd_errx(
            1, "UINT16_MAX / 2 converted to %.*s (expected %s)",
            bufsz, buf, "0"
        );
    }

    lgtd_jsonrpc_uint16_range_to_float_string(0xaaaa, 0, 360, buf, bufsz);
    if (strcmp(buf, "240")) {
        lgtd_errx(
            1, "0xaaaa converted to %.*s (expected %s)",
            bufsz, buf, "240"
        );
    }

    bufsz = 2;
    lgtd_jsonrpc_uint16_range_to_float_string(UINT16_MAX / 2, 0, 1, buf, bufsz);
    if (strcmp(buf, "0")) {
        lgtd_errx(
            1,
            "UINT16_MAX / 2 converted to %.*s "
            "(expected %s in case of overflow)",
            bufsz, buf, "0"
        );
    }

    return 0;
}
