ncmpc
=====


Description
-----------

ncmpc is a command-line client for the `Music Player Daemon
<http://www.musicpd.org/>`__ (MPD).

By default, ncmpc connects to the local MPD instance.  A different MPD
instance can be selected using the command line options
:option:`--host` and :option:`--port`, or by setting the environment
variables :envvar:`MPD_HOST` and :envvar:`MPD_PORT`::

 ncmpc --host=musicserver --port=44000

You can connect to a "local" socket by setting the host to a file path
(e.g. ":file:`/run/mpd/socket`").  Abstract sockets can be used with a ":file:`@`"
prefix (e.g. ":file:`@mpd`").

To use a password with MPD, set :envvar:`MPD_HOST` to
:samp:`password@host` or use the command line option
:option:`--password`.  Values from the command line override values
from the environment.


Synopsis
--------

 ncmpc [options]


Options
-------

.. option:: -?, --help

 Display help.

.. option:: -V, --version

 Display version information and build-time configuration.

.. option:: -c, --colors

 Enable colors.

.. option:: -C, --no-colors

 Disable colors.

.. option:: -m, --mouse

 Enable mouse.

.. option:: --host=HOST

 The MPD host to connect to.

.. option:: --port=PORT, -p PORT

 The port to connect to.

.. option:: -P, --password=PASSWORD

 Use password when connecting.

.. option:: -f, --config=FILE

 Read configuration from file.

.. option:: -k, --key-file=FILE

 Read key bindings from file.


Configuration
-------------

When ncmpc starts it tries to read the user's configuration file,
:file:`$XDG_CONFIG_HOME/ncmpc/config` (usually
:file:`~/.config/ncmpc/config`).  If no user configuration file is
found then ncmpc tries to load the global settings from
:file:`$SYSCONFDIR/ncmpc/config` (the actual path is displayed in the
output of the :option:`--version` option).  An example configuration
file (:file:`config.sample`) is shipped with ncmpc.


Connection
^^^^^^^^^^

:command:`host = HOST` - The MPD host to connect to.

:command:`port = PORT` - The port to connect to.

:command:`password = PASSWORD` - Use password when connecting.

:command:`timeout = TIMEOUT` - Attempt to reconnect to mpd if a
response to a command is not received within TIMEOUT
seconds. Specifying a value in the configuration file overrides the
":envvar:`MPD_TIMEOUT`" environment variable.  If no timeout is
specified in the configuration file or in the environment, the default
is 5 seconds.


Interface
^^^^^^^^^

:command:`enable-mouse = yes|no` - Enable mouse support (if enabled at compile time).

:command:`screen-list = SCREEN1 SCREEN2...` - A list of screens to
cycle through when using the previous/next screen commands.  Valid
choices, if enabled at compile time, are playlist, browse, artist,
help, search, song, keydef, lyrics, outputs, and chat.

:command:`search-mode = MODE` - Default search mode for the search
screen. MODE must be one of title, artist, album, filename, and
artist+title, or an interger index (0 for title, 1 for artist etc.).

:command:`auto-center = yes|no` - Enable/disable auto center
mode. When auto center mode is enabled ncmpc centers the current track
in the playlist window.

:command:`scroll-offset = NUM` - Keep at least NUM lines above and
below the cursor on list windows, if possible.

:command:`find-show-last = yes|no` - Show the most recent query instead of a blank line for a find.

:command:`find-wrap = yes|no` - Wrapped find mode.

:command:`wrap-around = yes|no` - Wrapped cursor movement.

:command:`bell-on-wrap = yes|no` - Ring bell when find wraps around.

:command:`audible-bell = yes|no` - Sound audible bell on alerts.

:command:`visible-bell = yes|no` - Visible bell on alerts.

:command:`crossfade-time = CROSSFADE TIME` - Default crossfade time in
seconds.

:command:`seek-time = NUM` - Seek forward/backward by NUM seconds.

