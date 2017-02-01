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

from lightsc import requests
from typing import Dict, List

from ... import bulbs, grids
from ...types import Dimensions, Position, TimeMonotonic

from .. import actions

from .base import UIActionEnum, UIComponent, logger


class Button(UIComponent):

    class ActionEnum(UIActionEnum):

        DOWN = 1
        UP = 0

    # make the size configurable too?
    def __init__(
        self,
        name: str,
        offset: Position,
        loop: asyncio.AbstractEventLoop,
        actions: Dict[UIActionEnum, actions.Action],
    ) -> None:
        size = Dimensions(1, 1)
        UIComponent.__init__(self, name, offset, size, loop, actions)
        self._last_level = None  # type: grids.LedLevel

    def draw(self, frame_ts_ms: TimeMonotonic, canvas: grids.LedCanvas) -> bool:
        animate_busy = self.busy and frame_ts_ms % 1000 // 100 % 2
        level = grids.LedLevel.MEDIUM if animate_busy else grids.LedLevel.ON

        if level == self._last_level:
            return False

        self._last_level = level
        for offset in self.size.iter_area():
            canvas.set(offset, level)

        return True

    def handle_input(self, offset: Position, value: grids.KeyState) -> None:
        if value is grids.KeyState.DOWN:
            logger.info("Button {} pressed".format(self.name))
            action = self.actions.get(Button.ActionEnum.DOWN)
        else:
            logger.info("Button {} depressed".format(self.name))
            action = self.actions.get(Button.ActionEnum.UP)

        if action is None:
            return

        try:
            self._action_queue.put_nowait(action)
        except asyncio.QueueFull:
            logger.warning("{!r}: action queue full".format(self))


class PowerButton(Button):

    def __init__(
        self,
        name: str,
        offset: Position,
        loop: asyncio.AbstractEventLoop,
        targets: List[str],
    ) -> None:
        Button.__init__(self, name, offset, loop, actions={
            Button.ActionEnum.UP: actions.Lightsd(
                requests=[requests.PowerToggle], targets=targets
            )
        })
        self.targets = targets

    def draw(self, frame_ts_ms: TimeMonotonic, canvas: grids.LedCanvas) -> bool:
        if self.busy and frame_ts_ms % 1000 // 100 % 2:
            level = grids.LedLevel.MEDIUM
        elif any(bulb.power for bulb in bulbs.iter_targets(self.targets)):
            level = grids.LedLevel.ON
        else:
            level = grids.LedLevel.VERY_LOW_3

        if level == self._last_level:
            return False

        self._last_level = level
        for offset in self.size.iter_area():
            canvas.set(offset, level)

        return True
