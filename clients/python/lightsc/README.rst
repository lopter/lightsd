A Python client to control your smart bulbs through lightsd
===========================================================

lightsd_ is a daemon (background service) to control your LIFX_ WiFi "smart"
bulbs. This package allows you to make RPC calls to lightsd to control your
light bulbs from Python. It is built on top of the ``asyncio`` module and
requires Python ≥ 3.5:

.. code-block:: python

   import asyncio
   import click

   from lightsc import LightsView, create_async_lightsd_connection
   from lightsc.requests import (
       GetLightState,
       PowerOff,
       PowerOn,
       SetLightFromHSBK,
    )

   async def example(url, targets):
       async with create_async_lightsd_connection(url) as client:
           click.echo("Connected to lightsd running at {}".format(client.url))

           view = LightsView()
           view.update(await client.apply(GetLightState(targets))
           click.echo("Discovered bulbs: {}".format(view))

           transition_ms = 600
           red_hsbk = (0., 1., 1., 3500)
           click.echo("Turning all bulbs to red in {}ms...".format(transition_ms))
           async with client.batch() as batch:
               batch.append(PowerOn(targets))
               batch.append(SetLightFromHSBK(targets, *red_hsbk, transition_ms=transition_ms))

           click.echo("Restoring original state")
           async with client.batch() as batch:
               for b in view.bulbs:
                   PowerState = PowerOn if b.power else PowerOff
                   hsbk = (b.h, b.s, b.b, b.k)

                   batch.append(PowerState([b.label]))
                   batch.append(SetLightFromHSBK([b.label], *hsbk, transition_ms=transition_ms))

   @click.command()
   @click.option("--lightsd-url", help="supported schemes: tcp+jsonrpc://, unix+jsonrpc://")
   @click.argument("bulb_targets", nargs=-1, required=True)
   def main(lightsd_url, bulb_targets)
       """This example will turn all your bulbs to red before restoring their
       original state.

       If an URL is not provided this script will attempt to connect to
       lightsd's UNIX socket.
       """

       evl = asyncio.get_event_loop()
       evl.run_until_complete(evl.create_task(example(lightsd_url, bulb_targets)))

.. _lightsd: https://www.lightsd.io/
.. _LIFX: http://lifx.co/

.. vim: set tw=80 spelllang=en spell:
