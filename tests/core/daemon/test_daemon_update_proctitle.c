void mock_setproctitle(const char *fmt, ...)
    __attribute__((format(printf, 1, 2)));

#undef LGTD_HAVE_PROCTITLE
#define LGTD_HAVE_PROCTITLE 1
#define setproctitle mock_setproctitle
#include "daemon.c"

#include "mock_gateway.h"
#include "mock_pipe.h"
#include "mock_router.h"
#include "mock_log.h"
#include "mock_timer.h"

#include "tests_utils.h"

const char *expected = "";
int setproctitle_call_count = 0;

void
mock_setproctitle(const char *fmt, ...)
{
    if (strcmp(fmt, "%s")) {
        errx(1, "unexepected format %s (expected %%s)", fmt);
    }

    va_list ap;
    va_start(ap, fmt);
    const char *title = va_arg(ap, const char *);
    va_end(ap);

    if (strcmp(title, expected)) {
        errx(1, "unexpected title: %s (expected %s)", title, expected);
    }

    setproctitle_call_count++;
}

int
main(void)
{
    lgtd_daemon_proctitle_initialized = true;

    expected = "bulbs(found=0, on=0); clients(connected=0)";
    lgtd_daemon_update_proctitle();
    if (setproctitle_call_count != 1) {
        errx(1, "setproctitle should have been called");
    }

    expected = (
        "lifx_gateways(found=1); "
        "bulbs(found=0, on=0); "
        "clients(connected=0)"
    );
    struct lgtd_lifx_gateway *gw_1 = lgtd_tests_insert_mock_gateway(1);
    if (setproctitle_call_count != 2) {
        errx(1, "setproctitle should have been called");
    }

    expected = (
        "lifx_gateways(found=1); "
        "bulbs(found=1, on=0); "
        "clients(connected=0)"
    );
    lgtd_tests_insert_mock_bulb(gw_1, 2);
    expected = (
        "lifx_gateways(found=1); "
        "bulbs(found=2, on=0); "
        "clients(connected=0)"
    );
    lgtd_tests_insert_mock_bulb(gw_1, 3);
    if (setproctitle_call_count != 4) {
        errx(1, "setproctitle should have been called");
    }

    expected = (
        "listening_on([::ffff:127.0.0.1]:1234); "
        "lifx_gateways(found=1); "
        "bulbs(found=2, on=0); "
        "clients(connected=0)"
    );
    lgtd_tests_insert_mock_listener("127.0.0.1", 1234);
    lgtd_daemon_update_proctitle();
    if (setproctitle_call_count != 5) {
        errx(1, "setproctitle should have been called");
    }

    expected = (
        "listening_on([::ffff:127.0.0.1]:1234); "
        "lifx_gateways(found=1); "
        "bulbs(found=2, on=1); "
        "clients(connected=0)"
    );
    LGTD_STATS_ADD_AND_UPDATE_PROCTITLE(bulbs_powered_on, 1);
    if (setproctitle_call_count != 6) {
        errx(1, "setproctitle should have been called");
    }

    expected = (
        "listening_on([::ffff:127.0.0.1]:1234); "
        "lifx_gateways(found=1); "
        "bulbs(found=2, on=1); "
        "clients(connected=1)"
    );
    LGTD_STATS_ADD_AND_UPDATE_PROCTITLE(clients, 1);
    if (setproctitle_call_count != 7) {
        errx(1, "setproctitle should have been called");
    }

    return 0;
}
