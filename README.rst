lightsd, a LIFX broker
======================

lightsd acts a central point of control for your LIFX_ WiFi bulbs. lightsd
should be a small, simple and fast daemon exposing an easy to use protocol
inspired by how musicpd_ works.

Having to run a daemon to control your LIFX bulbs may seem a little bit backward
but has some advantages:

- no discovery delay ever, you get all the bulbs and their state right away;
- lightsd is always in sync with the bulbs and always know their state;
- lightsd act as an abstraction layer and can expose new discovery mechanisms and
  an unified API across different kind of smart bulbs;
- For those of you with a high paranoia factor, lightsd let you place your bulbs
  in a totally separate and closed network.

.. _LIFX: http://lifx.co/
.. _musicpd: http://www.musicpd.org/

Current features
----------------

lightsd discovers your LIFX bulbs, stays in sync with them and support the
following commands through a JSON-RPC_ interface:

- power_off (with auto-retry);
- power_on (with auto-retry);
- power_toggle (power on if off and vice-versa, with auto-retry);
- set_light_from_hsbk;
- set_waveform (change the light according to a function like SAW or SINE);
- get_light_state;
- set_label;
- tag/untag (group/ungroup bulbs together).

The JSON-RPC interface works on top of TCP/IPv4/v6 or over a command pipe (named
pipe, see mkfifo(1)).

lightsd can target single or multiple bulbs at once:

- by device address;
- by device label;
- by tag;
- broadcast;
- composite (list of targets);

lightsd works and is developed against LIFX firmwares 1.1, 1.5, 2.0 and 2.1.

.. _JSON-RPC: http://www.jsonrpc.org/specification

Requirements
------------

lightsd aims to be highly portable on any slightly POSIX system (win32 support
should be quite easy, but isn't really the focus) and on any kind of hardware
including embedded devices. Hence why lightsd is written in C with reasonable
dependencies:

- libevent ≥ 2.0.19 (released May 2012);
- CMake ≥ 2.8.11 (released May 2013): only if you want to build lightsd from its
  sources.

lightsd optionally depends on libbsd ≥ 0.5.0 on platforms missing
``setproctitle`` (pretty much any non-BSD system, including Mac OS X).

lightsd is actively developed and tested from Arch Linux, Debian and Mac OS X;
both for 32/64 bits and little/big endian architectures.

Installation
------------

TBD.

.. _brew: http://brew.sh/

Build instructions
------------------

From a terminal prompt, clone the repository and run those commands:

::

   …/lightsd$ mkdir build && cd build
   …/lightsd/build$ cmake ..
   …/lightsd/build$ make -j5 lightsd

To start lightsd with the jsonrpc interface listening on localhost port 1234 and
a command pipe named lightsd.cmd:

::

   …lightsd/build$ core/lightsd -l ::1:1234 -c lightsd.cmd

This repository contains a small client that you can use to manipulate your
bulbs through lightsd. The client is written in `Python 3`_, Mac OS X and Linux
usually have it installed by default. From a new terminal prompt, cd to the root
of the repository and run it using:

::

   …/lightsd$ examples/lightsc.py

You can exit the lightsd.py using ^D (ctrl + d). Use ^C to stop lightsd.

lightsd can daemonize itself using the ``-d`` option:

::

   …/lightsd/build$ core/lightsd -d -l ::1:1234 -c lightsd.cmd

Check how lightsd is running:

::

   ps aux | grep lightsd

.. _Python 3: https://www.python.org/

Known issues
------------

The White 800 appears to be less reliable than the LIFX Original or Color 650.
The grouping (tagging) code of the LIFX White 800 in particular appears to be
bugged: after a tagging operation the LIFX White 800 keep saying it has no tags.

Power ON/OFF are the only commands with auto-retry, i.e: lightsd will keep
sending the command to the bulb until its state changes. This is not implemented
(yet) for ``set_light_from_hsbk``, ``set_waveform``, ``set_label``, ``tag`` and
``untag``.

In general, crappy WiFi network with latency, jitter or packet loss are gonna be
challenging until lightsd has an auto-retry mechanism, there is also room for
optimizations in how lightsd communicates with the bulbs.

While lightsd appears to be pretty stable, if you want to run lightsd in the
background, I recommend doing so in a process supervisor (e.g: Supervisor_) that
can restart lightsd if it crashes. Otherwise, please feel free to report crashes
to me.

.. _Supervisor: http://www.supervisord.org/

Developpers
-----------

Feel free to reach out via email or irc (kalessin on freenode, insist if I don't
reply). As the project name implies, I'm fairly interested in other smart bulbs.

.. vim: set tw=80 spelllang=en spell:
