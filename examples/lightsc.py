#!/usr/bin/env python3
# Copyright (c) 2015, Louis Opter <kalessin@kalessin.fr>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice,
#    this list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
#    may be used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.

import argparse
import contextlib
import json
import locale
import os
import socket
import subprocess
import sys
import urllib.parse
import uuid


class LightsClient:

    READ_SIZE = 4096
    TIMEOUT = 2  # seconds
    ENCODING = "utf-8"

    class Error(Exception):
        pass

    class TimeoutError(Error):
        pass

    class JSONError(Error):

        def __init__(self, response):
            self._response = response

        def __str__(self):
            return "received invalid JSON: {}".format(self._response)

    def __init__(self, url, encoding=ENCODING, timeout=TIMEOUT,
                 read_size=READ_SIZE):
        self.url = url
        self.encoding = encoding

        parts = urllib.parse.urlparse(url)

        if parts.scheme == "unix":
            self._socket = socket.socket(socket.AF_UNIX)
            path = os.path.join(parts.netloc, parts.path).rstrip(os.path.sep)
            self._socket.connect(path)
        elif parts.scheme == "tcp":
            self._socket = socket.create_connection((parts.hostname, parts.port))
        else:
            raise ValueError("Unsupported url {}".format(url))
        self._socket.settimeout(timeout)

        self._read_size = read_size
        self._pipeline = []
        self._batch = False

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, exc_tb):
        self.close()

    @classmethod
    def _make_payload(cls, method, params):
        return {
            "method": method,
            "params": params,
            "jsonrpc": "2.0",
            "id": str(uuid.uuid4()),
        }

    def _execute_payload(self, payload):
        response = bytearray()
        try:
            payload = json.dumps(payload)
            payload = payload.encode(self.encoding, "surrogateescape")
            self._socket.sendall(payload)

            while True:
                response += self._socket.recv(self._read_size)
                try:
                    return json.loads(response.decode(
                        self.encoding, "surrogateescape"
                    ))
                except Exception:
                    continue
        except socket.timeout:
            if not response:
                raise self.TimeoutError
            raise self.JSONError(response)

    def _jsonrpc_call(self, method, params):
        payload = self._make_payload(method, params)
        if self._batch:
            self._pipeline.append(payload)
            return
        return self._execute_payload(payload)

    def close(self):
        self._socket.close()

    @contextlib.contextmanager
    def batch(self):
        self._batch = True
        response = []
        yield response
        self._batch = False
        result = self._execute_payload(self._pipeline)
        if isinstance(result, list):
            response.extend(result)
        else:
            response.append(result)
        self._pipeline = []

    def set_light_from_hsbk(self, target, h, s, b, k, t):
        return self._jsonrpc_call("set_light_from_hsbk", [
            target, h, s, b, k, t
        ])

    def set_waveform(self, target, waveform,
                     h, s, b, k,
                     period, cycles, skew_ratio, transient):
        return self._jsonrpc_call("set_waveform", [
            target, waveform, h, s, b, k, period, cycles, skew_ratio, transient
        ])

    def saw(self, target, h, s, b, k, period, cycles, transient=True):
        return self.set_waveform(
            target, "SAW", h, s, b, k,
            cycles=cycles,
            period=period,
            skew_ratio=0.5,
            transient=transient
        )

    def sine(self, target, h, s, b, k,
             period, cycles, peak=0.5, transient=True):
        return self.set_waveform(
            target, "SINE", h, s, b, k,
            cycles=cycles,
            period=period,
            skew_ratio=peak,
            transient=transient
        )

    def half_sine(self, target, h, s, b, k, period, cycles, transient=True):
        return self.set_waveform(
            target, "HALF_SINE", h, s, b, k,
            cycles=cycles,
            period=period,
            skew_ratio=0.5,
            transient=transient
        )

    def triangle(self, target, h, s, b, k,
                 period, cycles, peak=0.5, transient=True):
        return self.set_waveform(
            target, "TRIANGLE", h, s, b, k,
            cycles=cycles,
            period=period,
            skew_ratio=peak,
            transient=transient
        )

    def square(self, target, h, s, b, k, period, cycles,
               duty_cycle=0.5, transient=True):
        return self.set_waveform(
            target, "SQUARE", h, s, b, k,
            cycles=cycles,
            period=period,
            skew_ratio=duty_cycle,
            transient=transient
        )

    def power_on(self, target):
        return self._jsonrpc_call("power_on", {"target": target})

    def power_off(self, target):
        return self._jsonrpc_call("power_off", {"target": target})

    def power_toggle(self, target):
        return self._jsonrpc_call("power_toggle", {"target": target})

    def get_light_state(self, target):
        return self._jsonrpc_call("get_light_state", [target])

    def tag(self, target, tag):
        return self._jsonrpc_call("tag", [target, tag])

    def untag(self, target, tag):
        return self._jsonrpc_call("untag", [target, tag])

    def set_label(self, target, label):
        return self._jsonrpc_call("set_label", [target, label])

    def adjust_brightness(self, target, adjustment):
        bulbs = self.get_light_state(target)["result"]
        for bulb in bulbs:
            h, s, b, k = bulb["hsbk"]
            b = max(min(b + adjustment, 1.0), 0.0)
            self.set_light_from_hsbk(bulb["label"], h, s, b, k, 500)


def _drop_to_shell(lightsc):
    c = lightsc  # noqa
    banner = (
        "Connected to {}, use the variable c to interact with your "
        "bulbs:\n\n>>> r = c.get_light_state(\"*\")".format(c.url)
    )

    try:
        from IPython import embed

        embed(header=banner + "\n>>> r")
        return
    except ImportError:
        pass

    import code

    banner += "\n>>> from pprint import pprint\n>>> pprint(r)\n"
    code.interact(banner=banner, local=locals())

if __name__ == "__main__":
    try:
        lightsdrundir = subprocess.check_output(["lightsd", "--rundir"])
    except Exception as ex:
        print(
            "Couldn't infer lightsd's runtime directory, is lightsd installed? "
            "({})\nTrying build/socket...".format(ex),
            file=sys.stderr
        )
        lightscdir = os.path.realpath(__file__).split(os.path.sep)[:-2]
        lightsdrundir = os.path.join(*[os.path.sep] + lightscdir + ["build"])
    else:
        encoding = locale.getpreferredencoding()
        lightsdrundir = lightsdrundir.decode(encoding).strip()

    parser = argparse.ArgumentParser(
        description="Interactive lightsd Python client"
    )
    parser.add_argument(
        "-u", "--url", type=str,
        help="How to connect to lightsd (e.g: "
             "unix:///run/lightsd/socket or tcp://[::1]:1234)",
        default="unix://" + os.path.join(lightsdrundir, "socket")
    )
    args = parser.parse_args()

    try:
        print("Connecting to lightsd@{}".format(args.url))
        with LightsClient(args.url) as client:
            _drop_to_shell(client)
    except Exception as ex:
        print(
            "Couldn't connect to {}, is the url correct "
            "and is lightsd running? ({})".format(args.url, ex),
            file=sys.stderr
        )
        sys.exit(1)
