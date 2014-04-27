lifxd, a LIFX broker
====================

lifxd acts a central point of control for your LIFX_ WiFi bulbs.

Having to run a daemon to control your LIFX bulbs may seem a little bit
backward but has some advantages:

- no discovery delay ever, you get all the bulbs and their state right away;
- lifxd is always in sync with the bulbs and always know their state;
- lifxd act as an abstraction layer and can expose new discovery mechanism (e.g:
  zeroconf) and totally new APIs;
- For those of you with a high paranoia factor, lifxd let you place your bulbs
  in a totally separate and closed network.

lifxd aims to be modular with a small core process that handles all the
communications with the bulbs and a higher-level plugin API.

.. _LIFX: http://lifx.co/

Features
--------

lifxd doesn't do much yet, it just discovers your bulbs and stay in sync with
them.

Developers
----------

The project is far from being usable right now, but I'll be happy to hear your
feedback and share ideas.

Be aware that some parts of the code aren't really clean yet: I'm more focused
on getting things working and good abstractions.

Requirements
------------

lifxd aims to be highly portable on any POSIX system (win32 support should be
quite easy, but isn't really the goal) and on any kind of hardware including
embedded devices. Hence why lifxd is written in C with reasonable dependencies:

- CMake ≥ 2.8;
- libevent ≥ 2.0.19.

.. vim: set tw=80 spelllang=en spell:
