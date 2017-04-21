ncmpc
=====

Overview
--------

ncmpc is a curses client for the `Music Player Daemon
<http://www.musicpd.org/>`__.  ncmpc connects to a MPD running on a
machine on the local network, and controls this with an interface
inspired by cplay.

Music Player Daemon (MPD) allows remote access for playing music (MP3, 
Ogg Vorbis, FLAC, AAC, and wave files) and managing playlists. MPD is 
designed for integrating a computer into a stereo system that provides 
control for music playback over a local network. Read more at musicpd.org



How to compile and install ncmpc
--------------------------------

See the `INSTALL <INSTALL>`__ file.


Usage
-----

ncmpc connects to a MPD running on a machine on the local network. 
By default, ncmpc  connects  to  localhost:6600.   This  can  be
changed  either  at  compile-time,  or  by  exporting  the MPD_HOST and
MPD_PORT environment variables, or by the command line options `--host`
and `--port`::

 ncmpc --host=musicserver --port=44000

For more information please view ncmpc's manual page.


Web Page, Forums and Bug Reports
--------------------------------

Home page and download:
        http://www.musicpd.org/clients/ncmpc/

MPD's home page:
        http://www.musicpd.org/

Bug tracker:
        http://bugs.musicpd.org/

Forum:
        http://forum.musicpd.org/
