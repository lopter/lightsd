#define NDEBUG 1

#include "jsonrpc.c"

#include "mock_client_buf.h"
#include "mock_proto.h"
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

    lgtd_jsonrpc_uint16_range_to_float_string(
        UINT16_MAX / 2 + UINT16_MAX / 3, 0, 1, buf, bufsz
    );
    if (strcmp(buf, "0.833325")) {
        lgtd_errx(
            1,
            "UINT16_MAX / 2 + UINT16_MAX / 3 converted to %.*s (expected %s)",
            bufsz, buf, "0.499992"
        );
    }

    lgtd_jsonrpc_uint16_range_to_float_string(UINT16_MAX / 2, 0, 1, buf, bufsz);
    if (strcmp(buf, "0.499992")) {
        lgtd_errx(
            1, "UINT16_MAX / 2 converted to %.*s (expected %s)",
            bufsz, buf, "0.499992"
        );
    }

    lgtd_jsonrpc_uint16_range_to_float_string(UINT16_MAX / 10, 0, 1, buf, bufsz);
    if (strcmp(buf, "0.099992")) {
        lgtd_errx(
            1, "UINT16_MAX / 10 converted to %.*s (expected %s)",
            bufsz, buf, "0.499992"
        );
    }

    lgtd_jsonrpc_uint16_range_to_float_string(UINT16_MAX / 100, 0, 1, buf, bufsz);
    if (strcmp(buf, "0.009994")) {
        lgtd_errx(
            1, "UINT16_MAX / 100 converted to %.*s (expected %s)",
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

    bufsz = 1;
    lgtd_jsonrpc_uint16_range_to_float_string(UINT16_MAX / 2, 0, 1, buf, bufsz);
    if (buf[0]) {
        lgtd_errx(1, "buffer of one should be '\\0'");
    }

    buf[0] = 'A';
    lgtd_jsonrpc_uint16_range_to_float_string(UINT16_MAX / 2, 0, 1, buf, 0);
    if (buf[0] != 'A') {
        lgtd_errx(1, "buffer of zero shouldn't be written to");
    }

    return 0;
}
