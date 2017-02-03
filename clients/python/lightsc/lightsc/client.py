# Copyright (c) 2016, Louis Opter <louis@opter.org>
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

import asyncio
import functools
import json
import locale
import logging
import os
import urllib.parse
import uuid

from typing import (
    Any,
    Callable,
    Dict,
    List,
    NamedTuple,
    Sequence,
    Tuple,
)
from typing import Type  # noqa

from . import (
    exceptions,
    requests,
    responses,
    structs,
)

logger = logging.getLogger("lightsc.client")


_JSONRPCMethod = NamedTuple("_JSONRPCMethod", [
    ("name", str),
    ("map_result", Callable[[Any], responses.Response]),
])
_JSONRPC_API = {
    requests.GetLightState: _JSONRPCMethod(
        name="get_light_state",
        map_result=lambda result: responses.LightsState([
            structs.LightBulb(
                b["label"], b["power"], *b["hsbk"], tags=b["tags"]
            ) for b in result
        ])
    ),
    requests.SetLightFromHSBK: _JSONRPCMethod(
        name="set_light_from_hsbk",
        map_result=lambda result: responses.Bool(result)
    ),
    requests.PowerOn: _JSONRPCMethod(
        name="power_on",
        map_result=lambda result: responses.Bool(result)
    ),
    requests.PowerOff: _JSONRPCMethod(
        name="power_off",
        map_result=lambda result: responses.Bool(result)
    ),
    requests.PowerToggle: _JSONRPCMethod(
        name="power_toggle",
        map_result=lambda result: responses.Bool(result)
    ),
    requests.SetWaveform: _JSONRPCMethod(
        name="set_waveform",
        map_result=lambda result: responses.Bool(result)
    ),
}  # type: Dict[Type[requests.RequestClass], _JSONRPCMethod]


class _JSONRPCCall:

    def __init__(
        self, method: str, params: Sequence[Any], timeout: int = None
    ) -> None:
        self.id = str(uuid.uuid4())
        self.method = method
        self.params = params
        self.timeout = timeout
        self.timeout_handle = None  # type: asyncio.Handle
        self.request = {
            "id": self.id,
            "jsonrpc": "2.0",
            "method": method,
            "params": params,
        }
        self.response = asyncio.Future()  # type: asyncio.futures.Future

    @property
    def response_or_exception(self) -> Any:
        ex = self.response.exception()
        return ex if ex is not None else self.response.result()


class AsyncJSONRPCLightsClient:

    READ_SIZE = 8192
    TIMEOUT = 2  # seconds
    ENCODING = "utf-8"

    def __init__(
        self,
        url: str,
        encoding: str = ENCODING,
        timeout: int = TIMEOUT,
        read_size: int = READ_SIZE,
        loop: asyncio.AbstractEventLoop = None
    ) -> None:
        self.url = url
        self.encoding = encoding
        self.timeout = timeout
        self.read_size = read_size
        self._listen_task = None  # type: asyncio.Task
        self._pending_calls = {}  # type: Dict[str, _JSONRPCCall]
        self._reader = None  # type: asyncio.StreamReader
        self._writer = None  # type: asyncio.StreamWriter
        self._loop = loop or asyncio.get_event_loop()

    def _handle_response(
        self, id: str, response: Any, timeout: bool = False
    ) -> None:
        call = self._pending_calls.pop(id)
        if timeout is True:
            call.response.set_exception(exceptions.LightsClientTimeoutError())
            return
        call.timeout_handle.cancel()
        call.response.set_result(response)

    async def _jsonrpc_execute(
        self, pipeline: List[_JSONRPCCall]
    ) -> Dict[str, Any]:
        if not pipeline:
            return {}

        requests = [call.request for call in pipeline]
        for req in requests:
            logger.info("Request {id}: {method}({params})".format(**req))

        payload = json.dumps(requests[0] if len(requests) == 1 else requests)
        self._writer.write(payload.encode(self.encoding, "surrogateescape"))

        await self._writer.drain()

        for call in pipeline:
            call.timeout_handle = self._loop.call_later(
                call.timeout,
                functools.partial(
                    self._handle_response, call.id, response=None, timeout=True
                )
            )
            self._pending_calls[call.id] = call

        futures = [call.response for call in pipeline]
        await asyncio.wait(futures, loop=self._loop)
        return {call.id: call.response_or_exception for call in pipeline}

    async def close(self) -> None:
        if self._listen_task is not None:
            self._listen_task.cancel()
            await asyncio.wait([self._listen_task], loop=self._loop)
            self._listen_task = None

        if self._writer is not None:
            if self._writer.can_write_eof():
                self._writer.write_eof()
            self._writer.close()
        if self._reader is not None:
            self._reader.feed_eof()
            if not self._reader.at_eof():
                await self._reader.read()
        self._reader = self._writer = None

        self._pending_calls = {}

    async def _reconnect(self) -> None:
        await self.close()
        await self.connect()

    async def apply(self, req: requests.Request, timeout: int = TIMEOUT):
        method = _JSONRPC_API[req.__class__]
        call = _JSONRPCCall(method.name, req.params, timeout=timeout)
        result = (await self._jsonrpc_execute([call]))[call.id]
        if isinstance(result, Exception):
            raise result
        return method.map_result(result)

    async def connect(self) -> None:
        parts = urllib.parse.urlparse(self.url)
        if parts.scheme == "unix+jsonrpc":
            path = os.path.join(parts.netloc, parts.path).rstrip(os.path.sep)
            open_connection = functools.partial(
                asyncio.open_unix_connection, path
            )
        elif parts.scheme == "tcp+jsonrpc":
            open_connection = functools.partial(
                asyncio.open_connection, parts.hostname, parts.port
            )
        else:
            raise ValueError("Unsupported url {}".format(self.url))

        try:
            self._reader, self._writer = await asyncio.wait_for(
                open_connection(limit=self.read_size, loop=self._loop),
                self.timeout,
                loop=self._loop,
            )
            self._listen_task = self._loop.create_task(self._listen())
        except Exception:
            logger.error("Couldn't open {}".format(self.url))
            raise

    async def _listen(self) -> None:
        # FIXME:
        #
        # This method is fucked, we need to add a real streaming mode on
        # lightsd's side and then an async version of ijson:

        buf = bytearray()  # those bufs need to be bound to some max size
        sbuf = str()

        while True:
            chunk = await self._reader.read(self.READ_SIZE)
            if not len(chunk):  # EOF, reconnect
                logger.info("EOF, reconnecting...")
                # XXX: deadlock within the close call in _reconnect? (and if
                # that's the case maybe you can use an event or something).
                await self._reconnect()
                return

            buf += chunk
            try:
                sbuf += buf.decode(self.encoding, "strict")  # strict is fucked
            except UnicodeError:
                continue
            buf = bytearray()

            while sbuf:
                # and this is completely fucked:
                try:
                    response = json.loads(sbuf)
                    sbuf = str()
                except Exception:
                    def find_response(delim: str) -> Tuple[Dict[str, Any], str]:
                        offset = sbuf.find(delim)
                        while offset != -1:
                            try:
                                response = json.loads(sbuf[:offset + 1])
                                return response, sbuf[offset + 1:]
                            except Exception:
                                offset = sbuf.find(delim, offset + 2)
                        return None, sbuf

                    for delim in {"}{", "}[", "]{", "]["}:
                        response, sbuf = find_response(delim)
                        if response is not None:
                            break  # yay!
                    else:
                        break  # need more data

                batch = response if isinstance(response, list) else [response]
                for response in batch:
                    id = response["id"]

                    error = response.get("error")
                    if error is not None:
                        code = error.get("code")
                        msg = error.get("message")
                        logger.warning("Error on request {}: {} - {}".format(
                            id, code, msg
                        ))
                        call = self._pending_calls.pop(id)
                        ex = exceptions.LightsClientError(msg)
                        call.response.set_exception(ex)
                        call.timeout_handle.cancel()
                        continue

                    logger.info("Response {}: {}".format(
                        id, response["result"]
                    ))
                    self._handle_response(id, response["result"])

    def batch(self) -> "_AsyncJSONRPCBatch":
        return _AsyncJSONRPCBatch(self)


