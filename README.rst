lightsd, a LIFX broker
======================

lightsd acts a central point of control for your LIFX_ WiFi bulbs. lightsd
should be a small, simple and fast daemon exposing an easy to use protocol
inspired by how musicpd_ works.

Having to run a daemon to control your LIFX bulbs may seem a little bit backward
but has some advantages:

- no discovery delay ever, you get all the bulbs and their state right away;
- lightsd is always in sync with the bulbs and always know their state;
- lightsd act as an abstraction layer and can expose new discovery mechanism and
  totally new APIs;
- For those of you with a high paranoia factor, lightsd let you place your bulbs
  in a totally separate and closed network.

.. _LIFX: http://lifx.co/
.. _musicpd: http://www.musicpd.org/

Current features
----------------

lightsd discovers your LIFX bulbs, stays in sync with them and support the
following commands through a JSON-RPC_ interface:

- power_off;
- power_on;
- set_light_from_hsbk;
- set_waveform (change the light according to a function like SAW or SINE);
- get_light_status (coming up).

lightsd can target single or multiple bulbs at once:

- by device address;
- by device label (coming up, labels are already discovered);
- by tag (coming up, tags are already discovered);
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
should be quite easy, but isn't really the goal) and on any kind of hardware
including embedded devices. Hence why lightsd is written in C with reasonable
dependencies:

- CMake ≥ 2.8;
- libevent ≥ 2.0.19.

lightsd is actively developed and tested from Arch Linux and Mac OS X.

.. vim: set tw=80 spelllang=en spell:
