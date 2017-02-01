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

from typing import List

from ...types import Dimensions, Position

from .base import UIGroup
from .buttons import PowerButton
from .sliders import BrightnessSlider, HueSlider, KelvinSlider, SaturationSlider


class HSBKControlPad(UIGroup):

    def __init__(
        self,
        name: str,
        offset: Position,
        sliders_size: Dimensions,
        loop: asyncio.AbstractEventLoop,
        targets: List[str],
    ) -> None:
        sliders = [
            functools.partial(HueSlider, name="hue"),
            functools.partial(SaturationSlider, name="saturation"),
            functools.partial(BrightnessSlider, name="brightness"),
            functools.partial(KelvinSlider, name="temperature"),
        ]
        sliders = [
            Slider(
                offset=Position(i, 0),
                size=sliders_size,
                loop=loop,
                targets=targets,
            )
            for i, Slider in enumerate(sliders)
        ]
        group_size = Dimensions(width=len(sliders), height=sliders_size.height)
        UIGroup.__init__(self, name, offset, group_size, loop, sliders)


class BulbControlPad(UIGroup):

    def __init__(
        self,
        name: str,
        offset: Position,
        loop: asyncio.AbstractEventLoop,
        targets: List[str],
        sliders_size: Dimensions,
    ) -> None:
        power_btn = PowerButton("toggle power", Position(0, 0), loop, targets)
        hsbk_pad = HSBKControlPad(
            "hsbk pad", Position(0, 1), sliders_size, loop, targets
        )

        group_size = Dimensions(width=0, height=1) + hsbk_pad.size
        UIGroup.__init__(self, name, offset, group_size, loop, [
            power_btn, hsbk_pad,
        ])
