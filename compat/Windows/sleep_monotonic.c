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

// See:
//
// https://msdn.microsoft.com/en-us/library/windows/desktop/ms686298(v=vs.85).aspx
// https://msdn.microsoft.com/en-us/library/dd743626(v=vs.85).aspx

#include <Windows.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "core/time_monotonic.h"
#include "core/lightsd.h"

enum { TIMER_RESOLUTION_MS = 1 };

static UINT
_lgtd_set_timer_resolution(UINT resolution_ms)
{
    TIMECAPS tc;
    if (timeGetDevCaps(&tc, sizeof(tc)) != TIMERR_NOERROR) {
        fprintf(stderr, "lightsd: multimedia timers unavailable.");
        exit(1);
    }

    UINT resolution = LGTD_MAX(tc.wPeriodMin, resolution_ms);
    resolution = LGTD_MIN(resolution, tc.wPeriodMax);
    timeBeginPeriod(resolution);
    return resolution;
}

static void
_lgtd_reset_timer_resolution(UINT resolution_ms)
{
    if (timeEndPeriod(resolution_ms) != TIMERR_NOERROR) {
        fprintf(stderr, "lightsd: can't reset timer resolution.");
    }
}

void
lgtd_sleep_monotonic_msecs(int msecs)
{
    UINT actual = _lgtd_set_timer_resolution(TIMER_RESOLUTION_MS);
    Sleep(msecs);
    _lgtd_reset_timer_resolution(actual);
}
