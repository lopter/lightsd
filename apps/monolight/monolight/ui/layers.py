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
import functools
import logging

from lightsc import requests

from .. import grids
from ..types import Dimensions, Position

from . import actions
from .elements import (
    Button,
    PowerButton,
    UILayer,
    groups,
)

logger = logging.getLogger("monolight.ui.layers")


class _ToggleUI(actions.Action):

    def __init__(self, grid: grids.MonomeGrid) -> None:
        actions.Action.__init__(self)
        self._grid = grid

    async def _run(self) -> None:
        self._grid.show_ui = not self._grid.show_ui


def root(loop: asyncio.AbstractEventLoop, grid: grids.MonomeGrid) -> UILayer:
    foreground_layer = UILayer("root", grid.size, loop)

    foreground_layer.insert(Button("toggle ui", Position(15, 7), loop, actions={
        Button.ActionEnum.UP: _ToggleUI(grid),
    }))

    # some shortcuts:
    foreground_layer.insert(Button("off *", Position(0, 7), loop, actions={
        Button.ActionEnum.UP: actions.Lightsd(
            requests=[requests.PowerOff], targets=["*"]
        )
    }))
    foreground_layer.insert(Button("on *", Position(1, 7), loop, actions={
        Button.ActionEnum.UP: actions.Lightsd(
            requests=[requests.PowerOn], targets=["*"]
        )
    }))
    foreground_layer.insert(
        Button("4000k *", Position(2, 7), loop, actions={
            Button.ActionEnum.UP: actions.Lightsd(requests=[
                functools.partial(
                    requests.SetLightFromHSBK,
                    ["*"], 0.0, 0.0, 1.0, 4000, 1000,
                ),
                functools.partial(requests.PowerOn, ["*"]),
            ])
        })
    )
    foreground_layer.insert(Button("blue", Position(3, 7), loop, actions={
        Button.ActionEnum.UP: actions.Lightsd(requests=[
            functools.partial(
                requests.SetLightFromHSBK,
                ["*"], 218, 1.0, 1.0, 3500, 600,
            ),
            functools.partial(requests.PowerOn, ["*"])
        ]),
    }))
    foreground_layer.insert(
        Button("pink breathe *", Position(4, 7), loop, actions={
            Button.ActionEnum.UP: actions.Lightsd(requests=[
                functools.partial(
                    requests.SetWaveform,
                    ["*"], "TRIANGLE", 285, 0.3, 0.8, 3500, 20000, 2, 0.5,
                ),
                functools.partial(requests.PowerOn, ["*"])
            ]),
        })
    )
    foreground_layer.insert(Button("alert *", Position(5, 7), loop, actions={
        Button.ActionEnum.UP: actions.Lightsd(requests=[
            functools.partial(
                requests.SetWaveform,
                ["*"], "SQUARE", 0, 1.0, 1, 3500, 500, 5, 0.5
            ),
            functools.partial(requests.PowerOn, ["*"]),
        ]),
    }))

    # example control blocks:
    BulbControlPad = functools.partial(
        groups.BulbControlPad,
        loop=loop,
        sliders_size=Dimensions(width=1, height=6),
    )
    foreground_layer.insert(BulbControlPad(
        "* control", Position(0, 0), targets=["*"],
    ))

    return foreground_layer
