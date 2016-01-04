The lights daemon protocol
==========================

The lightsd protocol is implemented on top of `JSON-RPC 2.0`_. This section
covers the available methods and how to target bulbs.

Since lightsd implements JSON-RPC without any kind of framing like it usually is
the case (using HTTP), this section also explains how to implement your own
lightsd client in `Writing a client for lightsd`_.

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
|                             | (that's the bulb mac address, see below)       |
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

Writing a client for lightsd
----------------------------

lightsd does JSON-RPC directly over TCP, requests and responses aren't framed in
any way like it is usually done by using HTTP.

This means that you will very likely need to write a JSON-RPC client
specifically for lightsd. You're actually encouraged to do that as lightsd will
probably augment JSON-RPC via lightsd specific `JSON-RPC extensions`_ in the
future.

.. _JSON-RPC extensions: http://www.jsonrpc.org/specification#extensions

JSON-RPC over TCP
~~~~~~~~~~~~~~~~~

JSON-RPC works in a request/response fashion: the socket (network connection) is
never used in a full-duplex fashion (data never flows in both direction at the
same time):

#. Write (send) a request on the socket;
#. Read (receive) the response on the socket;
#. Repeat.

Writing the request is easy: do successive write (send) calls until you have
successfully sent the whole request. The next step (reading/receiving) is a bit
more complex. And that said, if the response isn't useful to you, you can ask
lightsd to omit it by turning your request into a `notification`_: if you remove
the JSON-RPC id, then you can just send your requests (now notifications) on the
socket in a fire and forget fashion.

.. _notification: http://www.jsonrpc.org/specification#notification

Otherwise to successfully read and decode JSON-RPC over TCP you will need to
implement your own read loop, the algorithm follows. It focuses on the low-level
details, adapt it for the language and platform you are using:

#. Prepare an empty buffer that you can grow, we will accumulate received data
   in it;
#. Start an infinite loop and start a read (receive) for a chunk of data (e.g:
   4KiB), accumulate the received data in the previous buffer, then try to
   interpret the data as JSON:

   - if valid JSON can be decoded then break out of the loop;
   - else data is missing and continue the loop;
#. Decode the JSON data.

Here is a complete Python 3 request/response example:

.. code-block:: python
   :linenos:

   import json
   import socket
   import uuid

   READ_SIZE = 4096
   ENCODING = "utf-8"

   # Connect to lightsd, here using an Unix socket. The rest of the example is
   # valid for TCP sockets too. Replace /run/lightsd/socket by the output of:
   # echo $(lightsd --rundir)/socket
   lightsd_socket = socket.socket(socket.AF_UNIX)
   lightsd_socket.connect("/run/lightsd/socket")
   lightsd_socket.settimeout(2)  # seconds

   # Prepare the request:
   request = json.dumps({
       "method": "get_light_state",
       "params": ["*"],
       "jsonrpc": "2.0",
       "id": str(uuid.uuid4()),
   }).encode(ENCODING, "surrogateescape")

   # Send it:
   lightsd_socket.sendall(request)

   # Prepare an empty buffer to accumulate the received data:
   response = bytearray()
   while True:
       # Read a chunk of data, and accumulate it in the response buffer:
       response += lightsd_socket.recv(READ_SIZE)
       try:
           # Try to load the received the data, we ignore encoding errors
           # since we only wanna know if the received data is complete.
           json.loads(response.decode(ENCODING, "ignore"))
           break  # Decoding was successful, we have received everything.
       except Exception:
           continue  # Decoding failed, data must be missing.

   response = response.decode(ENCODING, "surrogateescape")
   print(json.loads(response))

Notes
~~~~~

- Use an incremental JSON parser if you have one handy: for responses multiple
  times the size of your receive window it will let you avoid decoding the whole
  response at each iteration of the read loop;
- lightsd supports batch JSON-RPC requests, use them!

.. vim: set tw=80 spelllang=en spell:
