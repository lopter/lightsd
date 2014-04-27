// Copyright (c) 2014, Louis Opter <kalessin@kalessin.fr>
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors
//    may be used to endorse or promote products derived from this software
//    without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <sys/queue.h>
#include <sys/tree.h>
#include <arpa/inet.h>
#include <assert.h>
#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/util.h>

#include "wire_proto.h"
#include "discovery.h"
#include "bulb.h"
#include "gateway.h"
#include "lifxd.h"

enum { LIFXD_DISCOVERY_BUF_SIZE = 1024 };

struct lifxd_discovery_rd_ctx {
    enum {
        LIFXD_DISCOVERY_INIT,
        LIFXD_DISCOVERY_HDR_RDY
    }                           state;
    struct event                *ev;
    struct sockaddr_in          peer;
    char                        buf[LIFXD_DISCOVERY_BUF_SIZE];
    // TODO use pkt_offset and read_offset and keep everything in buf
    unsigned                    offset;
    struct lifxd_packet_header  hdr;
    unsigned                    pkt_size;
};

struct lifxd_discovery_wr_ctx {
    struct event                *ev;
    struct lifxd_packet_header  command;
    bool                        encoded;
    unsigned                    to_write;
};

static struct {
    evutil_socket_t                 socket;
    struct lifxd_discovery_rd_ctx   read;
    struct lifxd_discovery_wr_ctx   write;
} lifxd_udp_endpoint = {
    .socket = -1,
    .write = {
        .command = {
            .size = LIFXD_PACKET_HEADER_SIZE,
            .protocol = LIFXD_PROTOCOL_VERSION,
            .packet_type = LIFXD_GET_PAN_GATEWAY
        }
    }
};

void
lifxd_discovery_stop(void)
{
    if (lifxd_udp_endpoint.socket == -1)
        return;

    event_del(lifxd_udp_endpoint.write.ev);
    event_free(lifxd_udp_endpoint.write.ev);
    lifxd_udp_endpoint.write.ev = NULL;
    event_del(lifxd_udp_endpoint.read.ev);
    event_free(lifxd_udp_endpoint.read.ev);
    lifxd_udp_endpoint.read.ev = NULL;

    evutil_closesocket(lifxd_udp_endpoint.socket);
    lifxd_udp_endpoint.socket = -1;

    lifxd_udp_endpoint.read.offset = 0;
    lifxd_udp_endpoint.read.state = LIFXD_DISCOVERY_INIT;
    lifxd_udp_endpoint.write.to_write = 0;
}

static int
lifxd_discovery_recvfrom(struct sockaddr_in *peer, int howmuch)
{
    struct lifxd_discovery_rd_ctx *read = &lifxd_udp_endpoint.read;

    assert(howmuch > 0);
    assert(read->offset + howmuch <= sizeof(read->buf));
    
    while (true) {
        ev_socklen_t socklen = sizeof(*peer);
        int nbytes = recvfrom(
            lifxd_udp_endpoint.socket,
            &read->buf[read->offset],
            howmuch,
            0,
            (struct sockaddr *)peer,
            &socklen
        );
        if (nbytes == -1) {
            int error = EVUTIL_SOCKET_ERROR();
            if (error == EAGAIN) {
                return 0;
            }
            if (error != EINTR) {
                lifxd_err(1, "read error on the udp discovery socket");
            }
        } else {
            return nbytes;
        }
    }
}

