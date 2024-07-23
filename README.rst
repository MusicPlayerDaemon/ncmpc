ncmpc
=====

ncmpc is a curses client for the `Music Player Daemon
<http://www.musicpd.org/>`__.

.. image:: https://www.musicpd.org/clients/ncmpc/screenshot.png
  :alt: Screenshot of ncmpc


How to compile and install ncmpc
--------------------------------

You need:

- a C++20 compliant compiler (e.g. gcc or clang)
- `libmpdclient <https://www.musicpd.org/libs/libmpdclient/>`__ 2.16
- `ncurses <https://www.gnu.org/software/ncurses/>`__
- `Meson 0.56 <http://mesonbuild.com/>`__ and `Ninja <https://ninja-build.org/>`__

Optional:

- `PCRE <https://www.pcre.org/>`__ (for regular expression support in
  the "find" command)
- `liblirc <https://sourceforge.net/projects/lirc/>`__ (for infrared
  remote support)
- `Sphinx <http://www.sphinx-doc.org/en/master/>`__ (for building
  documentation)

Run ``meson``::

 meson . output --buildtype=debugoptimized -Db_ndebug=true

Compile and install::

 ninja -C output
 ninja -C output install


Links
-----

- `Home page and download <http://www.musicpd.org/clients/ncmpc/>`__
- `Documentation <https://www.musicpd.org/doc/ncmpc/html/>`__
- `git repository <https://github.com/MusicPlayerDaemon/ncmpc/>`__
- `Bug tracker <https://github.com/MusicPlayerDaemon/ncmpc/issues>`__
- `Help translate ncmpc to your native language <https://hosted.weblate.org/projects/ncmpc/>`__
- `Forum <http://forum.musicpd.org/>`__
