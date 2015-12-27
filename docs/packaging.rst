Packaging lightsd
=================

lightsd has already been packaged for:

- Mac OS X's Homebrew: https://github.com/lopter/homebrew-lightsd;
- Arch Linux (AUR): https://aur.archlinux.org/packages/lightsd/;
- Debian/Ubuntu: https://github.com/lopter/dpkg-lightsd;
- OpenWRT: https://github.com/lopter/openwrt-lightsd.

Here is what you need to know to build lightsd in order to be distributed:

- make sure CMake's build type is set to ``RELEASE``;
- lightsd needs a runtime directory, this must be configured via CMake using
  ``LGTD_RUNTIME_DIRECTORY`` (e.g: ``/run/lightsd``);
- lightsd needs a lightsd user, make sure it is created when the package gets
  installed and make sure the user is informed, e.g::

      lightsd runs under the `lightsd' user and group by default; add yourself
      to this group to be able to open lightsd's socket and pipe under
      /run/lightsd:

        gpasswd -a $USER lightsd

      Re-open your current desktop or ssh session for the change to take effect.
      Then use systemctl to start lightsd; you can start playing with lightsd
      with:

        `lightsd --prefix`/share/doc/lightsd/examples/lightsc.py
- make sure lightsd is built with hardening flags;
- make sure the protocol documentation and the examples are properly shipped.

Overall, I'm all in favor of a tight collaboration between up and downstream and
have enough experience to package lightsd myself on pretty much any operating
system; most of it will be automated with the release process down the road.
What I really need is help to get lightsd in official distribution channels and
repositories.

.. vim: set tw=80 spelllang=en spell:
