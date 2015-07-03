#include "proto.c"

#include "mock_client_buf.h"
#include "tests_utils.h"
#include "tests_proto_utils.h"

int
main(void)
{
    struct lgtd_client client;

    lgtd_proto_list_tags(&client);
    if (strcmp(client_write_buf, "[]")) {
        errx(
            1, "expected empty list of tags but got %s instead",
            client_write_buf
        );
    }

    reset_client_write_buf();
    lgtd_tests_insert_mock_tag("test");
    lgtd_proto_list_tags(&client);
    if (strcmp(client_write_buf, "[\"test\"]")) {
        errx(
            1, "expected [\"test\"] but got %s instead",
            client_write_buf
        );
    }

    reset_client_write_buf();
    lgtd_tests_insert_mock_tag("@_@");
    lgtd_proto_list_tags(&client);
    if (strcmp(client_write_buf, "[\"@_@\",\"test\"]")) {
        errx(
            1, "expected [\"@_@\",\"test\"] but got %s instead",
            client_write_buf
        );
    }

    return 0;
}
