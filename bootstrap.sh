#! /bin/sh
# Check Autoconf version
if [ -x `which autoconf` ]; then
	AC_VER=`autoconf --version | head -1 | sed 's/^[^0-9]*//'`
	AC_VER_MAJOR=`echo $AC_VER | cut -f1 -d'.'`
	AC_VER_MINOR=`echo $AC_VER | cut -f2 -d'.' | sed 's/[^0-9]*$//'`

	if [ "$AC_VER_MAJOR" -lt "2" ]; then
		echo "Autoconf 2.13 or greater needed to build configure."
		exit 1
	fi

	if [ "$AC_VER_MINOR" -lt "13" ]; then
		echo "Autoconf 2.13 or greater needed to build configure."
		exit 1
	fi

	if [ "$AC_VER_MINOR" -lt "50" ]; then
		if [ ! -e configure.in ]; then
			ln -s configure.ac configure.in
		fi
		echo "If you see some warnings about cross-compiling, don't worry; this is normal."
	else
		echo "rm -f configure.in ?"
	fi
else
	echo Autoconf not found. AlsaPlayer CVS requires autoconf to bootstrap itself.
	exit 1
fi

run_cmd() {
    echo running $* ...
    if ! $*; then
			echo failed!
			exit 1
    fi
}

# Check if /usr/local/share/aclocal exists
if [ -d /usr/local/share/aclocal ]; then
	ACLOCAL_INCLUDE="$ACLOCAL_INCLUDE -I /usr/local/share/aclocal"
fi	

if [ -d m4 ] ; then
  run_cmd cat m4/*.m4 > acinclude.m4
fi
run_cmd aclocal $ACLOCAL_INCLUDE
run_cmd autoheader 
run_cmd libtoolize --automake
run_cmd automake --add-missing
run_cmd autoconf
echo
echo "Now run './configure'"
echo
