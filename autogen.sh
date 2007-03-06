#! /bin/sh

# Generate config.h.in
echo "autoheader..."
autoheader

echo "touch stamp-h"
touch stamp-h

# rerun libtoolize
echo "libtoolize..."
libtoolize --force

# add aclocal.m4 to current dir
echo "aclocal..."
aclocal

# This generates the configure script from configure.in
echo "autoconf..."
autoconf

# Generate Makefile.in from Makefile.am
echo "automake..."
automake

# configure
echo "./configure $*"
./configure $*
