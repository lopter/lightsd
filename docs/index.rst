The lights daemon documentation
===============================

Welcome! This is the documentation for lightsd_: a small program that runs in
the background, discovers smart bulbs [#bulbs]_ on your local network and let
you control them easily.

lightsd exposes a JSON-RPC_ interface over TCP IPv4/IPv6 [#tcp]_ and Unix
sockets. The same interface can be exposed over a `named pipe`_: in that case
responses can't be read back from lightsd but this is useful to control your
lights from very basic shell scripts.

lightsd works out of the box on Mac OS X and Arch Linux but is very easy to
build thanks to its very limited requirements. Check-out the installation
instructions:

.. toctree::
   :maxdepth: 2

   installation
   first-steps
   protocol
   known-issues
   developers
   changelog

.. [#bulbs] Currently only LIFX_ WiFi smart bulbs are supported.
.. [#tcp] And not over HTTP like most JSON-RPC implementations.

.. _lightsd: https://github.com/lopter/lightsd
.. _LIFX: http://www.lifx.co/
.. _JSON-RPC: http://www.jsonrpc.org/specification
.. _named pipe: http://www.openbsd.org/cgi-bin/man.cgi?query=mkfifo

Indices and tables
==================

* :ref:`genindex`
* :ref:`search`

.. vim: set tw=80 spelllang=en spell:
