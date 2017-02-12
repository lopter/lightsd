Changelog
=========

lightsd uses `semantic versionning <http://semver.org/>`_, here is the summary:

Given a version number MAJOR.MINOR.PATCH:

- MAJOR version gets incremented when you may need to modify your lightsd
  configuration to keep your setup running;
- MINOR version gets incremented when functionality or substantial improvements
  are added in a backwards-compatible manner;
- PATCH version gets incremented for backwards-compatible bug fixes.

1.2.1 (2017-02-12)
------------------

This release was mostly done to iron out issues in the release script. The only
noticeable change is the update to the LIFX product list (closes :gh:`23`).

1.2.0 (2017-02-04)
------------------

This release doesn't have any new user-facing feature for lightsd but packs a
bunch of fixes and improvements, both in the code but also the documentation and
examples.

However, this release sees the start of a Python client library for lightsd
which you'll find in the ``clients`` directory of the lightsd repository. This
library is currently used to build another child project of lightsd: monolight,
an user-interface for the `Monome grid`_. Both project should end up on PyPi_
soon.

Also worth noting, is the full continuous integration pipeline that has been
setup behind the scenes. Most of the work for this release went into it. It will
hopefully make it a lot easier to work on lightsd.

.. _Monome grid: http://www.monome.org/grid/
.. _PyPi: https://pypi.python.org/pypi

Fixes
~~~~~

- Discovery now works properly on computers with multiple network interfaces
  (closes :gh:`2`);
- Correctly support optional arguments in the JSON-RPC API;
- A couple of crash/security fixes, one being in the jsmn_ JSON parser which has
  been upgraded to its latest version;
- FreeBSD build (more BSD fixes to come though, see :gh:`16`);
- ``get_light_state`` now returns a valid target for the bulb's label (the bulb
  id) when it doesn't have a label set.

.. _jsmn: https://github.com/zserge/jsmn

Acknowledgments
~~~~~~~~~~~~~~~

Thanks to:

- `Sylvain Laurent`_ for his original work on fixing discovery;
- `Xavier Deguillard`_ for his contributions; additional automated tests will be
  setup to make crashes and security issues much harder to creep in;
- `Guillaume Gomez`_ & `Michael Zapata`_ for their help with the landing page on
  https://www.lightsd.io/;
- All the people who have been trying the project and reporting issues!

.. _Sylvain Laurent: https://github.com/Magicking/
.. _Xavier Deguillard: https://github.com/Rip-Rip
.. _Guillaume Gomez: https://github.com/GuillaumeGomez
.. _Michael Zapata: https://github.com/michael-zapata

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
