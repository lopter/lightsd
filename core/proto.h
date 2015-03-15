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

bool lgtd_proto_set_light_from_hsbk(const char *, int, int, int, int, int);
bool lgtd_proto_set_waveform(const char *,
                             enum lgtd_lifx_waveform_type,
                             int, int, int, int,
                             int, float, int, bool);
bool lgtd_proto_power_on(const char *);
bool lgtd_proto_power_off(const char *);