# LightsClient could eventually point to a different but api-compatible class
# someday:
LightsClient = AsyncJSONRPCLightsClient


class _AsyncJSONRPCBatch:

    def __init__(self, client: AsyncJSONRPCLightsClient) -> None:
        self.responses = None  # type: List[responses.Response]
        self.exceptions = None  # type: List[Exception]
        self._client = client
        self._batch = []  # type: List[Tuple[_JSONRPCMethod, _JSONRPCCall]]

    async def __aenter__(self) -> "_AsyncJSONRPCBatch":
        return self

    async def __aexit__(self, exc_type, exc_val, exc_tb):
        if exc_type is None:
            await self.execute()

    def append(
        self,
        req: requests.Request,
        timeout: int = AsyncJSONRPCLightsClient.TIMEOUT
    ) -> None:
        method = _JSONRPC_API[req.__class__]
        call = _JSONRPCCall(method.name, req.params, timeout=timeout)
        self._batch.append((method, call))

    async def execute(self) -> None:
        resp_by_id = await self._client._jsonrpc_execute([
            call for method, call in self._batch
        ])
        self.responses = []
        self.exceptions = []
        for method, call in self._batch:
            raw_resp = resp_by_id[call.id]
            if isinstance(raw_resp, Exception):
                self.exceptions.append(raw_resp)
            else:
                self.responses.append(method.map_result(raw_resp))


LightsCommandBatch = _AsyncJSONRPCBatch


async def get_lightsd_unix_socket_async(
    loop: asyncio.AbstractEventLoop = None,
) -> str:
    lightsdrundir = None
    try:
        process = await asyncio.create_subprocess_exec(
            "lightsd", "--rundir",
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.DEVNULL,
            loop=loop,
        )
    except FileNotFoundError:  # couldn't find the lightsd bin
        pass
    else:
        stdout, stderr = await process.communicate()
        stdout = stdout.decode(locale.getpreferredencoding()).strip()
        if process.returncode == 0 and stdout:
            lightsdrundir = stdout

    if lightsdrundir is None:
        lightsdrundir = "build"
        logger.warning(
            "Couldn't infer lightsd's runtime directory, is "
            "lightsd installed? Trying {}…".format(lightsdrundir)
        )

    return "unix+jsonrpc://" + os.path.join(lightsdrundir, "socket")


async def create_async_lightsd_connection(
    url: str = None,
    loop: asyncio.AbstractEventLoop = None
) -> AsyncJSONRPCLightsClient:
    if loop is None:
        loop = asyncio.get_event_loop()
    if url is None:
        url = await get_lightsd_unix_socket_async(loop)

    c = AsyncJSONRPCLightsClient(url, loop=loop)
    await c.connect()
    return c


def create_lightsd_connection(url: str = None) -> None:
    raise NotImplementedError("Sorry, no synchronous client available yet")
