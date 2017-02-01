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

import itertools

from typing import Iterator


class Position:

    def __init__(self, x: int, y: int) -> None:
        self.x = x
        self.y = y

    def __copy__(self) -> "Position":
        return Position(self.x, self.y)

    __deepcopy__ = __copy__

    def __repr__(self) -> str:
        return "{}, {}".format(self.x, self.y)

    def __sub__(self, other: "Position") -> "Position":
        return Position(x=self.x - other.x, y=self.y - other.y)

    def __add__(self, other: "Position") -> "Position":
        return Position(x=self.x + other.x, y=self.y + other.y)


class Dimensions:

    def __init__(self, height: int, width: int) -> None:
        self.height = height
        self.width = width

    def __repr__(self) -> str:
        return "height={}, width={}".format(self.height, self.width)

    def __sub__(self, other: "Dimensions") -> "Dimensions":
        return Dimensions(
            height=self.height - other.height, width=self.width - other.width
        )

    def __add__(self, other: "Dimensions") -> "Dimensions":
        return Dimensions(
            height=self.height + other.height, width=self.width + other.width
        )

    @property
    def area(self) -> int:
        return self.height * self.width

    def iter_area(self) -> Iterator[Position]:
        positions = itertools.product(range(self.width), range(self.height))
        return itertools.starmap(Position, positions)

TimeMonotonic = int
