# iot_bubundorf

Arduino code to control by bubendorf roller shutters through a MKR1000.

This is a simple http api for the remote control.

Setup
-----

The MKR1000 has pin 6 and 9 on a double relay.
Each relay control one "button" of the bubendorf remote control.

Control
-------

This system is controled using simple http requests:
- `GET /up` to rise roller shutters
- `GET /down` to lower roller shutters.
- `GET /switch` to rise if low or lower if up.
- `GET /status` to know current shutters status. Of course, this relies on arduino memory and get reset at the time than the board. Returns an integer: `0` when rollers are up, `100` when rollers are down, `50` when unknown.

There is currently no easy way to control how much up/down you want the rollers to be. An improvement could be to measure time to get up/down and apply fraction.

Credits
-------

Code presented here is a merge of the following sources:
- Same thing using Uno+ethernet shield: https://www.geeek.org/comment-domotiser-ses-volets-radio-pour-moins-de-50-960.html (french)
- WIFI101 example: WifiSimpleWebServer

TODOs
----

[] Handle commands such as "up 70%"
[] Real http request management
[] Accept status set by an external source (for initialization for instance)

License
-------

GPL License.
