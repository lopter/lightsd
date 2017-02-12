Installation instructions
=========================

Installation (for Mac OS X)
---------------------------

Make sure you have brew installed and updated: http://brew.sh/.

::

   brew install lopter/lightsd/lightsd

Or,

::

   brew tap lopter/lightsd
   brew install lightsd

Please, also install Python 3 and ipython if you want to follow the examples in
the next section:

::

   brew install python3
   pip3 install ipython

Read on :doc:`/first-steps` to see how to use lightsd.

Installation (for Arch Linux)
-----------------------------

.. note::

   A pre-built package may already exist at:
   https://downloads.lightsd.io/archlinux/ for your convenience.

Make sure you have Yaourt installed: https://archlinux.fr/yaourt-en (`wiki
page`_)::

   yaourt -Sya lightsd

Make sure to follow the post-installation instructions: replace ``$USER`` with
the user you usually use.

Please also install ipython if you want to follow the examples in the next
section::

   yaourt -Sya ipython

Read on :doc:`/first-steps` to see how to use lightsd.

.. _wiki page: https://wiki.archlinux.org/index.php/Yaourt

Installation (for Debian/Raspbian)
----------------------------------

Check if an appropriate package exists at: https://downloads.lightsd.io/debian/.
Download it and install it with ``dpkg -i``.

If you were able to install lightsd from a package then read on
:doc:`/first-steps` to see how to use lightsd. Otherwise look for the
Debian/Ubuntu build instructions below.

Installation (for OpenWRT)
--------------------------

Check if a package already exists at: https://downloads.lightsd.io/openwrt/. If
that's the case transfer it to your device and install the package with ``opkg
install``. The lightsd package will depend on ``libevent2-core``.

If you were able to install lightsd from a package then read on
:doc:`/first-steps` to see how to use lightsd. Otherwise look for the OpenWRT
build instructions below.

.. _build_instructions:

Build instructions (for Debian based systems, including Ubuntu/Raspbian)
------------------------------------------------------------------------

.. note:: Those instructions have been tested on Debian Wheezy & Jessie.

Install the following packages:

::

   apt-get install build-essential cmake libevent-dev git ca-certificates ipython3 fakeroot wget devscripts debhelper

Download and extract lightsd:

.. parsed-literal::

   wget -O lightsd\_\ |release|.orig.tar.gz \https://github.com/lopter/lightsd/archive/|release|.tar.gz
   tar -xzf lightsd\_\ |release|.orig.tar.gz
   cd lightsd-|release|
   wget -O - \https://github.com/lopter/lightsd/releases/download/|release|/dpkg-|release|.tar.gz | tar -xzf -

Build the package:

::

   debuild -uc -us

Install the package:

.. note::

   You will need to run this command as root with `sudo(8)`_ or be logged in as
   root already.

.. parsed-literal::

   dpkg -i ../lightsd\_\ |release|-1\_$(dpkg --print-architecture).deb

Still as root, run the command the package asks you to run:

.. note::

   If you are *not using sudo* on your system replace ``$USER`` by your regular
   non-root username:

::

   gpasswd -a $USER lightsd

Log out and back in as ``$USER`` for the change to take effect.

Read on :doc:`/first-steps` to see how to use lightsd.

.. _sudo(8): http://manpages.debian.org/cgi-bin/man.cgi?query=sudo&sektion=8

Build instructions (for OpenWRT)
--------------------------------

Follow the `buildroot instructions`_ then, from your build root, just add
lightsd's feed::

   cat >>feeds.conf$([ -f feeds.conf ] || echo .default) <<EOF
   src-git lightsd https://github.com/lopter/openwrt-lightsd.git
   EOF
   ./scripts/feeds update -a

Install lightsd::

   ./scripts/feeds install lightsd

Run your usual ``make menuconfig``, ``make`` firmware flash flow, lightsd should
be running at startup. If you only wish the build the lightsd package and not
the entire system follow the `single package howto`_.

Read on :doc:`/first-steps` to see how to use lightsd.

.. _buildroot instructions: https://wiki.openwrt.org/doc/howto/buildroot.exigence
.. _single package howto: https://wiki.openwrt.org/doc/howtobuild/single.package

Build instructions (for other systems)
--------------------------------------

lightsd should work on any slightly POSIX system (i.e: not Windows), make sure
you have the following requirements installed:

- libevent ≥ 2.0.19 (released May 2012);
- CMake ≥ 2.8.9 (released August 2012).

lightsd is developed and tested from Arch Linux, Debian, OpenBSD and Mac OS X;
both for 32/64 bits and little/big endian architectures.

Please also install ipython with Python 3 if you want to follow the examples in
the next section.

From a terminal prompt, clone the repository and move to the root of it:

::

   git clone https://github.com/lopter/lightsd.git
   cd lightsd

From the root of the repository:

::

   mkdir build && cd build
   cmake -DCMAKE_BUILD_TYPE=RELEASE ..
   make -j5 lightsd

Read on :doc:`/first-steps` to see how to use lightsd.

.. vim: set tw=80 spelllang=en spell:
