dnl
dnl  $Id$
dnl

AC_INIT
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE([ncmpc], [0.11.2-svn])
AM_CONFIG_HEADER([config.h])

dnl Check for programs
AC_PROG_CC
AC_PROG_INSTALL
dnl AC_PROG_LIBTOOL

dnl =======================================================
dnl initialize variables
dnl =======================================================

dnl i18n
ALL_LINGUAS="es da de fr gl no ru sv"

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

dnl Try to find the socket functions
AC_CHECK_FUNC([socket],
	      [], 
              [AC_CHECK_LIB([socket],[socket],[LIBS="$LIBS -lsocket"],
	                    [AC_MSG_ERROR(No UNIX socket API found)])
              ])

dnl Try to find the gethostbyname functions
AC_CHECK_FUNC([gethostbyname],
	      [], 
              [AC_CHECK_LIB([nsl],[gethostbyname],[LIBS="$LIBS -lnsl"],
	                    [AC_MSG_ERROR(No UNIX gethostbyname API found)])
              ])


dnl ncurses
ncurses=auto
AC_ARG_WITH([ncurses],
	    AC_HELP_STRING([--with-ncurses], [compile/link with ncurses library] ),
	    [ncurses=ncurses])
AC_ARG_WITH(ncursesw,
	    AC_HELP_STRING([--with-ncursesw], [compile/link with wide-char ncurses library @<:@default@:>@]),
	    [ncurses=ncursesw])

if test "x$ncurses" = "xauto"; then
	AC_CHECK_LIB([ncursesw], 
		     [initscr],
		     [ncurses=ncursesw],
		     [ncurses=ncurses])
fi
		     

AC_CHECK_LIB([$ncurses], 
	     [initscr],
	     [LIBS="$LIBS -l$ncurses"], 
	     [AC_MSG_ERROR($ncurses library is required)])


if test "x$ncurses" = "xncursesw" ; then
   AC_DEFINE(_XOPEN_SOURCE_EXTENDED, 1, [wide-char wget_ch()])
   AC_DEFINE(USE_NCURSESW, 1, [use wide-char ncurses library])
fi


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

dnl check for gthreads
PKG_CHECK_MODULES([GTHREAD], 
		  [gthread-2.0],
		  [gthread=yes],
		  [AC_MSG_WARN([gthread-2.0 >= 0.20 is required the lyrics screen])])
AC_SUBST(GTHREAD_LIBS)
AC_SUBST(GTHREAD_CFLAGS)
		  
dnl check for libcurl
PKG_CHECK_MODULES([libcurl], 
		  [libcurl],
		  [libcurl=yes],
		  #[libcurl=yes,LIBS="$LIBS -lcurl"], #doesn't work for me on arch
		  [AC_MSG_WARN([libcurl is required for the lyrics screen])])

if test "x$libcurl" = "xyes"; then
 LIBS="$LIBS -lcurl" 
fi
		  
dnl check for expat
expat=expat
AC_CHECK_LIB([$expat], 
	     [XML_ParserCreate],
	     [have_expat=yes], 
	     [AC_MSG_WARN($expat library is required for lyrics screen)])
if test "x$have_expat" = "xyes"; then
LIBS="$LIBS -l$expat"
fi
		  
#if test "x$gthread-2.0" != "xyes"; then
#  PKG_CHECK_MODULES([GTHREAD], 
#		     [glib-2.0 >= 2.2],
#		     [nls=no],
#		     [AC_MSG_ERROR([glib-2.2 is required])])
#fi

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
   AC_CHECK_LIB([$ncurses], 
	        [getmouse],
		[AC_DEFINE([HAVE_GETMOUSE], [1], [$ncurses - getmouse()])],
		[])
fi

dnl Optional screen - artist
AC_MSG_CHECKING([whether to include the artist screen])
AC_ARG_ENABLE([artist-screen], 
              AC_HELP_STRING([--enable-artist-screen],
	                     [Enable artist screen (EXPERIMENTAL) @<:@default=no@:>@]),
	      [artist_screen="$enableval"],
	      [artist_screen=no])
AC_MSG_RESULT([$artist_screen])
if test "x$artist_screen" != "xyes" ; then
   AC_DEFINE(DISABLE_ARTIST_SCREEN, 1, [Disable artist screen])
fi

dnl Optional screen - search
AC_MSG_CHECKING([whether to include the search screen])
AC_ARG_ENABLE([search-screen], 
              AC_HELP_STRING([--enable-search-screen],
	                     [Enable search screen (EXPERIMENTAL) @<:@default=yes@:>@]),
	      [search_screen="$enableval"],
	      [search_screen=yes])
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
	                     [Enable clock screen @<:@default=yes@:>@]),
	      [clock_screen="$enableval"],
	      [clock_screen=yes])
AC_MSG_RESULT([$clock_screen])
if test "x$clock_screen" != "xyes" ; then
   AC_DEFINE(DISABLE_CLOCK_SCREEN, 1, [Disable clock screen])
fi

dnl Optional screen - lyrics
AC_MSG_CHECKING([whether to include the lyrics screen]) 
AC_ARG_ENABLE([lyrics-screen], 
              AC_HELP_STRING([--enable-lyrics-screen],
	                     [Enable lyrics screen @<:@default=no@:>@]),
	      [lyrics_screen="$enableval"],
	      [lyrics_screen=no])	
if test "x$lyrics_screen" != "xyes" ; then
   AC_DEFINE(DISABLE_LYRICS_SCREEN, 1, [Disable lyrics screen])
   lyrics_screen=no
fi
#lyrics_screen=yes
if test "$libcurl" != "yes" ; then
 AC_MSG_RESULT([$lyrics_screen])  
 # AC_DEFINE(DISABLE_LYRICS_SCREEN, 1, [Disable lyrics screen])
  # lyrics_screen=no
fi
if test "$gthread" != "yes" ; then
   AC_DEFINE(DISABLE_LYRICS_SCREEN, 1, [Disable lyrics screen])
   lyrics_screen=no
AC_MSG_RESULT([$lyrics_screen]) 
fi
if test "$have_expat" != "yes" ; then
   AC_DEFINE(DISABLE_LYRICS_SCREEN, 1, [Disable lyrics screen])
   lyrics_screen=no
fi

AC_MSG_RESULT([$lyrics_screen])

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

dnl Default timedisplay type
AC_MSG_CHECKING([for default timedisplay type])
AC_ARG_WITH([default-timedisplay_type],
	    AC_HELP_STRING([--with-default-timedisplay_type=ARG],
	                   [default_timedisplay]),
            [DEFAULT_TIMEDISPLAY_TYPE="$withval"],
            [DEFAULT_TIMEDISPLAY_TYPE="elapsed"])
AC_MSG_RESULT([$DEFAULT_TIMEDISPLAY_TYPE])
AC_DEFINE_UNQUOTED([DEFAULT_TIMEDISPLAY_TYPE], ["$DEFAULT_TIMEDISPLAY_TYPE"], [Default way to display time, either 'elapsed' or 'remaining'])

AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile po/Makefile.in])
AC_OUTPUT