:command:`lyrics-timeout = NUM` - Quits downloading lyrics of a song
after the timeout of NUM seconds is reached, if NUM is greater than
zero.

:command:`jump-prefix-only = yes|no` - When using the jump command,
search for the prefix of an entry.  That means typing "m" will start
to the first entry which begins with "m".

:command:`lyrics-autosave = yes|no` - Automatically save lyrics after
receiving them.

:command:`lyrics-show-plugin = yes|no` - Show the name of the plugin
used to receive lyrics on the lyrics screen.

:command:`text-editor = EDITOR` - The text editor used for editing
lyrics.

:command:`text-editor-ask = yes|no` - Ask before starting an editor.

:command:`chat-prefix = PREFIX` - Prefix messages send with the chat
screen with PREFIX.  By default they are prefixed with the current
user name enclosed by "<" and ">" and a space (i.e. "<name> ").

:command:`second-column = yes|no` - Display song length in a second
column.


Display
^^^^^^^

:command:`welcome-screen-list = yes|no` - Show a list of the screens
in the top line.

:command:`wide-cursor = yes|no` - Make the cursor as wide as the
screen.

:command:`hardware-cursor = yes|no` - Use the terminal's hardware
cursor instead of inverse colors.

:command:`hide-cursor = NUM` - Hide the playlist cursor after NUM
seconds of inactivity.

:command:`scroll = yes|no` - Scroll the title if it is too long for
the screen.

:command:`scroll-sep = STRING` - the separator to show at the end of
the scrolling title.

:command:`list-format = SONG FORMAT` - The format used to display
songs in the main window.

:command:`search-format = SONG FORMAT` - The format used to display
songs in the search window. Default is to use list-format.

:command:`status-format = SONG FORMAT` - The format used to display
songs on the status line.

:command:`status-message-time = TIME` - The time, in seconds, for
which status messages will be displayed.

:command:`display-time = yes|no` - Display the time in the status bar
when idle.

:command:`timedisplay-type = elapsed/remaining` - Sets whether to
display remaining or elapsed time in the status window.  Default is
elapsed.

:command:`visible-bitrate = yes|no` - Show the bitrate in the status
bar when playing a stream.

:command:`set-xterm-title = yes|no` - Change the XTerm title (ncmpc
will not restore the title).

:command:`xterm-title-format = SONG FORMAT` - The format used to for
the xterm title when ncmpc is playing.


Colors
^^^^^^

:command:`enable-colors = yes|no` - Enable/disable colors.  Defaults
to ``yes``.

The colors used by `ncmpc` can be customized.  The ``color`` directive
can be used to change how a certain style looks.  It can contain a
text color and attributes.  The following standard colors can be
specified by name (`official reference
<https://invisible-island.net/ncurses/man/curs_color.3x.html>`__):

 ``black``, ``red``, ``green``, ``yellow``, ``blue``, ``magenta``,
 ``cyan``, ``white``

Example::

  color list = cyan

Modern terminals support up to 256 colors, but they are not
standardized.  You can select them by specifying the number.
Example::

  color title = 42

The background color can be specified after the text color separated
by a slash.  You can omit the text color if you want to change only
the background color::

  color title = white/blue
  color title = /blue

The color ``none`` uses the terminal's default color.

Attributes can be used to modify the font appearance.  The following
attributes can be specified (`official reference
<https://invisible-island.net/ncurses/man/curs_attr.3x.html>`__),
though many of them are not supported by prevalent terminals:

 ``standout``, ``underline``, ``reverse``, ``blink``, ``dim``,
 ``bold``

Example::

  color alert = red blink

:command:`color background = COLOR` - Set the default background
color.

:command:`color title = COLOR[,ATTRIBUTE]...` - Set the text color and
attributes for the title row.

:command:`color title-bold = COLOR[,ATTRIBUTE]...` - Set the text
color for the title row (the bold part).

