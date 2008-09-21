#! /bin/sh

# Generate config.h.in
echo "touch stamp-h"
touch stamp-h

# add aclocal.m4 to current dir
echo "aclocal..."
aclocal -I $PWD/m4

# rerun libtoolize
echo "libtoolize..."
libtoolize --force

# This generates the configure script from configure.in
echo "autoconf..."
autoconf

echo "autoheader..."
autoheader

# Generate Makefile.in from Makefile.am
echo "automake..."
automake --add-missing


# configure
if test x$NOCONFIGURE = x; then
	echo "./configure $*"
	./configure $*
fi
