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

#ifndef __attribute__
# define __atttribute__(e)
#endif

void lgtd_console_err(int, const char *, va_list)
    __attribute__((noreturn));
void lgtd_console_errx(int, const char *, va_list)
    __attribute__((noreturn));
void lgtd_console_warn(const char *, va_list);
void lgtd_console_warnx(const char *, va_list);
void lgtd_console_info(const char *, va_list);
void lgtd_console_debug(const char *, va_list);
