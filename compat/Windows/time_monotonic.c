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

// This is pretty much "Using QPC in native code" from:
//
// https://msdn.microsoft.com/en-us/library/windows/desktop/dn553408(v=vs.85).aspx

#include <Windows.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "core/time_monotonic.h"

lgtd_time_mono_t
lgtd_time_monotonic_msecs(void)
{
    static LARGE_INTEGER frequency = { .QuadPart = 0 }; // ticks per second
    if (frequency.QuadPart == 0
        && QueryPerformanceFrequency(&frequency) == false) {
        LPSTR msg = NULL;
        DWORD msg_len = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER
            | FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            // By default the message is stored in the buffer pointed by
            // lpMsgBuf, but we use FORMAT_MESSAGE_ALLOCATE_BUFFER to have
            // the buffer allocated for us.
            // So instead you need to give the function a pointer on your
            // pointer instead of your buffer and do an ugly cast:
            (LPSTR)&msg,
            0,
            NULL
        );
        if (!msg_len) {
            msg = "unknown error";
        }
        fprintf(stderr, "lightsd: QPC timer unavailable: %s.\r\n", msg);
        exit(1);
    }

    LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
    // Multiply the number of ticks by 1000 first to get a final value in ms:
    time.QuadPart *= 1000;
    // Then (the order is important to avoid loosing precision) divide by the
    // number of ticks per second:
    time.QuadPart /= frequency.QuadPart;
    return time.QuadPart;
}
