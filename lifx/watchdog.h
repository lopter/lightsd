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

enum lgtd_lifx_watchdog_constants {
    LGTD_LIFX_WATCHDOG_INTERVAL_MSECS = 500,
    LGTD_LIFX_WATCHDOG_ACTIVE_DISCOVERY_INTERVAL_MSECS = 2000,
    LGTD_LIFX_WATCHDOG_PASSIVE_DISCOVERY_INTERVAL_MSECS = 10000,
    LGTD_LIFX_WATCHDOG_DEVICE_TIMEOUT_MSECS = 3000,
    LGTD_LIFX_WATCHDOG_DEVICE_FORCE_REFRESH_MSECS = 2000
};

bool lgtd_lifx_watchdog_setup(void);
void lgtd_lifx_watchdog_start(void);
void lgtd_lifx_watchdog_close(void);
void lgtd_lifx_watchdog_start_discovery(void);
