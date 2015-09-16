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

The JSON-RPC interface works on top of TCP/IPv4/v6, Unix sockets, or over a
command pipe (named pipe, see `mkfifo(1)`_).

lightsd can target single or multiple bulbs at once:

- by device address;
- by device label;
- by tag;
- broadcast;
- composite (list of targets);

lightsd works and is developed against LIFX firmwares 1.1, 1.5, 2.0 and 2.1.

.. _JSON-RPC: http://www.jsonrpc.org/specification
.. _mkfifo(1): http://www.openbsd.org/cgi-bin/man.cgi?query=mkfifo

Documentation
-------------

Checkout http://lightsd.readthedocs.org/en/latest/ for installation instructions
and a walk-through some examples.

Requirements
------------

lightsd aims to be highly portable on any slightly POSIX system (native Windows
support has been kept in mind should be quite easy, but isn't really the focus)
and on any kind of hardware including embedded devices. Hence why lightsd is
written in C with reasonable dependencies:

- libevent ≥ 2.0.19 (released May 2012);
- CMake ≥ 2.8.11 (released May 2013): only if you want to build lightsd from its
  sources.

lightsd is actively developed and tested from Arch Linux, Debian and Mac OS X;
both for 32/64 bits and little/big endian architectures.

Developers
----------

Feel free to reach out via email or irc (kalessin on freenode, insist if I don't
reply). As the project name implies, I'm fairly interested in other smart bulbs.

.. vim: set tw=80 spelllang=en spell:
