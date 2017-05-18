// Copyright (c) 2015, Louis Opter <kalessin@kalessin.fr>
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

// This is pretty much Listing 2 from:
//
// https://developer.apple.com/library/mac/qa/qa1398/_index.html

#include <mach/mach_time.h>
#include <sys/time.h>
#include <assert.h>
#include <stdint.h>

#include "core/time_monotonic.h"

enum { MSECS_IN_NSEC = 1000000 };

lgtd_time_mono_t
lgtd_time_monotonic_msecs(void)
{
    static mach_timebase_info_data_t timebase = { 0, 0 };
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }

    uint64_t time = mach_absolute_time();

    return time * timebase.numer / timebase.denom / MSECS_IN_NSEC;
}