static void
lifxd_discovery_handle_read(void)
{
    struct lifxd_discovery_rd_ctx *read = &lifxd_udp_endpoint.read;
    struct sockaddr_in peer;

    while (1) {
        if (read->state == LIFXD_DISCOVERY_INIT) {
            read->offset += lifxd_discovery_recvfrom(
                &peer, LIFXD_PACKET_HEADER_SIZE - read->offset
            );
            if (read->offset == LIFXD_PACKET_HEADER_SIZE) {
                memcpy(&read->peer, &peer, sizeof(peer));
                // TODO: leave the header in the buf and use its size as an offset:
                memcpy(&read->hdr, read->buf, LIFXD_PACKET_HEADER_SIZE);
                lifxd_wire_decode_header(&read->hdr);
                read->pkt_size = read->hdr.size - LIFXD_PACKET_HEADER_SIZE;
                if (read->pkt_size > LIFXD_DISCOVERY_BUF_SIZE) {
                    lifxd_warnx(
                        "received bugged header from [%s]:%hu "
                        "with packet size = %d, type = %#x",
                        inet_ntoa(peer.sin_addr),
                        ntohs(peer.sin_port),
                        read->pkt_size,
                        read->hdr.packet_type
                    );
                    read->offset = 0;
                    continue;
                }
                read->state = LIFXD_DISCOVERY_HDR_RDY;
                int extra_bytes = read->offset - LIFXD_PACKET_HEADER_SIZE;
                if (extra_bytes) {
                    memmove(read->buf, &read->buf[read->offset], extra_bytes);
                    read->offset = 0;
                }
            } else {
                return;
            }
        }
        if (read->state == LIFXD_DISCOVERY_HDR_RDY) {
            while (read->offset < read->pkt_size) {
                int nbytes = lifxd_discovery_recvfrom(
                    &peer, read->pkt_size - read->offset
                );
                if (!nbytes) {
                    return;
                }
                if (peer.sin_addr.s_addr != read->peer.sin_addr.s_addr) {
                    lifxd_warnx("different bulbs are advertising on the network");
                } else {
                    read->offset += nbytes;
                }
            }
            const struct lifxd_packet_infos *pkt_infos =
                lifxd_gateway_get_packet_infos(read->hdr.packet_type);
            // We can run into 4 scenarios at this point:
            // 1. We get a packet we don't know how to handle: discard and restart
            //    the discovery;
            // 2. We get a packet we know how to handle but which is not part of
            //    the discovery process (i.e: not GET_PAN_GW nor PAN_GW): try to
            //    handle it if we already know the gateway where it is from and
            //    restart the discovery processs;
            // 3. We get a GET_PAN_GW packet, probably, our own packet: discard it;
            // 4. We get a PAN_GW packet: handle it and stop the discovery process.
            if (pkt_infos) {
                if (pkt_infos->type != LIFXD_GET_PAN_GATEWAY) {
                    struct lifxd_gateway *gw = lifxd_gateway_get(read->hdr.gw_addr);
                    if (!gw && pkt_infos->type == LIFXD_PAN_GATEWAY) {
                        gw = lifxd_gateway_open(
                            inet_ntoa(peer.sin_addr),
                            ntohs(peer.sin_port),
                            read->hdr.gw_addr
                        );
                    }
                    if (gw) {
                        pkt_infos->decode(read->buf);
                        pkt_infos->handle(gw, &read->hdr, read->buf);
                        if (pkt_infos->type == LIFXD_PAN_GATEWAY) {
                            lifxd_discovery_stop();
                            return;
                        } else if (!lifxd_discovery_start()) {
                            lifxd_warn("can't start auto discovery");
                        }
                    } else {
                        lifxd_err(1, "can't allocate gateway");
                    }
                } else {
                    lifxd_debug(
                        "discarding GET_PAN_GATEWAY packet from [%s]:%hu",
                        inet_ntoa(peer.sin_addr),
                        ntohs(peer.sin_port)
                    );
                }
            } else {
                lifxd_warnx(
                    "received unknown packet %#x from [%s]:%hu",
                    read->hdr.packet_type,
                    inet_ntoa(peer.sin_addr),
                    ntohs(peer.sin_port)
                );
                if (!lifxd_discovery_start()) {
                    lifxd_warn("can't start auto discovery");
                }
            }
            read->offset = 0;
            read->state = LIFXD_DISCOVERY_INIT;
        }
    }
}

static void
lifxd_discovery_handle_write(void)
{
    struct lifxd_discovery_wr_ctx *write = &lifxd_udp_endpoint.write;

    if (write->to_write) {
        struct sockaddr_in addr = {
            .sin_family = AF_INET,
            .sin_port = htons(lifxd_opts.master_port)
        };
        int nbytes;
        inet_pton(AF_INET, "255.255.255.255", &addr.sin_addr);
    retry:
        nbytes = sendto(
            lifxd_udp_endpoint.socket,
            (void *)&write->command,
            write->to_write,
            0,
            (struct sockaddr *)&addr,
            sizeof(addr)
        );
        if (nbytes == -1) {
            int error = EVUTIL_SOCKET_ERROR();
            if (error == EINTR) {
                goto retry;
            } else if (error != EAGAIN) {
                lifxd_warn("can't broadcast discovery packet");
            }
        } else {
            write->to_write -= nbytes;
        }
    }

    if (!write->to_write) {
        lifxd_info("discovery packet has been broadcasted");
        if (event_del(write->ev) == -1) {
            lifxd_warn("can't stop discovery");
        }
    }
}

