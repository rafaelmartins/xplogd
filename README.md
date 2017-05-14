# xplogd

WIP

X-Plane 11 plugin that sends position data to a remote HTTP endpoint.

This plugin only works on Linux!


## How to build and use?

After installing the dependencies (libcurl), run:

    $ ./autogen.sh  # (if running from Git, depends on GNU autotools)
    $ ./configure
    $ make plugin

To install the plugin, run:

    $ export XPLANE_DIR=...   # use the directory where you installed X-Plane 11
    $ unzip xplogd-plugin-0.1.zip -d "${XPLANE_DIR}/Resources/plugins/"

Edit the `${XPLANE_DIR}/Resources/plugins/xplogd.txt` file, to point to the URL
of the endpoint you want to `POST` your position data to. This endpoint should
be served through HTTPS for production environment, and include basic auth support.
The user and password should be provided in the URL.


## Protocol

```c
/* xplogd protocol, version 1
 *
 * This is the definition of our protocol. all the fields are separated with
 * a newline '\n' character.
 *
 * The server should return 202 to notify that accepted the data, and always
 * check if the client sent the correct content-type header
 * (application/vnd.xplogd.serialized).
 *
 * When a request is not accepted, the client may take some action to recover,
 * like stop sending for a while, or increase the send intervals.
 */
const char *body_format =
    "1\n"   // protocol version
    "%s\n"  // aircraft icao
    "%s\n"  // aircraft tailnum
    "%f\n"  // latitude in degrees
    "%f\n"  // longitude in degrees
    "%f\n"  // altitude in meters (not feets!)
    "%f\n"  // track in degrees
    "%f\n"  // ground speed in meters per second (not knots!)
    "%f\n"  // air speed in meters per second (not knots!)
    "%f\n"  // vertical speed meters per second (not feets per minute!)
    "";
```
