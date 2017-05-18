#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "core/time_monotonic.h"
#include "core/lightsd.h"

int
main(void)
{
    lgtd_time_mono_t start = lgtd_time_monotonic_msecs();
    lgtd_sleep_monotonic_msecs(50);
    int duration = (int)(lgtd_time_monotonic_msecs() - start);
    printf("slept for %dms\r\n", duration);
    return LGTD_ABS(duration - 50) > 10;
}
