Changelog
=========

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
