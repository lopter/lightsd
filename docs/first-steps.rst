First steps
===========

Those instructions assume that you have followed the :doc:`installation
instructions <installation>`.

Starting and stopping lightsd
-----------------------------

lightsd listens for UDP traffic from the bulbs on ``0.0.0.0`` port
``udp/56700``.

Mac OS X
~~~~~~~~

.. note::

   This warning will be displayed the first time you start lightsd after an
   install or upgrade:

   .. image:: /images/mac_os_x_startup_warning.png
      :width: 500px

   Click Allow, lightsd uses the network to communicate with your bulbs.

Start lightsd with:

::

   launchctl load ~/Library/LaunchAgents/homebrew.mxcl.lightsd.plist

Stop lightsd with:

::

   launchctl unload ~/Library/LaunchAgents/homebrew.mxcl.lightsd.plist

Check how lightsd is running with:

::

   ps aux | grep lightsd

Read the logs with:

::

   tail -F `brew --prefix`/var/log/lightsd.log

Try to :ref:`toggle your lights <toggle>` and read on some of the examples
bundled with lightsd.

Linux (systemd)
~~~~~~~~~~~~~~~

Start lightsd with:

::

   systemctl start lightsd

Stop lightsd with:

::

   systemctl stop lightsd

Enable lightsd at boot:

::

   systemctl enable lightsd

Check how lightsd is running with:

::

   ps aux | grep lightsd

Read the logs with:

::

   journalctl -x -f _SYSTEMD_UNIT=lightsd.unit

Try to :ref:`toggle your lights <toggle>` and read on some of the examples
bundled with lightsd.

Manually (other systems)
~~~~~~~~~~~~~~~~~~~~~~~~

Assuming you've just built :ref:`lightsd from the sources
<build_instructions>`, lightsd will be in the ``core`` directory [#]_.

The examples are communicating with lightsd through a pipe or an unix socket,
start lightsd with them:

::

   core/lightsd -c pipe -s socket

From another terminal, check how lightsd is running with:

::

   ps aux | grep lightsd

You can stop lightsd with ^C (ctrl+c).

Checkout the :ref:`examples <examples>`.

.. [#] ``build/core`` if you start from the root of the repository.

.. _cli:

Command line options
~~~~~~~~~~~~~~~~~~~~

::

   Usage: lightsd ...

     [-l,--listen addr:port [+]]          Listen for JSON-RPC commands over TCP at
                                          this address (can be repeated).
     [-c,--comand-pipe /command/fifo [+]] Open an unidirectional JSON-RPC
                                          command pipe at this location (can be
                                          repeated).
     [-s,--socket /unix/socket [+]]       Open an Unix socket at this location
                                          (can be repeated).
     [-f,--foreground]                    Stay in the foreground (default).
     [-d,--daemonize]                     Fork in the background.
     [-u,--user user]                     Drop privileges to this user (and the
                                          group of this user if -g is missing).
     [-g,--group group]                   Drop privileges to this group (-g requires
                                          the -u option to be used).
     [-t,--no-timestamps]                 Disable timestamps in logs.
     [-h,--help]                          Display this.
     [-V,--version]                       Display version and build information.
     [-v,--verbosity debug|info|warning|error]

   or,

     --prefix                             Display the install prefix for lightsd.

   or,

     --rundir                             Display the runtime directory for lightsd.

.. _toggle:

Toggle your lights
------------------

::

   `lightsd --prefix`/share/doc/lightsd/examples/toggle

Or, from the root of the repository:

::

   examples/toggle

.. _examples:

Using lightsc.sh
----------------

`lightsc.sh`_ is a small shell script that wraps a few things around lightsd'
command pipe. Once you've sourced it with:

::

   . `lightsd --prefix`/share/lightsd/lightsc.sh

Or, from the root of the repository:

::

   . share/lightsc.sh

You can use the following things to send commands to your bulbs from your
current shell or shell script:

.. data:: LIGHTSD_COMMAND_PIPE

   By default lightsc will use `lightsd --rundir`/pipe but you can set that to
   your own value.

.. function:: lightsc method ...

   Call the given :ref:`method <proto_methods>` with the given arguments.
   lightsc display the generated JSON that was sent.

.. function:: lightsc_get_pipe

   Equivalent to ``${LIGHTSD_COMMAND_PIPE:-`lightsd --rundir`/pipe}`` but also
   check if lightsd is running.

.. function:: lightsc_make_request method ..

   Like lightsc but display the generated json instead of sending it out to
   lightsd: with this and lightsc_get_pipe you can do batch requests:

.. note::

   Keep in mind that arguments must be JSON, you will have to enclose tags and
   labels into double quotes '"likethis"'. The command pipe is write-only: you
   cannot read any result back.

Examples:

Build a batch request manually:

::

   tee `lightsc_get_pipe` <<EOF
   [
       $(lightsc_make_request power_on ${*:-'"#tag"'}),
       $(lightsc_make_request set_light_from_hsbk ${*:-'"#othertag"'} 37.469443 1.0 0.05 3500 600),
       $(lightsc_make_request set_light_from_hsbk ${*:-'["bulb","otherbulb"]'} 47.469443 0.2 0.05 3500 600)
   ]
   EOF

.. _lightsc.sh: https://github.com/lopter/lightsd/blob/master/share/lightsc.sh.in

Using lightsc.py
----------------

`lightsc.py`_ is a minimalistic Python client for lightsd, if you run it as a
program it will open a python shell from which you can directly manipulate your
bulbs. Start lightsc.py with:

::

   `lightsd --prefix`/share/doc/lightsd/examples/lightsc.py

Or, from the root of the repository:

::

   examples/lightsc.py

From there a ``c`` variable has been initialized for you, this a small object
let you directly execute commands on your bulb:

For example toggle your lights again:


.. code-block:: python

   c.power_toggle("*")

Fetch the state of all your bulbs:

.. code-block:: python

   bulbs = {b["label"]: b for b in c.get_light_state("*")["result"]}

Check out :doc:`lightsd's protocol </protocol>` to see everything you can do.

.. _lightsc.py: https://github.com/lopter/lightsd/blob/master/examples/lightsc.py

.. vim: set tw=80 spelllang=en spell:
