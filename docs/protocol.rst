The lights daemon protocol
==========================

The lightsd protocol is implemented on top of `JSON-RPC 2.0`_.

.. _JSON-RPC 2.0: http://www.jsonrpc.org/specification

Targeting bulbs
---------------

Commands that manipulate bulbs will take a *target* argument to define on which
bulb(s) the operation should apply:

+-----------------------------+--------------------------------------------+
| ``\*``                      | targets all bulbs                          |
+-----------------------------+--------------------------------------------+
| ``#TagName``                | targets bulbs tagged with *TagName*        |
+-----------------------------+--------------------------------------------+
| ``124f31a5``                | directly target the bulb with the given id |
+-----------------------------+--------------------------------------------+
| ``[\*, #Kitchen, 123456]``  | compose different targets together         |
+-----------------------------+--------------------------------------------+

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


.. vim: set tw=80 spelllang=en spell:
