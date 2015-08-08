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

#include <assert.h>
#include <stdint.h>

#include "stats.h"

struct lgtd_stats lgtd_counters = { .gateways = 0 };

void
lgtd_stats_add(int offset, int value)
{
    assert(offset >= 0);
    assert(offset < (int)sizeof(lgtd_counters));
    assert(offset % sizeof(int) == 0);

    int *counter = (int *)((uint8_t *)&lgtd_counters + offset);

    assert(*counter + value >= 0);

    *counter += value;
}

int
lgtd_stats_get(int offset)
{
    assert(offset >= 0);
    assert(offset < (int)sizeof(lgtd_counters));
    assert(offset % sizeof(int) == 0);

    return *(int *)((uint8_t *)&lgtd_counters + offset);
}
