dnl
dnl  $Id$
dnl

AC_INIT
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE(ncmpc, 0.11.0-svn)
AM_CONFIG_HEADER([config.h])

dnl Check for programs
AC_PROG_CC
AC_PROG_INSTALL
dnl AC_PROG_LIBTOOL

dnl =======================================================
dnl initialize variables
dnl =======================================================

set -- $CFLAGS


dnl
dnl Check for types
dnl

AC_SOCKLEN_T


dnl
dnl Check for headers
dnl

AC_CHECK_HEADER([libgen.h],
                AC_DEFINE([HAVE_LIBGEN_H], [1], [glibc - libgen.h]),
		,)

AC_CHECK_HEADER([locale.h],
 	        AC_DEFINE([HAVE_LOCALE_H], [1], [locale.h]),
		,)


dnl
dnl Check for functions
dnl
AC_CHECK_FUNCS([basename strcasestr])


dnl
dnl Check for libraries
dnl

dnl ncurses
AC_CHECK_LIB([ncurses], [initscr],, 
             [AC_MSG_ERROR(ncurses library is required)])
LIBS="$LIBS -lncurses"

dnl popt
AC_CHECK_LIB([popt], 
             [poptGetArg],
             [LIBS="$LIBS -lpopt"],
             AC_MSG_ERROR([Missing popt command line parsing library]))


dnl Check for glib-2
PKG_CHECK_MODULES([GLIB], 
		  [glib-2.0 >= 2.2],
		  ,
		  AC_MSG_ERROR([glib-2.2 is required]))

dnl i18n
ALL_LINGUAS=""
AC_MSG_CHECKING([whether to include NLS support])
AC_ARG_ENABLE([nls],
              AC_HELP_STRING([--enable-nls],
	                     [include natural language support @<:@default=yes@:>@]),
              [nls="$enableval"],
	      [nls=yes])
AC_MSG_RESULT([$nls])
if test "x$nls" = "xyes"; then
   ALL_LINGUAS="sv"
   AM_GLIB_GNU_GETTEXT
   GETTEXT_PACKAGE=$PACKAGE
   AC_SUBST(GETTEXT_PACKAGE)
   AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], 
                      ["${GETTEXT_PACKAGE}"], 
                      [gettext domain])
  nls=$gt_cv_have_gettext
fi


dnl Debugging 
AC_MSG_CHECKING([whether to build with debug support])
AC_ARG_ENABLE([debug], 
              AC_HELP_STRING([--enable-debug],
                             [Enable debugging @<:@default=no@:>@)]),
	      [enable_debug="$enableval"], 
	      [enable_debug=no])
AC_MSG_RESULT([$enable_debug])
if test "x$enable_debug" = "xyes" ; then
   CFLAGS="$CFLAGS -Wall -g -DDEBUG"
fi

dnl Optional screen - key editor
AC_MSG_CHECKING([whether to include the key editor screen])
AC_ARG_ENABLE([key-screen], 
              AC_HELP_STRING([--enable-key-screen],
	                     [Enable key editor screen @<:@default=yes@:>@]),
	      [keydef_screen="$enableval"],
	      [keydef_screen=yes])
AC_MSG_RESULT([$keydef_screen])
if test "x$keydef_screen" = "xyes" ; then
   AC_DEFINE(ENABLE_KEYDEF_SCREEN, 1, [Enable key editor screen])
fi

dnl Optional screen - clock
AC_MSG_CHECKING([whether to include the clock screen])
AC_ARG_ENABLE([clock-screen], 
              AC_HELP_STRING([--enable-clock-screen],
	                     [Enable clock screen @<:@default=no@:>@]),
	      [clock_screen="$enableval"],
	      [clock_screen=no])
AC_MSG_RESULT([$clock_screen])
if test "x$clock_screen" = "xyes" ; then
   AC_DEFINE(ENABLE_CLOCK_SCREEN, 1, [Enable clock screen])
fi

dnl Default host
AC_MSG_CHECKING([for default MPD host])
AC_ARG_WITH([default-host],
	    AC_HELP_STRING([--with-default-host=ARG],
	                   [Default MPD host @<:@localhost@:>@]),
            [DEFAULT_HOST="$withval"],
            [DEFAULT_HOST="localhost"])
AC_MSG_RESULT([$DEFAULT_HOST])
AC_DEFINE_UNQUOTED([DEFAULT_HOST], ["$DEFAULT_HOST"], [Default MPD host])

dnl Default port
AC_MSG_CHECKING([for default MPD port])
AC_ARG_WITH([default-port],
            AC_HELP_STRING([--with-default-port=ARG],
                           [Default port @<:@6600@:>@]),
            [DEFAULT_PORT="$withval"],
            [DEFAULT_PORT="6600"])
AC_MSG_RESULT([$DEFAULT_PORT])
AC_DEFINE_UNQUOTED([DEFAULT_PORT],     [$DEFAULT_PORT],   [Default MPD port])
AC_DEFINE_UNQUOTED([DEFAULT_PORT_STR], ["$DEFAULT_PORT"], [Default MPD port])


AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile po/Makefile.in])
AC_OUTPUT

