The lights daemon protocol
==========================

The lightsd protocol is implemented on top of `JSON-RPC 2.0`_.

.. _JSON-RPC 2.0: http://www.jsonrpc.org/specification

Targeting bulbs
---------------

Commands that manipulate bulbs will take a *target* argument to define on which
bulb(s) the operation should apply:

+-----------------------------+-----------------------------------------------+
| ``\*``                      | targets all bulbs                             |
+-----------------------------+-----------------------------------------------+
| ``#TagName``                | targets bulbs tagged with *TagName*           |
+-----------------------------+-----------------------------------------------+
| ``124f31a5``                | directly target the bulb with the given id    |
+-----------------------------+-----------------------------------------------+
| ``label``                   | directly target the bulb with the given label |
+-----------------------------+-----------------------------------------------+
| ``[#TagName, 123f31a5]``    | composite target (JSON array)                 |
+-----------------------------+-----------------------------------------------+

A target is either a string, a hexadecimal number (without any prefix like 0x)
or an array of targets.

Available methods
-----------------

.. function:: power_off(target)

   Power off the given bulb(s).

.. function:: power_on(target)

   Power on the given bulb(s).

.. function:: set_light_from_hsbk(target, h, s, b, k, t)

   :param float h: Hue from 0 to 360.
   :param float s: Saturation from 0 to 1.
   :param float b: Brightness from 0 to 1.
   :param int k: Temperature in Kelvin from 2500 to 9000.
   :param int t: Transition duration to this color in ms.

.. function:: set_waveform(target, waveform, h, s, b, k, period, cycles, skew_ratio, transient)

   :param string waveform: One of ``SAW``, ``SINE``, ``HALF_SINE``,
                           ``TRIANGLE``, ``PULSE``.
   :param float h: Hue from 0 to 360.
   :param float s: Saturation from 0 to 1.
   :param float b: Brightness from 0 to 1.
   :param int k: Temperature in Kelvin from 2500 to 9000.
   :param int period: milliseconds per cycle.
   :param int cycles: number of cycles.
   :param float skew_ratio: from 0 to 1.
   :param bool transient: if true the target will keep the color it has at the
                          end of the waveform, otherwise it will revert back to
                          its original state.

.. function:: tag_list(tag)

    Return an array of labels or adresses of the devices having the given tag.

.. vim: set tw=80 spelllang=en spell:
