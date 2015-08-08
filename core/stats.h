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

#pragma once

struct lgtd_stats {
    int gateways;
    int bulbs;
    int bulbs_powered_on;
    int clients;
};

void lgtd_stats_add(int, int);
int lgtd_stats_get(int);

#define LGTD_STATS_GET(name) lgtd_stats_get(offsetof(struct lgtd_stats, name))

#define LGTD_STATS_ADD_AND_UPDATE_PROCTITLE(name, value) do {           \
    lgtd_stats_add(offsetof(struct lgtd_stats, name), (value));         \
    lgtd_daemon_update_proctitle();                                     \
} while (0)
