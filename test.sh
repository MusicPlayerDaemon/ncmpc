#!/bin/sh -e
#
# This shell script tests the build of ncmpc with various compile-time
# options.
#
# Author: Max Kellermann <max@duempel.org>

PREFIX=/tmp/ncmpc
rm -rf $PREFIX

export CFLAGS="-Os"

test -x configure || NOCONFIGURE=1 ./autogen.sh

# all features on, wide curses
./configure --prefix=$PREFIX/full --enable-debug --enable-werror \
    --with-ncursesw \
    --enable-lyrics-screen --enable-colors --enable-lirc
make -j2 install

# all features on, no debugging
./configure --prefix=$PREFIX/full --disable-debug --enable-werror \
    --with-ncursesw \
    --enable-lyrics-screen --enable-colors --enable-lirc
make -j2 install

# all features on, narrow curses
./configure --prefix=$PREFIX/narrow --enable-debug --enable-werror \
    --with-ncurses \
    --enable-lyrics-screen --enable-colors --enable-lirc
make -j2 install

# all features on, no wide characters and no NLS
./configure --prefix=$PREFIX/nonls --enable-debug --enable-werror \
    --disable-wide --disable-nls \
    --enable-lyrics-screen --enable-colors --enable-lirc
make -j2 install

# no bloat
./configure --prefix=$PREFIX/nobloat --enable-debug --enable-werror \
    --disable-wide --disable-nls \
    --enable-lyrics-screen --disable-lirc --disable-key-screen \
    --disable-colors --disable-mouse
make -j2 install

# ncmpc-mini and ncmpc-tiny
CFLAGS="-Os" ./configure --prefix=$PREFIX/mini --disable-debug --enable-werror \
    --enable-mini
make -j2 install
make src/ncmpc-tiny
