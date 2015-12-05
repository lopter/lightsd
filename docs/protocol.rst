The lights daemon protocol
==========================

The lightsd protocol is implemented on top of `JSON-RPC 2.0`_.

.. _JSON-RPC 2.0: http://www.jsonrpc.org/specification

Targeting bulbs
---------------

Commands that manipulate bulbs will take a *target* argument to define on which
bulb(s) the operation should apply. The target argument is either a string
(identifying a target as explained in the following table), or an array of
strings (targets).

+-----------------------------+------------------------------------------------+
| ``*``                       | targets all bulbs                              |
+-----------------------------+------------------------------------------------+
| ``#TagName``                | targets bulbs tagged with *TagName*            |
+-----------------------------+------------------------------------------------+
| ``124f31a5``                | directly target the bulb with the given id     |
|                             | (mac addr, see below)                          |
+-----------------------------+------------------------------------------------+
| ``label``                   | directly target the bulb with the given Label  |
+-----------------------------+------------------------------------------------+
| ``[#TagName, 123f31a5]``    | composite target (JSON array)                  |
+-----------------------------+------------------------------------------------+

The mac address (id) of each bulb can be found with get_light_state_ under the
``_lifx`` map, e.g:

::

   "_lifx": {
       "addr": "d0:73:d5:02:e5:30",
       "gateway": {
           […]

This bulb has id ``d073d502e530``.

.. note::

   The maximum supported length for labels and tag names by LIFX bulbs is 32.
   Anything beyond that will be ignored.

.. _proto_methods:

Available methods
-----------------

.. function:: power_off(target)

   Power off the given bulb(s).

.. function:: power_on(target)

   Power on the given bulb(s).

.. function:: power_toggle(target)

   Power on (if they are off) or power off (if they are on) the given bulb(s).

.. function:: set_light_from_hsbk(target, h, s, b, k, t)

   :param float h: Hue from 0 to 360.
   :param float s: Saturation from 0 to 1.
   :param float b: Brightness from 0 to 1.
   :param int k: Temperature in Kelvin from 2500 to 9000.
   :param int t: Transition duration to this color in ms.

.. function:: set_waveform(target, waveform, h, s, b, k, period, cycles, skew_ratio, transient)

   :param string waveform: One of ``SAW``, ``SINE``, ``HALF_SINE``,
                           ``TRIANGLE``, ``SQUARE``.
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

   The meaning of the ``skew_ratio`` argument depends on the type of waveform:

   +---------------+-----------------------------------------------------------+
   | ``SAW``       | Should be 0.5.                                            |
   +---------------+-----------------------------------------------------------+
   | ``SINE``      | Defines the peak point of the function, 0.5 gives you a   |
   |               | sine and 1 or 0 will give you cosine. Ignored by firmware |
   |               | 1.1.                                                      |
   +---------------+-----------------------------------------------------------+
   | ``HALF_SINE`` | Should be 0.5.                                            |
   +---------------+-----------------------------------------------------------+
   | ``TRIANGLE``  | Defines the peak point of the function like ``SINE``.     |
   |               | Ignored by firmware 1.1.                                  |
   +---------------+-----------------------------------------------------------+
   | ``SQUARE``    | Ratio of a cycle the targets are set to the given color.  |
   +---------------+-----------------------------------------------------------+

.. function:: get_light_state(target)

   Return a list of dictionnaries, each dict representing the state of one
   targeted bulb, the list is not in any specific order. Each dict has the
   following fields:

   - hsbk: tuple (h, s, b, k) see function:`set_light_from_hsbk`;
   - label: bulb label (utf-8 encoded string);
   - power: boolean, true when the bulb is powered on false otherwise;
   - tags: list of tags applied to the bulb (utf-8 encoded strings).

.. function:: set_label(target, label)

   Label the target bulb(s) with the given label.

   .. note::

      Use :func:`tag` instead set_label to give a common name to multiple bulbs.

.. function:: tag(target, label)

   Tag (group) the given target bulb(s) with the given label (group name), then
   label can be used as a target by prefixing it with ``#``.

   To add a device to an existing "group" simply do:

   ::

      tag(["#myexistingtag", "bulbtoadd"], "myexistingtag")

   .. note::

      Notice how ``#`` is prepended to the tag label depending on whether it's
      used as a target or a regular argument.

.. function:: untag(target, label)

   Remove the given tag from the given target bulb(s). To completely delete a
   tag (group), simple do:

   ::

      untag("#myexistingtag", "myexistingtag")

Notes
-----

- lightsd supports batch JSON-RPC requests, use them!

.. vim: set tw=80 spelllang=en spell:
