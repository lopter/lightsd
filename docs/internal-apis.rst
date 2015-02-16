lightds internal APIs
=====================

lgtd_proto_*
------------

.. function:: lgtd_proto_power_off(target)

   Turns off the targeted bulbs.

.. function:: lgtd_proto_power_on(target)

   Turns on the targeted bulbs.

.. function:: lgtd_proto_set_light_from_hsbk(target, h, s, b, k)

   :param string target: targeted bulbs;
   :param int h: Hue from 0 (0°) to 65535 (360°);
   :param int s: Saturation from 0 to 65535;
   :param int b: Brightness from 0 to 65535;
   :param int k: Temperature in Kelvin (max 9000K).

.. vim: set tw=80 spelllang=en spell:
