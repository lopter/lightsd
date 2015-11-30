Changelog
=========

1.1.2 (2015-11-30)
------------------

- Fix LIFX LAN protocol V2 handling (properly set RES_REQUIRED and properly
  listen on each gateway's socket);
- The bulb timeout has been increased from 3 to 20s;
- Improved LIFX traffic logging.

1.1.1 (2015-11-17)
------------------

.. warning::

   This release broke the compatibility with the LIFX LAN protocol "v2", please
   upgrade to 1.1.2.

- Greatly improve responsiveness by setting the LIFX source identifier [#]_.
- Fix parallel builds in the Debian package & fix the homebrew formulae for OS X
  10.11 (El Capitan).

.. [#] http://lan.developer.lifx.com/docs/header-description#frame

1.1.0 (2015-11-07)
------------------

.. note::

   The ``-f`` (``--foreground``) option is being deprecated with this release
   and isn't documented anymore, lightsd starts in the foreground by default and
   this option is not necessary, please stop using it.

New features
~~~~~~~~~~~~

- Add syslog support via the ``--syslog`` and ``--syslog-facility`` options
  (closes :gh:`1`);
- Debian & OpenWRT packaging and installation instructions.

Fixes
~~~~~

- lightsc.sh: support OSes with openssl but without a base64 utility (closes
  :gh:`3`);
- lightsc.py: unix url support fixes and bump the receive buffer size to
  accommodate people with many bulbs;
- Add missing product ids/models.

1.0.1 (2015-09-18)
------------------

- Fix set_waveform on big endian architectures;
- Fix build under Debian oldstable;
- Fix build under OpenBSD [#]_;
- Fix process title even when no bulbs are discovered;
- Add product id for the 230V version of the LIFX White 800.

.. [#] Using GCC 4.2, so you just need to do ``pkg_add cmake libevent`` to
       build a release.

1.0.0 (2015-09-17)
------------------

- First announced release.

.. vim: set tw=80 spelllang=en spell:
