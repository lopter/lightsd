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

struct lgtd_command_pipe {
    SLIST_ENTRY(lgtd_command_pipe)  link;
    const char                      *path;
    int                             fd;
    struct event                    *read_ev;
    struct evbuffer                 *read_buf;
    struct lgtd_client              client;
};
SLIST_HEAD(lgtd_command_pipe_list, lgtd_command_pipe);

extern struct lgtd_command_pipe_list lgtd_command_pipes;

bool lgtd_command_pipe_open(const char *);
void lgtd_command_pipe_close_all(void);
