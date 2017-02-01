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
import copy
import enum
import logging
import os

from typing import Dict, List

from ... import grids
from ...types import Dimensions, Position, TimeMonotonic

from .. import actions

logger = logging.getLogger("monolight.ui.elements")


class UIComponentInsertionError(Exception):
    pass


UIActionEnum = enum.Enum


class UIComponent:

    ACTION_QUEUE_SIZE = 1

    def __init__(
        self,
        name: str,
        offset: Position,
        size: Dimensions,
        loop: asyncio.AbstractEventLoop,
        actions: Dict[UIActionEnum, actions.Action] = None,
    ) -> None:
        self.name = name
        self.size = size
        self.offset = offset
        self.loop = loop
        self.busy = False
        self.children = set()  # type: Set[UIComponent]
        self.actions = actions if actions is not None else {}
        for action in self.actions.values():
            action.set_source(self)
        self.parent = None  # type: UIComponent

        if loop is not None:
            qsize = self.ACTION_QUEUE_SIZE
            self._action_queue = asyncio.Queue(qsize)  # type: asyncio.Queue
            self._action_runner = loop.create_task(self._process_actions())
            self._action_queue_get = None  # type: asyncio.Future
            self._current_action = None  # type: UIActionEnum

        self._nw_corner = offset - Position(1, 1)
        self._se_corner = Position(
            x=self.offset.x + self.size.width - 1,
            y=self.offset.y + self.size.height - 1,
        )

    def __repr__(self, indent=None):
        if self.name:
            return "<{}(\"{}\", size=({!r}), offset=({!r})>".format(
                self.__class__.__name__, self.name, self.size, self.offset
            )
        return "<{}(size=({!r}), offset=({!r})>".format(
            self.__class__.__name__, self.size, self.offset
        )

    def shutdown(self) -> None:
        for children in self.children:
            children.shutdown()
        self._action_runner.cancel()
        if self._action_queue_get is not None:
            self._action_queue_get.cancel()

    def collides(self, other: "UIComponent") -> bool:
        """Return True if ``self`` and ``other`` overlap in any way.

        .. important::

           ``self`` and ``other`` must be in the same container otherwise
           the result is undefined.
        """

        return all((
            self._nw_corner.x < other._se_corner.x,
            self._se_corner.x > other._nw_corner.x,
            self._nw_corner.y < other._se_corner.y,
            self._se_corner.y > other._nw_corner.y,
        ))

    async def _process_actions(self) -> None:
        current_action = None
        next_action = None
        while True:
            tasks = []
            if next_action is None:
                if current_action is None:
                    next_action = (await self._action_queue.get()).execute()
                    current_action = self.loop.create_task(next_action)
                    next_action = None
                tasks.append(current_action)
                self._action_queue_get = next_action = self.loop.create_task(
                    self._action_queue.get()
                )
            tasks.append(next_action)

            done, pending = await asyncio.wait(
                tasks, return_when=asyncio.FIRST_COMPLETED, loop=self.loop,
            )

            if current_action in done:
                self._action_queue.task_done()
                # always retrieve the result, we might have an error to raise:
                current_action.result()
                current_action = None
            if next_action in done:
                next_action = next_action.result()
                if current_action is None:
                    current_action = self.loop.create_task(
                        next_action.execute()
                    )
                self._action_queue_get = next_action = None

    def draw(self, frame_ts_ms: TimeMonotonic, canvas: grids.LedCanvas) -> bool:
        raise NotImplementedError

    def handle_input(self, offset: Position, value: grids.KeyState) -> None:
        raise NotImplementedError


class _UIPosition(UIComponent):

    def __init__(self, position: Position) -> None:
        UIComponent.__init__(
            self, "_ui_position", position, Dimensions(1, 1), loop=None
        )


class _UIContainer(UIComponent):

    def __init__(
        self,
        name: str,
        offset: Position,
        size: Dimensions,
        loop: asyncio.AbstractEventLoop
    ) -> None:
        UIComponent.__init__(self, name, offset, size, loop)

    def __repr__(self, indent=1) -> str:
        linesep = ",{}{}".format(os.linesep, "  " * indent)
        return (
            "<{}(\"{}\", size=({!r}), offset=({!r}), "
            "components=[{nl}  {indent}{}{nl}{indent}])>".format(
                self.__class__.__name__,
                self.name,
                self.size,
                self.offset,
                linesep.join(
                    component.__repr__(indent + 1)
                    for component in self.children
                ),
                indent="  " * (indent - 1),
                nl=os.linesep,
            )
        )

    def fits(self, other: "UIComponent") -> bool:
        """Return True if ``self`` has enough space to contain ``other``."""

        return (
            other._se_corner.x < self.size.width and
            other._se_corner.y < self.size.height
        )

    def insert(self, new: "UIComponent") -> None:
        if new in self.children:
            raise UIComponentInsertionError(
                "{!r} is already part of {!r}".format(new, self)
            )
        if not self.fits(new):
            raise UIComponentInsertionError(
                "{!r} doesn't fit into {!r}".format(new, self)
            )
        for child in self.children:
            if child.collides(new):
                raise UIComponentInsertionError(
                    "{!r} conflicts with {!r}".format(new, child)
                )

        new.parent = self
        self.children.add(new)

    def submit_input(self, offset: Position, value: grids.KeyState) -> bool:
        if self.collides(_UIPosition(offset)):
            self.handle_input(offset - self.offset, value)
            return True

        return False

    def handle_input(self, offset: Position, value: grids.KeyState) -> None:
        for component in self.children:
            if component.collides(_UIPosition(offset)):
                component.handle_input(offset - component.offset, value)

    def draw(self, frame_ts_ms: TimeMonotonic, canvas: grids.LedCanvas) -> bool:
        dirty = False
        for component in self.children:
            vec = copy.copy(self.offset)
            if not isinstance(component, _UIContainer):
                vec += component.offset
            shifted_canvas = canvas.shift(vec)
            dirty = component.draw(frame_ts_ms, shifted_canvas) or dirty
        return dirty


class UIGroup(_UIContainer):

    def __init__(
        self,
        name: str,
        offset: Position,
        size: Dimensions,
        loop: asyncio.AbstractEventLoop,
        members: List[UIComponent],
    ) -> None:
        UIComponent.__init__(self, name, offset, size, loop)
        for member in members:
            self.insert(member)


class UILayer(_UIContainer):

    def __init__(
        self, name: str, size: Dimensions, loop: asyncio.AbstractEventLoop
    ) -> None:
        _UIContainer.__init__(self, name, Position(0, 0), size, loop)
        self.canvas = grids.LedCanvas(self.size, grids.LedLevel.OFF)

    def render(self, frame_ts_ms: TimeMonotonic) -> bool:
        return self.draw(frame_ts_ms, self.canvas)
