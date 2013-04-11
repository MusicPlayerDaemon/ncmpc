#!/bin/sh -e

rm -rf config.cache build
mkdir build

# create po/Makefile.in.in
glib-gettextize --force --copy

# add aclocal.m4 to current dir
echo "aclocal..."
aclocal -I m4

# This generates the configure script from configure.in
echo "autoconf..."
autoconf

echo "autoheader..."
autoheader

# Generate Makefile.in from Makefile.am
echo "automake..."
automake --add-missing $AUTOMAKE_FLAGS


# configure
if test x$NOCONFIGURE = x; then
	echo "./configure $*"
	./configure $*
fi
