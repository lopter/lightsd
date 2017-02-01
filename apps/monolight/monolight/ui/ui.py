# Copyright (c) 2016, Louis Opter <louis@opter.org>
#
# This file is part of lightsd.
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
import logging
import time

from typing import Tuple  # noqa

from .. import grids

from . import layers

DEFAULT_FRAMERATE = 40

logger = logging.getLogger("monolight.ui")


async def _ui_refresh(loop: asyncio.AbstractEventLoop, framerate: int) -> None:
    while True:
        if not grids.running_event.is_set():
            await grids.running_event.wait()

        if not any(grid.show_ui for grid in grids.running):
            # TODO: handle clean-up when we get ^C while in there:
            await asyncio.wait(
                [grid.wait_ui() for grid in grids.running],
                return_when=asyncio.FIRST_COMPLETED,
                loop=loop
            )

        render_starts_at = time.monotonic()

        for grid in grids.running:
            if not grid.show_ui:
                continue

            layer = grid.foreground_layer
            if layer is None:
                layer = layers.root(loop, grid)
                grid.layers.insert(0, layer)
                logger.info("UI initialized on grid {}: {!r}".format(
                    grid.monome.id, layer
                ))

            if layer.render(frame_ts_ms=int(time.monotonic() * 1000)):
                logger.info("Refreshing UI on grid {}".format(grid.monome.id))
                grid.display(layer.canvas)

        render_latency = time.monotonic() - render_starts_at
        # The plan is to have lightsd push updates and then make
        # something smarter than this:
        await asyncio.sleep(1000 / framerate / 1000 - render_latency, loop=loop)


async def _process_inputs(loop: asyncio.AbstractEventLoop) -> None:
    while True:
        if not grids.running_event.is_set():
            await grids.running_event.wait()

        done, pending = await asyncio.wait(
            [grid.get_input() for grid in grids.running],
            return_when=asyncio.FIRST_COMPLETED,
            loop=loop,
        )  # type: Tuple[Set[asyncio.Future], Set[asyncio.Future]]
        keypresses = []
        for future in done:
            try:
                keypresses.append(future.result())
            except asyncio.CancelledError:
                continue
        for grid, position, value in keypresses:
            logger.info("Keypress {} on grid {} at {}".format(
                value, grid.monome.id, position
            ))
            if grid.foreground_layer is not None:
                grid.foreground_layer.submit_input(position, value)


def start(
    loop: asyncio.AbstractEventLoop, framerate: int = DEFAULT_FRAMERATE
) -> asyncio.Future:
    return asyncio.gather(
        loop.create_task(_ui_refresh(loop, framerate)),
        loop.create_task(_process_inputs(loop)),
        loop=loop,
    )
