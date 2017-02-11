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

import asyncio  # noqa
import lightsc
import logging

from typing import TYPE_CHECKING, List, Type, Union

from .. import bulbs

if TYPE_CHECKING:
    from .elements import UIComponent  # noqa

logger = logging.getLogger("monolight.ui.actions")


class Action:

    def __init__(self) -> None:
        self.loop = None  # type: asyncio.AbstractEventLoop
        self._source = None  # type: UIComponent

    def set_source(self, source: "UIComponent") -> "Action":
        self.loop = source.loop
        self._source = source
        return self

    async def _run(self) -> None:
        # NOTE: Must be re-entrant (which means that all attributes on
        #       self are read-only.
        pass

    async def execute(self) -> None:
        self._source.busy = True
        try:
            await self._run()
        finally:
            self._source.busy = False


class Lightsd(Action):

    # XXX:
    #
    # This isn't correct, as of now RequestType is just a "factory" that
    # optionally takes a targets argument or not:
    RequestType = Type[lightsc.requests.RequestClass]
    RequestTypeList = List[RequestType]

    def __init__(
        self, requests: RequestTypeList = None, targets: List[str] = None
    ) -> None:
        Action.__init__(self)
        self._targets = targets or []
        self._batch = requests or []  # type: Lightsd.RequestTypeList

    def add_target(self, target: str) -> "Lightsd":
        self._targets.append(target)
        return self

    def add_request(self, type: RequestType) -> "Lightsd":
        self._batch.append(type)
        return self

    async def _run(self) -> None:
        requests = []
        async with bulbs.lightsd.batch() as batch:
            for RequestClass in self._batch:
                if self._targets:
                    req = RequestClass(self._targets)
                else:
                    req = RequestClass()
                batch.append(req)
                requests.append(req.__class__.__name__)
        for ex in batch.exceptions:
            logger.warning("Request {} failed on batch-[{}]".format(
                ", ".join(requests)
            ))


class Slide(Action):

    def __init__(self, step: Union[float, int]) -> None:
        Action.__init__(self)
        self.step = step
        self._step = step

    def set_step(self, step: int) -> None:
        self._step = step

    async def _run(self) -> None:
        await self._source.update(self._step)