static void
lifxd_discovery_event_callback(evutil_socket_t socket, short events, void *ctx)
{
    (void)socket;
    (void)ctx;

    if (events & EV_TIMEOUT) {
        lifxd_errx(1, "timeout on the udp discovery socket");
        // TODO: reset state and restart discovery.
    }
    if (events & EV_READ) {
        lifxd_discovery_handle_read();
    }
    if (events & EV_WRITE) {
        lifxd_discovery_handle_write();
    }
}

static bool
lifxd_discovery_setup(void)
{
    assert(lifxd_udp_endpoint.socket == -1);

    if (!lifxd_udp_endpoint.write.encoded) {
        lifxd_wire_encode_header(&lifxd_udp_endpoint.write.command);
        lifxd_udp_endpoint.write.encoded = true;
    }

    lifxd_udp_endpoint.socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (lifxd_udp_endpoint.socket == -1) {
        return false;
    }

    int val = 1;
    int err = setsockopt(
        lifxd_udp_endpoint.socket, SOL_SOCKET, SO_BROADCAST, &val, sizeof(val)
    );
    if (err == -1) {
        goto err_setsockopt;
    }

    if (evutil_make_socket_nonblocking(lifxd_udp_endpoint.socket) == -1) {
        goto err_ononblock;
    }

    struct evutil_addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof(hints));
    char port_str[6] = { 0 };
    evutil_snprintf(
        port_str, sizeof(port_str), "%hu", lifxd_opts.master_port
    );
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    hints.ai_flags = EVUTIL_AI_NUMERICHOST;

    err = evutil_getaddrinfo("0.0.0.0", port_str, &hints, &res);
    if (err || !res) {
        goto err_getaddrinfo;
    }
    if (bind(lifxd_udp_endpoint.socket, res->ai_addr, res->ai_addrlen) == -1) {
        evutil_freeaddrinfo(res);
        goto err_bind;
    }
    freeaddrinfo(res);

    lifxd_udp_endpoint.read.ev = event_new(
        lifxd_ev_base,
        lifxd_udp_endpoint.socket,
        EV_READ|EV_PERSIST,
        lifxd_discovery_event_callback,
        NULL
    );
    lifxd_udp_endpoint.write.ev = event_new(
        lifxd_ev_base,
        lifxd_udp_endpoint.socket,
        EV_WRITE|EV_PERSIST,
        lifxd_discovery_event_callback,
        NULL
    );
    if (!lifxd_udp_endpoint.read.ev || !lifxd_udp_endpoint.write.ev) {
        goto err_event_new;
    }
    err = event_add(lifxd_udp_endpoint.read.ev, NULL);
    if (err) {
        goto err_event_add;
    }

    return true;

err_event_add:
err_event_new:
    if (lifxd_udp_endpoint.read.ev) {
        event_free(lifxd_udp_endpoint.read.ev);
        lifxd_udp_endpoint.read.ev = NULL;
    }
    if (lifxd_udp_endpoint.write.ev) {
        event_free(lifxd_udp_endpoint.write.ev);
        lifxd_udp_endpoint.write.ev = NULL;
    }
err_bind:
err_getaddrinfo:
err_setsockopt:
err_ononblock:
    evutil_closesocket(lifxd_udp_endpoint.socket);
    lifxd_udp_endpoint.socket = -1;
    return false;
}

bool
lifxd_discovery_start(void)
{
    assert(lifxd_udp_endpoint.write.to_write == 0);

    if (lifxd_udp_endpoint.socket == -1 && !lifxd_discovery_setup()) {
        return false;
    }

    lifxd_udp_endpoint.write.to_write = LIFXD_PACKET_HEADER_SIZE;
    if (event_add(lifxd_udp_endpoint.write.ev, NULL) == -1) {
        goto err_enable_writes;
    }
    if (event_add(lifxd_udp_endpoint.read.ev, NULL) == -1) {
        goto err_enable_reads;
    }

    lifxd_info("starting auto-discovery");
    return true;

err_enable_reads:
    event_del(lifxd_udp_endpoint.write.ev);
err_enable_writes:
    lifxd_udp_endpoint.write.to_write = 0;
    return false;
}
