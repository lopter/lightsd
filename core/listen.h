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

struct evconnlistener;

struct lgtd_listen {
    SLIST_ENTRY(lgtd_listen)    link;
    ev_socklen_t                addrlen;
    struct sockaddr             *sockaddr;
    struct evconnlistener       *evlistener;
};
SLIST_HEAD(lgtd_listen_list, lgtd_listen);

extern struct lgtd_listen_list lgtd_listeners;

bool lgtd_listen_open(const char *, const char *);
bool lgtd_listen_unix_open(const char *);
void lgtd_listen_close_all(void);
