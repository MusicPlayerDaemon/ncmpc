dnl
dnl  $Id$
dnl

AC_INIT
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE([ncmpc], [0.11.1-rc3])
AM_CONFIG_HEADER([config.h])

dnl Check for programs
AC_PROG_CC
AC_PROG_INSTALL
dnl AC_PROG_LIBTOOL

dnl =======================================================
dnl initialize variables
dnl =======================================================

dnl i18n
ALL_LINGUAS="es de fr ru sv"

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
AC_CHECK_LIB([ncurses], 
	     [initscr],
             [LIBS="$LIBS -lncurses"], 
             [AC_MSG_ERROR(ncurses library is required)])


dnl Check for glib-2.4
PKG_CHECK_MODULES([GLIB], 
		  [glib-2.0 >= 2.4],
		  [glib24=yes],
		  [AC_MSG_WARN([glib-2.4 is required for NLS support!])])
if test "x$glib24" != "xyes"; then
   PKG_CHECK_MODULES([GLIB], 
		     [glib-2.0 >= 2.2],
		     [nls=no],
		     [AC_MSG_ERROR([glib-2.2 is required])])
fi


dnl i18n
AM_NLS
if test "x$glib24" = "xyes"; then
  nls=$USE_NLS
else
  USE_NLS=no
  AC_SUBST(USE_NLS)
fi

if test "x$nls" = "xyes"; then
   AM_GLIB_GNU_GETTEXT
   GETTEXT_PACKAGE=$PACKAGE
   AC_SUBST(GETTEXT_PACKAGE)
   AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], 
                      ["${GETTEXT_PACKAGE}"], 
                      [gettext domain])
  nls=$gt_cv_have_gettext
  if test "x$nls" != "xyes"; then
     AC_MSG_WARN([NLS support disabled!])
  fi
else
  AM_PO_SUBDIRS
fi


dnl disbale raw mode
AC_MSG_CHECKING([Place the terminal into raw mode])
AC_ARG_ENABLE([raw-mode],
	AC_HELP_STRING([--disable-raw-mode],
		       [Disable raw terminal mode]),
		       [use_raw=$enableval], 
                       [use_raw=yes])
AC_MSG_RESULT([$use_raw])
if test "x$use_raw" = "xyes" ; then
  AC_DEFINE([ENABLE_RAW_MODE], [1], [Place the terminal into raw mode])
fi

dnl Debugging 
AC_MSG_CHECKING([whether to build with debug support])
AC_ARG_ENABLE([debug], 
              AC_HELP_STRING([--enable-debug],
                             [Enable debugging @<:@default=no@:>@]),
	      [enable_debug="$enableval"], 
	      [enable_debug=no])
AC_MSG_RESULT([$enable_debug])
if test "x$enable_debug" = "xyes" ; then
   CFLAGS="$CFLAGS -Wall -g -DDEBUG"
fi

dnl Optional screen - help screen
#AC_MSG_CHECKING([whether to include the help screen])
#AC_ARG_ENABLE([help-screen], 
#              AC_HELP_STRING([--enable-help-screen],
#	                     [Enable the help screen @<:@default=yes@:>@]),
#	      [help_screen="$enableval"],
#	      [help_screen=yes])
#AC_MSG_RESULT([$help_screen])
#if test "x$help_screen" = "xyes" ; then
#   AC_DEFINE(ENABLE_HELP_SCREEN, 1, [Enable the help screen])
#fi

dnl Optional screen - browse screen
#AC_MSG_CHECKING([whether to include the browse screen])
#AC_ARG_ENABLE([browse-screen], 
#              AC_HELP_STRING([--enable-browse-screen],
#	                     [Enable the browse screen @<:@default=yes@:>@]),
#	      [browse_screen="$enableval"],
#	      [browse_screen=yes])
#AC_MSG_RESULT([$browse_screen])
#if test "x$browse_screen" = "xyes" ; then
#   AC_DEFINE(ENABLE_BROWSE_SCREEN, 1, [Enable the browse screen])
#fi

dnl Optional - curses getmouse support
AC_MSG_CHECKING([whether to include mouse support])
AC_ARG_ENABLE([mouse], 
              AC_HELP_STRING([--enable-mouse],
	                     [Enable curses getmouse support @<:@default=yes@:>@]),
	      [getmouse="$enableval"],
	      [getmouse=yes])
AC_MSG_RESULT([$getmouse])
if test "x$getmouse" = "xyes" ; then
   AC_CHECK_LIB([ncurses], 
	        [getmouse],
		[AC_DEFINE([HAVE_GETMOUSE], [1], [ncurses - getmouse()])],
		[])
fi

dnl search screen
AC_MSG_CHECKING([whether to include the search screen])
AC_ARG_ENABLE([search-screen], 
              AC_HELP_STRING([--enable-search-screen],
	                     [Enable search screen (EXPERIMENTAL) @<:@default=no@:>@]),
	      [search_screen="$enableval"],
	      [search_screen=no])
AC_MSG_RESULT([$search_screen])
if test "x$search_screen" != "xyes" ; then
   AC_DEFINE(DISABLE_SEARCH_SCREEN, 1, [Disable search screen])
fi

dnl Optional screen - key editor
AC_MSG_CHECKING([whether to include the key editor screen])
AC_ARG_ENABLE([key-screen], 
              AC_HELP_STRING([--enable-key-screen],
	                     [Enable key editor screen @<:@default=yes@:>@]),
	      [keydef_screen="$enableval"],
	      [keydef_screen=yes])
AC_MSG_RESULT([$keydef_screen])
if test "x$keydef_screen" != "xyes" ; then
   AC_DEFINE(DISABLE_KEYDEF_SCREEN, 1, [Disable key editor screen])
fi

dnl Optional screen - clock
AC_MSG_CHECKING([whether to include the clock screen])
AC_ARG_ENABLE([clock-screen], 
              AC_HELP_STRING([--enable-clock-screen],
	                     [Enable clock screen @<:@default=no@:>@]),
	      [clock_screen="$enableval"],
	      [clock_screen=no])
AC_MSG_RESULT([$clock_screen])
if test "x$clock_screen" != "xyes" ; then
   AC_DEFINE(DISABLE_CLOCK_SCREEN, 1, [Disable clock screen])
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