:command:`color line = COLOR` - Set the color of the line on the
second row.

:command:`color line-flags = COLOR[,ATTRIBUTE]...` - Set the text
color used to indicate mpd flags on the second row.

:command:`color list = COLOR[,ATTRIBUTE]...` - Set the text color in
the main area of ncmpc.

:command:`color list-bold = COLOR[,ATTRIBUTE]...` - Set the bold text
color in the main area of ncmpc.

:command:`color browser-directory = COLOR[,ATTRIBUTE]...` - Set the
text color used to display directories in the browser window.

:command:`color browser-playlist = COLOR[,ATTRIBUTE]...` - Set the
text color used to display playlists in the browser window.

:command:`color progressbar = COLOR[,ATTRIBUTE]...` - Set the color of
the progress indicator.

:command:`color progressbar-background = COLOR[,ATTRIBUTE]...` - Set the color of
the progress indicator background.

:command:`color status-state = COLOR[,ATTRIBUTE]...` - Set the text
color used to display mpd status in the status window.

:command:`color status-song = COLOR[,ATTRIBUTE]...` - Set the text
color used to display song names in the status window.

:command:`color status-time = COLOR[,ATTRIBUTE]...` - Set the text
color used to display time the status window.

:command:`color alert = COLOR[,ATTRIBUTE]...` - Set the text color
used to display alerts in the status window.

:command:`colordef COLOR = R, G, B` - Redefine any of the base
colors. The RGB values must be integer values between 0 and 1000.
*Note*: Only some terminals allow redefinitions of colors!



Keys
----

When ncmpc starts it tries to read user-defined key bindings from the
:file:`$XDG_CONFIG_HOME/ncmpc/keys` (usually
:file:`~/.config/ncmpc/keys`) file.  If no user-defined key bindings
are found then ncmpc tries to load the global key bindings from
:file:`$SYSCONFDIR/ncmpc/keys` (the actual path is displayed on the
help screen).

You can view ncmpc's key bindings by pressing '1' (help) when ncmpc is
running.  To edit key bindings press 'K' and use the key editor in
ncmpc.


Song Format
-----------

Format of song display for status and the list window.  The metadata
delimiters are: %artist%, %albumartist%, %composer%, %performer%,
%title%, %album%, %shortalbum%, %track%, %disc, %genre%, %name%,
%time%, %date%, %file%, %shortfile%.

The [] operators are used to group output such that if none of the
metadata delimiters between '[' and ']' are matched, then none of the
characters between '[' and ']' are output; literal text is always
output. '&' and '|' are logical operators for AND and OR. '#' is used
to escape characters.

Some useful examples for format are::

 "%file%"

and::

 "[[%artist% - ]%title%]|%file%"

Another one is::

 "[%artist%|(artist n/a)] - [%title%|(title n/a)]"


Chat Protocol
-------------

If ncmpc has been compiled with "chat" support, it uses the
client-to-client protocol available in MPD 0.17 or higher to
communicate with other clients.  In order to receive messages it
subscribes to the channel with the name "chat", and displays any
message sent there as-is.  When the user enters a message, it is first
with the prefix specified by the :command:`chat-prefix` option (or the
default prefix), and then sent to the "chat" channel for others to
read.


Bugs
----

Report bugs on https://github.com/MusicPlayerDaemon/ncmpc/issues


Note
---

Since MPD uses UTF-8, ncmpc needs to convert characters to the charset
used by the local system.  If you get character conversion errors when
your running ncmpc you probably need to set up your locale.  This is
done by setting any of the :envvar:`LC_CTYPE`, :envvar:`LANG` or
:envvar:`LC_ALL` environment variables (:envvar:`LC_CTYPE` only
affects character handling).


See also
--------

:manpage:`mpd(1)`, :manpage:`mpc(1)`, :manpage:`locale(5)`,
:manpage:`locale(7)`
