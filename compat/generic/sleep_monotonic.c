// Copyright (c) 2017, Louis Opter <louis@opter.org>
//
// This file is part of lighstd.
//
// lighstd is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// lighstd is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with lighstd.  If not, see <http://www.gnu.org/licenses/>.

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "core/time_monotonic.h"
#include "core/lightsd.h"

void
lgtd_sleep_monotonic_msecs(int msecs)
{
    assert(msecs >= 0);

    struct timespec remainder = LGTD_MSECS_TO_TIMESPEC(msecs),
                    request = { 0, 0 },
                    *rmtp = &remainder,
                    *rqtp = &request;
    do {
        LGTD_SWAP(struct timespec *, rqtp, rmtp);
        int err = nanosleep(rqtp, rmtp);
        if (err && errno != EINTR) {
            const char *reason = strerror(errno);
            fprintf(stderr, "lightsd: nanosleep failed: %s.\n", reason);
            abort();
        }
    } while (remainder.tv_sec > 0 && remainder.tv_nsec > 0);
}
