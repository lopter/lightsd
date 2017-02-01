# Copyright (c) 2016, Louis Opter <louis@opter.org>
#
# This file is part of lighstd.
#
# lighstd is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# lighstd is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with lighstd.  If not, see <http://www.gnu.org/licenses/>.

import setuptools

version = "0.0.1.dev0"

setuptools.setup(
    name="monolight",
    version=version,
    description="A Monome UI to control smart bulbs using lightsd",
    author="Louis Opter",
    author_email="louis@opter.org",
    packages=setuptools.find_packages(exclude=['tests', 'tests.*']),
    include_package_data=True,
    entry_points={
        "console_scripts": [
            "monolight = monolight.monolight:main",
        ],
    },
    install_requires=[
        "click~=6.6",
        "pymonome~=0.8.2",
    ],
    tests_require=[
        "doubles~=1.1.3",
        "freezegun~=0.3.5",
        "pytest~=3.0",
    ],
    extras_require={
        "dev": [
            "flake8",
            "mypy-lang",
            "ipython",
            "pdbpp",
            "pep8",
            "typed-ast",
        ],
    },
)
