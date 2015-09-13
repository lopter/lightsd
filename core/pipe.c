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

#include <sys/queue.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <event2/buffer.h>
#include <event2/event.h>

#include "jsmn.h"
#include "jsonrpc.h"
#include "client.h"
#include "pipe.h"
#include "lightsd.h"

struct lgtd_command_pipe_list lgtd_command_pipes =
    SLIST_HEAD_INITIALIZER(&lgtd_command_pipes);

static void
_lgtd_command_pipe_close(struct lgtd_command_pipe *pipe)
{
    assert(pipe);

    event_del(pipe->read_ev);
    if (pipe->fd != -1) {
        close(pipe->fd);
    }
    SLIST_REMOVE(&lgtd_command_pipes, pipe, lgtd_command_pipe, link);
    evbuffer_free(pipe->read_buf);
    event_free(pipe->read_ev);
    free(pipe->client.jsmn_tokens);
    free(pipe);
}

static void
lgtd_command_pipe_close(struct lgtd_command_pipe *pipe)
{
    const char *path = pipe->path;
    _lgtd_command_pipe_close(pipe);
    unlink(path);
    lgtd_info("closed command pipe %s", path);
}

static void lgtd_command_pipe_reset(struct lgtd_command_pipe *);

static void
lgtd_command_pipe_read_callback(evutil_socket_t socket, short events, void *ctx)
{
    assert(ctx);
    assert(socket != -1);

    (void)socket;
    (void)events;

    struct lgtd_command_pipe *pipe = ctx;

    bool drain = false;
    for (int nbytes = evbuffer_read(pipe->read_buf, pipe->fd, -1);
         nbytes;
         nbytes = evbuffer_read(pipe->read_buf, pipe->fd, -1)) {
        if (nbytes == -1) {
            if (errno == EINTR) {
                continue;
            }
            if (errno != EAGAIN) {
                lgtd_warn("read error on command pipe %s", pipe->path);
                break;
            }
            return; // EAGAIN, go back to the event loop
        }

        if (!drain) {
            jsmntok_t *tokens = NULL;
            int ntokens = 0;
            jsmn_parser jsmn_ctx;
        next_request:
            (void)0;
            const char *buf = (char *)evbuffer_pullup(pipe->read_buf, -1);
            ssize_t bufsz = evbuffer_get_length(pipe->read_buf);
        parse_after_realloc:
            jsmn_init(&jsmn_ctx);
            jsmnerr_t rv = jsmn_parse(&jsmn_ctx, buf, bufsz, tokens, ntokens);
            switch (rv) {
            case JSMN_ERROR_NOMEM:
            case JSMN_ERROR_INVAL:
                lgtd_warnx("pipe %s: request too big or invalid: %s", pipe->path, buf);
                // ignore what's left
                drain = true;
                break;
            case JSMN_ERROR_PART:
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch"
            case 0:
#pragma GCC diagnostic pop
                if (bufsz >= LGTD_CLIENT_MAX_REQUEST_BUF_SIZE) {
                    lgtd_warnx("pipe %s: request too big", pipe->path);
                    drain = true;
                }
                break;
            default:
                ntokens = rv;
                if (tokens) {
                    pipe->client.json = buf;
                    lgtd_jsonrpc_dispatch_request(&pipe->client, ntokens);
                    pipe->client.json = NULL;
                    int request_size = tokens[0].end;
                    tokens = NULL;
                    evbuffer_drain(pipe->read_buf, request_size);
                    if (request_size >= bufsz) {
                        break;
                    }
                } else {
                    pipe->client.jsmn_tokens = reallocarray(
                        pipe->client.jsmn_tokens, ntokens, sizeof(*tokens)
                    );
                    tokens = pipe->client.jsmn_tokens;
                    goto parse_after_realloc;
                }
                goto next_request;
            }
        }

        if (drain) {
            ssize_t bufsz = evbuffer_get_length(pipe->read_buf);
            evbuffer_drain(pipe->read_buf, bufsz);
            drain = false;
        }
    }

    lgtd_command_pipe_reset(pipe);
}

static mode_t
lgtd_command_pipe_get_umask(void)
{
    mode_t mask = umask(0);
    umask(mask);
    return mask;
}

static bool
_lgtd_command_pipe_open(const char *path)
{
    assert(path);

    struct lgtd_command_pipe *pipe;
    SLIST_FOREACH(pipe, &lgtd_command_pipes, link) {
        if (!strcmp(pipe->path, path)) {
            return true;
        }
    }

    pipe = calloc(1, sizeof(*pipe));
    if (!pipe) {
        lgtd_warn("can't open command pipe %s", path);
        return false;
    }

    pipe->path = path;
    pipe->fd = -1;

    mode_t mode = S_IWUSR|S_IRUSR|S_IXUSR|S_IRGRP|S_IXGRP|S_IWGRP;
    if (mkfifo(path, mode)) {
        if (errno != EEXIST) {
            goto error;
        }
        mode &= ~lgtd_command_pipe_get_umask();
        if (chmod(path, mode)) {
            goto error;
        }
    }

    pipe->fd = open(path, O_RDONLY|O_NONBLOCK);
    if (pipe->fd == -1) {
        goto error;
    }

    if (evutil_make_socket_nonblocking(pipe->fd) == -1) {
        goto error;
    }

    pipe->read_ev = event_new(
        lgtd_ev_base,
        pipe->fd,
        EV_READ|EV_PERSIST,
        lgtd_command_pipe_read_callback,
        pipe
    );
    if (!pipe->read_ev) {
        goto error;
    }

    pipe->read_buf = evbuffer_new();
    if (!pipe->read_buf) {
        goto error;
    }

    if (event_add(pipe->read_ev, NULL)) {
        goto error;
    }

    SLIST_INSERT_HEAD(&lgtd_command_pipes, pipe, link);

    return true;

error:
    lgtd_warn("can't open command pipe %s", path);
    if (pipe->read_buf) {
        evbuffer_free(pipe->read_buf);
    }
    if (pipe->read_ev) {
        event_free(pipe->read_ev);
    }
    if (pipe->fd != -1) {
        close(pipe->fd);
    }
    free(pipe);
    return false;
}

static void
lgtd_command_pipe_reset(struct lgtd_command_pipe *pipe)
{
    const char *path = pipe->path;
    // we could optimize a bit to avoid re-allocations here:
    _lgtd_command_pipe_close(pipe);
    if (!_lgtd_command_pipe_open(path)) {
        lgtd_warn("can't re-open pipe %s", path);
    }
}

bool
lgtd_command_pipe_open(const char *path)
{
    if (_lgtd_command_pipe_open(path)) {
        lgtd_info("command pipe ready at %s", path);
        return true;
    }
    return false;
}

void
lgtd_command_pipe_close_all(void)
{
    while (!SLIST_EMPTY(&lgtd_command_pipes)) {
        lgtd_command_pipe_close(SLIST_FIRST(&lgtd_command_pipes));
    }
}
