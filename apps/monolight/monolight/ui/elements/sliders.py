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
import lightsc
import operator
import statistics
import time

from typing import Any, Callable, Iterable, List, NamedTuple, TypeVar

from ... import bulbs, grids
from ...types import Dimensions, Position, TimeMonotonic

from .. import actions

from .base import UIActionEnum, UIComponent, logger


class SliderTraits:
    """Configure the SliderBase class.

    :param gather_fn: a function returning the data observed on the
                      targets associated with this slider.
    :param consolidation_fn: a function returning the current value of
                             this slider using the data returned by gather_fn.
    :param scatter_fn: an async function that can apply the value of the slider
                       to the targets it tracks.
    """

    Controls = NamedTuple(
        "Controls", [("coarse", float), ("fine", float)]
    )

    def __init__(
        self,
        range: range,
        controls: Controls,
        gather_fn: Callable[[List[str]], List[Any]],
        consolidate_fn: Callable[[List[Any]], float],
        scatter_fn: Callable[[List[str], Any, int], None],
    ) -> None:
        self.RANGE = range
        self.controls = controls
        self.gather = gather_fn
        self.consolidate = consolidate_fn
        self.scatter = scatter_fn


class Slider(UIComponent):
    """Base slider implementation.

    :param size: the size of the slider.
    :param offset: position of the slider in within its parent component.
    :param targets: the list of targets this slider is tracking.

    .. note:: Only vertical sliders of width 1 are currently supported.
    """

    class ActionEnum(UIActionEnum):

        COARSE_INC = 0
        FINE_INC = 1
        FINE_DEC = 2
        COARSE_DEC = 3

    def __init__(
        self,
        name: str,
        offset: Position,
        size: Dimensions,
        loop: asyncio.AbstractEventLoop,
        targets: List[str],
        traits: SliderTraits,
    ) -> None:
        controls = traits.controls
        UIComponent.__init__(self, name, offset, size, loop, {
            Slider.ActionEnum.COARSE_INC: actions.Slide(controls.coarse),
            Slider.ActionEnum.FINE_INC: actions.Slide(controls.fine),
            Slider.ActionEnum.FINE_DEC: actions.Slide(-controls.fine),
            Slider.ActionEnum.COARSE_DEC: actions.Slide(-controls.coarse),
        })
        self.value = float(traits.RANGE.start)
        self.targets = targets
        self.traits = traits

        self._action_map = {
            (0, size.height - 1): Slider.ActionEnum.COARSE_DEC,
            (0, size.height - 2): Slider.ActionEnum.FINE_DEC,
            (0, 0): Slider.ActionEnum.COARSE_INC,
            (0, 1): Slider.ActionEnum.FINE_INC,
        }
        self._steps = size.area * (grids.LedLevel.ON + 1)
        self._steps_per_button = self._steps / size.height

    def handle_input(self, offset: Position, value: grids.KeyState) -> None:
        logger.info("Slider {} pressed at {}".format(self.name, offset))
        if value is not grids.KeyState.UP:
            return

        action = self.actions.get(self._action_map.get((offset.x, offset.y)))
        logger.info("Slider {} action = {}".format(
            self.name, self._action_map.get((offset.x, offset.y))
        ))

        if action is None:
            return

        try:
            self._action_queue.put_nowait(action)
        except asyncio.QueueFull:
            logger.warning("Slider {} action queue full".format(self.name))

    def draw(self, frame_ts_ms: TimeMonotonic, canvas: grids.LedCanvas) -> bool:
        new_value = self.traits.consolidate(self.traits.gather(self.targets))
        if new_value is None or new_value == self.value:
            return False

        step = (new_value - self.traits.RANGE.start) / (
            (self.traits.RANGE.stop - self.traits.RANGE.start)
            / self._steps
        )
        logger.info(
            "Slider {} updated from {}/{max} to {}/{max} ({:.4}/{})".format(
                self.name, self.value, new_value, step, self._steps,
                max=self.traits.RANGE.stop,
            )
        )
        self.value = new_value

        on_range = range(
            self.size.height - 1,
            self.size.height - 1 - int(step // self._steps_per_button),
            -1
        )
        y = on_range.start
        for each in on_range:
            canvas.set(Position(0, y), grids.LedLevel.ON)
            y -= 1
        if y >= 0:
            level = grids.LedLevel(int(step % self._steps_per_button))
            canvas.set(Position(0, y), level)
            y -= 1
        for y in range(y, -1, -1):
            canvas.set(Position(0, y), grids.LedLevel.OFF)

        return True

    async def update(self, change: int, transition_ms: int = 600) -> None:
        if change == 0:
            return

        # min/max could eventually be traits.overflow and traits.underflow
        if change > 0:
            new_value = min(self.value + change, float(self.traits.RANGE.stop))
        else:
            new_value = max(self.value + change, float(self.traits.RANGE.start))

        logger.info("Slider {} moving to {}".format(self.name, new_value))

        scatter_starts_at = time.monotonic()
        await self.traits.scatter(self.targets, new_value, transition_ms)
        scatter_exec_time = time.monotonic() - scatter_starts_at

        transition_remaining = transition_ms / 1000 - scatter_exec_time
        if transition_remaining > 0:
            await asyncio.sleep(transition_remaining)

T = TypeVar("T")


def _gather_fn(
    getter: Callable[[lightsc.structs.LightBulb], T], targets: List[str]
) -> Iterable[T]:
    return map(getter, bulbs.iter_targets(targets))

gather_hue = functools.partial(_gather_fn, operator.attrgetter("h"))
gather_saturation = functools.partial(_gather_fn, operator.attrgetter("s"))
gather_brightness = functools.partial(_gather_fn, operator.attrgetter("b"))
gather_temperature = functools.partial(_gather_fn, operator.attrgetter("k"))


def mean_or_none(data: Iterable[T]) -> T:
    try:
        return statistics.mean(data)
    except statistics.StatisticsError:  # no data
        return None


# NOTE:
#
# This will become easier once lightsd supports updating one parameter
# independently from the others:
async def _scatter_fn(
    setter: Callable[
        [lightsc.structs.LightBulb, T, int],
        lightsc.requests.Request
    ],
    targets: List[str],
    value: T,
    transition_ms: int,
) -> None:
    async with bulbs.lightsd.batch() as batch:
        for target in bulbs.iter_targets(targets):
            batch.append(setter(target, value, transition_ms))

scatter_hue = functools.partial(
    _scatter_fn, lambda b, h, t: lightsc.requests.SetLightFromHSBK(
        [b.label], h, b.s, b.b, b.k, transition_ms=t,
    )
)
scatter_saturation = functools.partial(
    _scatter_fn, lambda b, s, t: lightsc.requests.SetLightFromHSBK(
        [b.label], b.h, s, b.b, b.k, transition_ms=t,
    )
)
scatter_brightness = functools.partial(
    _scatter_fn, lambda b, br, t: lightsc.requests.SetLightFromHSBK(
        [b.label], b.h, b.s, br, b.k, transition_ms=t,
    )
)
scatter_temperature = functools.partial(
    _scatter_fn, lambda b, k, t: lightsc.requests.SetLightFromHSBK(
        [b.label], b.h, b.s, b.b, int(k), transition_ms=t,
    )
)

HueSlider = functools.partial(Slider, traits=SliderTraits(
    range=lightsc.constants.HUE_RANGE,
    controls=SliderTraits.Controls(
        coarse=lightsc.constants.HUE_RANGE.stop // 20, fine=1
    ),
    gather_fn=gather_hue,
    consolidate_fn=mean_or_none,
    scatter_fn=scatter_hue,
))
SaturationSlider = functools.partial(Slider, traits=SliderTraits(
    range=lightsc.constants.SATURATION_RANGE,
    controls=SliderTraits.Controls(coarse=0.1, fine=0.01),
    gather_fn=gather_saturation,
    consolidate_fn=mean_or_none,
    scatter_fn=scatter_saturation,
))
BrightnessSlider = functools.partial(Slider, traits=SliderTraits(
    range=lightsc.constants.BRIGHTNESS_RANGE,
    controls=SliderTraits.Controls(coarse=0.1, fine=0.01),
    gather_fn=gather_brightness,
    consolidate_fn=mean_or_none,
    scatter_fn=scatter_brightness,
))
KelvinSlider = functools.partial(Slider, traits=SliderTraits(
    range=lightsc.constants.KELVIN_RANGE,
    controls=SliderTraits.Controls(coarse=500, fine=100),
    gather_fn=gather_temperature,
    consolidate_fn=mean_or_none,
    scatter_fn=scatter_temperature,
))
