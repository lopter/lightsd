#pragma once

static char client_write_buf[4096] = { 0 };
static int client_write_buf_idx = 0;

static inline void
reset_client_write_buf(void)
{
    memset(client_write_buf, 0, sizeof(client_write_buf));
    client_write_buf_idx = 0;
}

int
bufferevent_write(struct bufferevent *bev, const void *data, size_t nbytes)
{
    (void)bev;
    int to_write = LGTD_MIN(
        nbytes, sizeof(client_write_buf) - client_write_buf_idx
    );
    memcpy(&client_write_buf[client_write_buf_idx], data, to_write);
    client_write_buf_idx += to_write;
    return 0;
}
