dnl
dnl  $Id: configure.ac,v 1.10 2004/03/18 09:33:43 kalle Exp $
dnl

AC_INIT(main.c)
AM_INIT_AUTOMAKE(ncmpc, 0.10.1)

dnl Check for programs
AC_PROG_CC
AC_PROG_INSTALL
dnl AC_PROG_LIBTOOL

dnl =======================================================
dnl initialize variables
dnl =======================================================

set -- $CFLAGS
CFLAGS="-Wall $CFLAGS"

keydef_screen=yes

dnl
dnl Check for types
dnl
dnl AC_CHECK_TYPE(socklen_t, 
dnl 	      AC_DEFINE(HAVE_SOCKLEN_T, 1, socklen_t defined in sys/socket.h),
dnl )

AC_SOCKLEN_T


dnl
dnl Check for headers
dnl

AC_CHECK_HEADER(libgen.h,
                AC_DEFINE(HAVE_LIBGEN_H, 1, glibc - libgen.h),
		,)

AC_CHECK_HEADER(locale.h,
 	        AC_DEFINE(HAVE_LOCALE_H, 1, locale.h),
		,)


dnl
dnl Check for functions
dnl
AC_CHECK_FUNCS(basename strcasestr)


dnl
dnl Check for libraries
dnl

dnl ncurses
AC_CHECK_LIB(ncurses, initscr,, [AC_MSG_ERROR(ncurses library is required)])
LIBS="$LIBS -lncurses"

dnl glib-2.0
AM_PATH_GLIB_2_0(, , [AC_MSG_ERROR(glib-2.x is required)], glib)

dnl popt
AC_CHECK_LIB(popt, 
             poptGetArg,
             LIBS="$LIBS -lpopt",
             AC_MSG_ERROR(Missing popt command line parsing library))




dnl Debugging 
AC_ARG_ENABLE(debug, 
              AS_HELP_STRING([--enable-debug],[Enable debugging]),
	      , 
	      enable_debug=no)

if test "$enable_debug" = yes; then
   CFLAGS="$CFLAGS -g -DDEBUG"
fi

dnl Key editor
AC_ARG_ENABLE(key-editor, 
              AS_HELP_STRING([--enable-key-editor],[Enable key editor]),
	      keydef_screen=no,
	      keydef_screen=yes)
if test "$keydef_screen" = yes; then
       CFLAGS="$CFLAGS -DENABLE_KEYDEF_SCREEN"
fi
dnl   AC_DEFINE(ENABLE_KEYDEF_SCREEN, 1, [Enable builtin key editor]), 


dnl Default host
AC_ARG_WITH(default-host,
	    AS_HELP_STRING([--with-default-host=ARG],
                           [Default MPD host (localhost)]),
            DEFAULT_HOST="$withval",
            DEFAULT_HOST="localhost")

dnl Default port
AC_ARG_WITH(default-port,
            AS_HELP_STRING([--with-default-port=ARG],[Default port (2100)]),
            DEFAULT_PORT="$withval",
            DEFAULT_PORT="2100")


CFLAGS="$CFLAGS $GLIB_CFLAGS -DSYSCONFDIR=\\\"\$(sysconfdir)\\\""
LIBS="$LIBS $GLIB_LIBS"


dnl Autoheader
AC_DEFINE_UNQUOTED(DEFAULT_PORT,      $DEFAULT_PORT,     Default MPD port)
AC_DEFINE_UNQUOTED(DEFAULT_PORT_STR, "$DEFAULT_PORT",    Default MPD port)
AC_DEFINE_UNQUOTED(DEFAULT_HOST,     "$DEFAULT_HOST",    Default MPD host)

AM_CONFIG_HEADER(config.h)

AC_OUTPUT(doc/Makefile Makefile)

echo "
Configuration:  
  prefix:                             ${prefix}
  sysconfdir:                         ${sysconfdir}
  Default MPD host:                   ${DEFAULT_HOST}
  Default MPD port:                   ${DEFAULT_PORT}
  Enable debugging:                   ${enable_debug}
  Key edit screen:                    ${keydef_screen}
"
echo
