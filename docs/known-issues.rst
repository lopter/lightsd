Known issues
============

Severe issues have been seen with firmwares released with new bulbs models
during 2015. lightsd appears to make those bulbs crash [#]_ after some period of
time; original bulbs with older firmware will not have those issues. It's
interesting to note that both original and newer bulbs are served the same
traffic by lightsd which might point out regressions in LIFX's firmwares.

.. [#] Forcing you to turn them off and back on using a regular light switch.

Power ON/OFF are the only commands with auto-retry, i.e: lightsd will keep
sending the command to the bulb until its state changes. This is not implemented
(yet) for ``set_light_from_hsbk``, ``set_waveform``, ``set_label``, ``tag`` and
``untag``.

In general, crappy WiFi network with latency, jitter or packet loss are gonna be
challenging until lightsd has an auto-retry mechanism, there is also room for
optimizations in how lightsd communicates with the bulbs.

.. vim: set tw=80 spelllang=en spell:
