Known issues
============

The White 800 appears to be less reliable than the LIFX Original or Color 650.
The grouping (tagging) code of the LIFX White 800 in particular appears to be
bugged: after a tagging operation the LIFX White 800 keep saying it has no tags.

The Color 650 with the firmware 2.1 appears to completely hang sometimes and
you have to restart it, the official LIFX app appears to be affected too.

Power ON/OFF are the only commands with auto-retry, i.e: lightsd will keep
sending the command to the bulb until its state changes. This is not implemented
(yet) for ``set_light_from_hsbk``, ``set_waveform``, ``set_label``, ``tag`` and
``untag``.

In general, crappy WiFi network with latency, jitter or packet loss are gonna be
challenging until lightsd has an auto-retry mechanism, there is also room for
optimizations in how lightsd communicates with the bulbs.

.. vim: set tw=80 spelllang=en spell:
