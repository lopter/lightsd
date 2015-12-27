Known issues
============

All LIFX bulbs –except for the original model released with their kickstarter
campaign (called the “Original 1000”)– are suffering from a critical firmware
bug [#]_. Under some specific and unknown Wi-Fi conditions LIFX bulbs will
crash, forcing you to turn them off and then back on using a physical light
switch.

Crashed LIFX bulbs will show up as unavailable [#]_ in the LIFX mobile
application and they will be missing in lightsd's ``get_light_state`` result
list.

The only known workaround at this time is to try different Wi-Fi channel, you
might find one that doesn't trigger the bug for your bulbs and radio-frequency
environment (LIFX bulbs are running in the very busy 2.4GHz band).

.. [#] More accurately a firmware bug within the Qalcomm Atheros chip upon which
       all LIFX products released since 2015 are built.

.. [#] Kinda like this emoji: ⛔️.

----

Power ON/OFF are the only commands with auto-retry, i.e: lightsd will keep
sending the command to the bulb until its state changes. This is not implemented
(yet) for ``set_light_from_hsbk``, ``set_waveform``, ``set_label``, ``tag`` and
``untag``.

In general, crappy Wi-Fi network with latency, jitter or packet loss are gonna
be challenging until lightsd has an auto-retry mechanism, there is also room for
optimizations in how lightsd communicates with the bulbs.

.. vim: set tw=80 spelllang=en spell:
