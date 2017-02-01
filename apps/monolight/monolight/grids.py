# Copyright (c) 2016, Louis Opter <louis@opter.org>
# # This file is part of lightsd.
#
# lightsd is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# lightsd is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with lightsd.  If not, see <http://www.gnu.org/licenses/>.

import asyncio
import collections
import enum
import functools
import logging
import monome

from typing import TYPE_CHECKING, Any, Iterator, Tuple, NamedTuple, cast
from typing import List, Set  # noqa

from .types import Dimensions, Position
if TYPE_CHECKING:
    from .ui.elements import UILayer  # noqa


logger = logging.getLogger("monolight.grids")

running = set()  # type: Set[MonomeGrid]
running_event = None  # type: asyncio.Event
_not_running_event = None  # type: asyncio.Event


class KeyState(enum.IntEnum):

    DOWN = 1
    UP = 0


KeyPress = NamedTuple("KeyPress", [
    ("grid", "MonomeGrid"),
    ("position", Position),
    ("state", KeyState),
])


class LedLevel(enum.IntEnum):

    OFF = 0
    VERY_LOW_1 = 1
    VERY_LOW_2 = 2
    VERY_LOW_3 = 3
    LOW = LOW_1 = 4
    LOW_2 = 5
    LOW_3 = 6
    LOW_4 = 7
    MEDIUM = MEDIUM_1 = 8
    MEDIUM_2 = 9
    MEDIUM_3 = 10
    MEDIUM_4 = 11
    HIGH = HIGH_1 = 12
    HIGH_2 = 13
    HIGH_3 = 14
    HIGH_4 = ON = 15


class LedCanvas(collections.abc.Iterable):

    def __init__(
        self, size: Dimensions, level: LedLevel = LedLevel.OFF
    ) -> None:
        self.size = size
        self._levels = [level] * size.area

    def _index(self, offset: Position) -> int:
        return self.size.width * offset.y + offset.x

    def set(self, offset: Position, level: LedLevel) -> None:
        self._levels[self._index(offset)] = level

    def shift(self, offset: Position) -> "LedCanvas":
        class _Proxy:
            def __init__(_self, canvas: LedCanvas, shift: Position):
                _self._canvas = canvas
                _self._shift = shift

            def set(_self, offset: Position, level: LedLevel) -> None:
                offset += _self._shift
                return _self._canvas.set(offset, level)

            def shift(_self, offset: Position) -> LedCanvas:
                return cast(LedCanvas, _Proxy(_self, offset))

            def __getattr__(self, name: str) -> Any:
                return self._canvas.__getattribute__(name)
        # I guess some kind of interface would avoid the cast, but whatever:
        return cast(LedCanvas, _Proxy(self, offset))

    def get(self, offset: Position) -> LedLevel:
        return self._levels[self._index(offset)]

    def __iter__(self) -> Iterator[Tuple[int, int, LedLevel]]:
        for off_x in range(self.size.width):
            for off_y in range(self.size.height):
                yield off_x, off_y, self.get(Position(x=off_x, y=off_y))


class AIOSCMonolightApp(monome.Monome):

    def __init__(self, loop: asyncio.AbstractEventLoop) -> None:
        monome.Monome.__init__(self, "/monolight")
        self._grid = None  # type: MonomeGrid
        self.loop = loop

    def ready(self) -> None:
        self._grid = MonomeGrid(self)
        running.add(self._grid)
        logger.info("Grid {} ready".format(self.id))
        if len(running) == 1:
            running_event.set()
            _not_running_event.clear()

    def disconnect(self) -> None:
        if len(running) == 1:
            running_event.clear()
            _not_running_event.set()
        running.remove(self._grid)
        self._grid.shutdown()
        monome.Monome.disconnect(self)
        logger.info("Grid {} disconnected".format(self.id))

    def grid_key(self, x: int, y: int, s: int) -> None:
        if self._grid is not None:
            keypress = KeyPress(self._grid, Position(x, y), KeyState(s))
            self._grid.submit_input(keypress)


class MonomeGrid:

    def __init__(self, monome_app: AIOSCMonolightApp) -> None:
        self.loop = monome_app.loop
        self.size = Dimensions(height=monome_app.height, width=monome_app.width)
        self.layers = []  # type: List[UILayer]
        self._show_ui = asyncio.Event(loop=self.loop)
        self._show_ui.set()
        self._input_queue = asyncio.Queue(loop=self.loop)  # type: asyncio.Queue
        self._queue_get = None  # type: asyncio.Future
        self.monome = monome_app
        self._led_buffer = monome.LedBuffer(
            width=self.size.width, height=self.size.height
        )

    def shutdown(self):
        self._queue_get.cancel()
        self.show_ui = False
        for layer in self.layers:
            layer.shutdown()
        self.monome.led_level_all(LedLevel.OFF.value)

    def submit_input(self, keypress: KeyPress) -> None:
        self._input_queue.put_nowait(keypress)

    async def get_input(self) -> KeyPress:
        self._queue_get = self.loop.create_task(self._input_queue.get())
        keypress = await asyncio.wait_for(
            self._queue_get, timeout=None, loop=self.loop
        )
        self._input_queue.task_done()
        return keypress

    def _hide_ui(self) -> None:
        self._show_ui.clear()
        self.monome.led_level_all(LedLevel.OFF.value)

    def _display_ui(self) -> None:
        self._show_ui.set()
        self._led_buffer.render(self.monome)

    @property
    def show_ui(self) -> bool:
        return self._show_ui.is_set()

    @show_ui.setter
    def show_ui(self, value: bool) -> None:
        self._hide_ui() if value is False else self._display_ui()

    async def wait_ui(self) -> None:
        await self._show_ui.wait()

    @property
    def foreground_layer(self):
        return self.layers[-1] if self.layers else None

    def display(self, canvas: LedCanvas) -> None:
        for off_x, off_y, level in canvas:
            self._led_buffer.led_level_set(off_x, off_y, level.value)
        self._led_buffer.render(self.monome)


_serialosc = None

async def start_serialosc_connection(
    loop: asyncio.AbstractEventLoop, monome_id: str = "*",
) -> None:
    global _serialosc, running_event, _not_running_event

    running_event = asyncio.Event(loop=loop)
    _not_running_event = asyncio.Event(loop=loop)
    _not_running_event.set()
    App = functools.partial(AIOSCMonolightApp, loop)
    _serialosc = await monome.create_serialosc_connection({monome_id: App})


async def stop_all() -> None:
    global running_event, _not_running_event

    if _serialosc is not None:
        _serialosc.transport.close()
    # copy the set since we're gonna modify it as we iter through it:
    for grid in list(running):
        grid.monome.disconnect()
    await _not_running_event.wait()
    running_event = _not_running_event = None
