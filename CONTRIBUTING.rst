Contributing to lightsd
=======================

*tl;dr: be nice, skim over this and submit your code.*

lightsd is open-source_ software licensed under the GPLv3_ (see `Rationale for
the GPLv3`_). This basically means that you can use lightsd free of charge, as
you see fit, for an unlimited amount of time, no ads, no evaluation period, no
activation key.

Likewise, you can also download and consult the entire source code of lightsd.
Finally, you are welcome to modify lightsd but the GPLv3 forces you to make
those changes public if you decide to share your modified version of lightsd
with other people or companies.

This document describes how to share your modifications directly with lightsd's
developers so that they can be incorporated, distributed and maintained with the
rest of the project.

.. _open-source: http://opensource.org/faq#osd
.. _GPLv3: https://www.gnu.org/licenses/quick-guide-gplv3.html

Project governance
------------------

lightsd started off as a personal project; I hope it doesn't stay personal, but
at the end of the day the whole design and its implementation were done by
mostly one person. Therefore that person will have the last word when it comes
to design decisions and accepting or rejecting new code. This will remain the
case until the project attracts enough knowledgeable developers/maintainers for
decisions to be shared.

In all cases, do not take a "no" personally and hold everyone accountable for
constructive feedback, as explained in the project's code of conduct [#]_.

