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


Make sure you execute the ``ln -sfv`` command displayed at the end of the
installation:

::

   ln -sfv /usr/local/opt/lightsd/*.plist ~/Library/LaunchAgents

Please, also install Python 3 and ipython if you want to follow the examples in
the next section:

::

   brew install python3
   pip3 install ipython

Read on :doc:`/first-steps` to see how to use lightsd.

Installation (for Arch Linux)
-----------------------------

Make sure you have yaourt installed: https://archlinux.fr/yaourt-en (`wiki
page`_).

::

  yaourt -Sy lightsd

Make sure to follow the post-installation instructions: replace ``$USER`` with
the user you usually use.


Please also install ipython if you want to follow the examples in the next
section:

::

  yaourt -Sy ipython

Read on :doc:`/first-steps` to see how to use lightsd.

.. _Yaourt: 
.. _wiki page: https://wiki.archlinux.org/index.php/Yaourt

.. _build_instructions:

Build instructions (for other systems)
--------------------------------------

lightsd should work on any slightly POSIX system (i.e: not Windows), make sure
you have the following requirements installed:

- libevent ≥ 2.0.19 (released May 2012);
- CMake ≥ 2.8.11 (released May 2013).

lightsd is developed and tested from Arch Linux, Debian and Mac OS X; both for
32/64 bits and little/big endian architectures.

For Debian and Ubuntu you would need to install the following packages to build
lightsd:

::

   build-essential cmake libevent-dev git ca-certificates

Please also install ipython if you want to follow the examples in the next
section. On Debian and Ubuntu you would install the ``ipython3`` package.

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
