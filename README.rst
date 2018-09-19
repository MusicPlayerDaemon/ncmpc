ncmpc
=====

ncmpc is a curses client for the `Music Player Daemon
<http://www.musicpd.org/>`__.


How to compile and install ncmpc
--------------------------------

You need:

- a C++14 compliant compiler (e.g. gcc or clang)
- `libmpdclient <https://www.musicpd.org/libs/libmpdclient/>`__ 2.9
- `GLib 2.30 <https://developer.gnome.org/glib/>`__
- `ncurses <https://www.gnu.org/software/ncurses/>`__
- `Meson 0.47 <http://mesonbuild.com/>`__ and `Ninja <https://ninja-build.org/>`__

Optional:

- `liblirc <https://sourceforge.net/projects/lirc/>`__ (for infrared
  remote support)
- `Sphinx <http://www.sphinx-doc.org/en/master/>`__ (for building
  documentation)

Run ``meson``::

 meson . output

Compile and install::

 ninja -C output
 ninja -C output install


Usage
-----

ncmpc connects to a MPD running on a machine on the local network. 
By default, ncmpc  connects  to  localhost:6600.   This  can  be
changed  either  at  compile-time,  or  by  exporting  the MPD_HOST and
MPD_PORT environment variables, or by the command line options ``--host``
and ``--port``::

 ncmpc --host=musicserver --port=44000

For more information please view ncmpc's manual page.


Links
-----

- `Home page and download <http://www.musicpd.org/clients/ncmpc/>`__
- `git repository <https://github.com/MusicPlayerDaemon/ncmpc/>`__
- `Bug tracker <https://github.com/MusicPlayerDaemon/ncmpc/issues>`__
- `Help translate ncmpc to your native language <https://hosted.weblate.org/projects/ncmpc/>`__
- `Forum <http://forum.musicpd.org/>`__