.. [#] See ``CODE_OF_CONDUCT.md`` at the root of the repository.

Project goals
-------------

The current goal is to be the best and most correct implementation of the `LIFX
LAN protocol`_.

Unlike other projects around LIFX bulbs, lightsd is a background service (daemon
in the Unix terminology). It allows lightsd to act as a proxy for the bulbs and
to report or change the status of the bulbs with low latency. Those two
properties are fundamental to the project:

- being able to work as a proxy makes lightsd easy to extend to other IoT
  protocols. It also allows network isolation;
- being as real time as possible will –I hope– pave the way for new kind of
  usages. For example lightsd is currently is used to pair LIFX bulbs with
  motion sensors, such system requires low latency [#]_. Integrations with
  video games  also come to mind and may require low latency [#]_.

lightsd took the bold choice of being implemented in pure C. C simply is the
most portable language both from a tool-chain perspective but also from a
hardware perspective: anything that can run a very stripped down version of
Linux should be able to run lightsd successfully. Portability is a goal and I
hope that it will unlock new kind of usages & interactions. A corollary to
portability is that lightsd should have the shortest startup and discovery time
possible: lightsd might not always run as a background service.

lightsd must work out of the box and be installable by a high school student who
discovered command line interfaces last week. When documenting something assume
readers kinda know how to copy paste commands in a terminal emulator and install
packages on their system but nothing more. Do not assume they know how to
properly administrate their system, lightsd should be an opportunity to learn
that and inspire the user to learn more. Using lightsd has to be a rewarding
experience.

.. [#] When motion is detected you want to turn the lights immediatly not in 100
       or more milliseconds.
.. [#] This is actually `already happening`_.

.. _already happening: https://community.lifx.com/t/lightsd-a-daemon-with-a-json-rpc-api-to-control-your-bulbs/446/41
.. _LIFX LAN protocol: http://lan.developer.lifx.com/

Non-goals
---------

Adding unpolished features is a non goal, do less features and do them well,
less is better.

HTTP 1.x is a non-goal, it's just not adapted for the RPC (streaming, server to
client pushes and vice-versa) approach lightsd would like to take. Likewise,
technologies such as web-sockets, which could help with that, seem to be quite a
mess in practice. HTTP 2.x however is a different story, and while there is no
plan to support it, I think it's a much more promising technology. gRPC, built
on top of HTTP 2, might be something of interest. I also really like the idea of
having a web-client for lightsd, hopefully browsers will be able to support gRPC
in the future.

Reporting bugs
--------------

I can be joined via email, via IRC in `#lightsd`_ on Freenode, you can also post
in this topic_ and if you feel comfortable writing bug reports you can directly
open an issue on GitHub_.

.. _#lightsd: irc://chat.freenode.net/#lightsd
.. _topic: https://community.lifx.com/t/lightsd-a-daemon-with-a-json-rpc-api-to-control-your-bulbs/446/
.. _Github: https://github.com/lopter/lightsd/issues/new

Submitting contributions
------------------------

Submitting a contribution is pretty much like reporting a bug. At this point,
I'll deal with pretty much any non f'ed-up format. What really matters to me is
the content: make sure your understand how the project is governed and what the
goals are.

It's probably better to start a discussion with me before implementing
something. Feel free to pick an open bug and work on it, if you're not
comfortable in C I'll guide you through the whole thing. I'm ready to work with
motivated beginners, and whatever the outcome is (i.e: your contribution being
merged or not), I'll make sure it's constructive and that you learn something
useful.

I'm trying to make this a cool project to learn programming: in the middle you
have lightsd: a traditional UNIX daemon written in C that doesn't compromise on
modernity (modular approach, evented I/O, unit-tested, fast and lightweight,
highly portable, well integrated in modern init systems). On the left you have
those bulbs that I'm sure could run cool home-brewed firmware and use some
reverse-engineering. And on the right you have this JSON-RPC API that can be
used to implement any kind of cool client.

In other words, by simply using lightsd you are already contributing. For
example a better LIFX firmware would be an awesome contribution. A cool mobile
application would be even better.

Coding style
------------

lightsd is written in C99, C11 doesn't bring much and will make us incompatible
with some platforms (e.g: gcc 4.2 and Microsoft is known to really lag behind on
C standards support).

lightsd must work on 32/64 bits and little/big endian architectures.

lightsd is designed to work on machines without an FPU_, do not use floats
unless there is no other choice.

lightsd is portable and uses CMake_ to introspect the system it's being built
on. Currently supported systems are: Linux, BSD and Darwin (Mac OS X), new
features must work on all three of them.

New code must be unit-tested, CMake is also used as a test runner.

lightsd coding style is:

- overall mostly `K&R`_/1TBS_;
- tabs are 4 spaces;
- C++ style comments;
- 80 columns max (kinda flexible in the headers);
- don't use typedef (rationale: reduces readability, typedefs in C are really
  only useful on integers for things like ``pid_t`` where an int isn't the right
  semantic or for fixed-width integers);
- no includes in the headers (rationale: avoid fucked-up circular dependencies
  scenarios);
- includes order: alphabetically sorted system includes (with ``sys/`` includes
  first however), then libraries includes, then local includes;
- when defining a function the return type and the function name must be on two
  different lines (rationale: make searching a function definition really easy
  with the ``^`` regular expression anchor).

Overall, just be consistent with the existing coding-style, I'll setup `clang
format`_ when I get a chance, it should make the style a non-issue.

.. _FPU: https://en.wikipedia.org/wiki/Floating-point_unit
.. _CMake: https://cmake.org/overview/
.. _K&R: https://en.wikipedia.org/wiki/Indent_style#K.26R_style
.. _1TBS: https://en.wikipedia.org/wiki/Indent_style#Variant:_1TBS_.28OTBS.29
.. _clang format: http://clang.llvm.org/docs/ClangFormat.html

Rationale for the GPLv3
-----------------------

I chose the GPLv3 license for lightsd because it's a personal project and I
don't want lightsd to be used then modified to make a new (or improve an
existing) product behind closed doors. Moreover, I work on lightsd on my free
time and I don't want my time to be used by a company for free.

That said I'd like to make lightsd easy to integrate (i.e: without any
modification) in a closed source context. For example I'm perfectly fine if
lightsd is bundled with a mobile application as long as it's not modified and
that application stays transparent on its use of lightsd and links to lightsd's
homepage.

In the unlikely event that lightsd gains significant adoption I want it to be
the reference (and preferably unique) implementation of its own protocol but
also a reference implementation for the other protocols it implements. I hope
that the GPL will be a good incentive to achieve that goal.

.. vim: set tw=80 spelllang=en spell:
