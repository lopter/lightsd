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
- tag/untag (group/ungroup bulbs together).

The JSON-RPC interface works on top of TCP/IPv4/v6, Unix sockets (coming up) or
over a command pipe (named pipe, see mkfifo(1)).

lightsd can target single or multiple bulbs at once:

- by device address;
- by device label;
- by tag;
- broadcast;
- composite (list of targets);

lightsd works and is developed against LIFX firmwares 1.1, 1.5 and 2.0.

.. _JSON-RPC: http://www.jsonrpc.org/specification

Developpers
-----------

Feel free to reach out via email or irc (kalessin on freenode). As the project
name implies, I'm fairly interested in other smart bulbs.

Requirements
------------

lightsd aims to be highly portable on any slightly POSIX system (win32 support
should be quite easy, but isn't really the focus) and on any kind of hardware
including embedded devices. Hence why lightsd is written in C with reasonable
dependencies:

- CMake ≥ 2.8;
- libevent ≥ 2.0.19.

lightsd optionally depends on libbsd ≥ 0.5.0 on platforms missing
``setproctitle`` (pretty much any non-BSD system, including Mac OS X).

lightsd is actively developed and tested from Arch Linux, Debian and Mac OS X;
both for 32/64 bits and little/big endian architectures.

Build instructions
------------------

Clone this repository and then:

::

   .../lightsd$ mkdir build && cd build
   .../lightsd/build$ cmake ..
   .../lightsd/build$ make -j5

Finally, to start lightsd with the jsonrpc interface listening on localhost
port 1234 and a command pipe named lightsd.cmd:

::

   .../lightsd/build$ core/lightsd -v info -l ::1:1234 -c lightsd.cmd

lightsd forks in the background by default, display running processes and check
how we are doing:

::

   ps aux | grep lightsd

You can stop lightsd with:

::

   pkill lightsd

Use the ``-f`` option to run lightsd in the foreground.

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

.. vim: set tw=80 spelllang=en spell:
