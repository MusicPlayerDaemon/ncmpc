dnl
dnl  $Id: configure.ac,v 1.10 2004/03/18 09:33:43 kalle Exp $
dnl

AC_INIT(main.c)
AM_INIT_AUTOMAKE(ncmpc, 0.10.0-svn)

dnl Check for programs
AC_PROG_CC
AC_PROG_INSTALL
dnl AC_PROG_LIBTOOL

dnl =======================================================
dnl initialize variables
dnl =======================================================

set -- $CFLAGS
CFLAGS="-Wall $CFLAGS"

dnl
dnl Check for types
dnl
AC_CHECK_TYPE(socklen_t, 
	      AC_DEFINE(HAVE_SOCKLEN_T, 1, socklen_t defined in sys/socket.h),
)


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

dnl iconv
AM_ICONV

dnl nl_langinfo()
AM_LANGINFO_CODESET

dnl ncurses
AC_CHECK_LIB(ncurses, initscr,, [AC_MSG_ERROR(ncurses library is required)])

dnl glib-2.0
AM_PATH_GLIB_2_0(, , [AC_MSG_ERROR(glib-2.x is required)], glib)

dnl popt
AC_CHECK_LIB(popt, 
             poptGetArg,
             LIBS="$LIBS -lpopt",
             AC_MSG_ERROR(Missing popt command line parsing library))




dnl Debugging 
AC_ARG_ENABLE(debug, 
              [  --enable-debug        Enable debugging [default=no]],
	      , 
	      enable_debug=no)

if test "$enable_debug" = yes; then
  CFLAGS="$CFLAGS -g -DDEBUG"
fi

dnl Enable 
AC_ARG_ENABLE(colors,
	    [  --enable-colors         Enable colors [default=no]],
	    ,
            enable_colors=no)

if test "$enable_colors" = yes; then
  CFLAGS="$CFLAGS -DENABLE_COLORS"
fi


dnl Default charset
AC_ARG_WITH(default-charset,
	    [  --with-default-charset=ARG  Default charset (ISO-8859-1)], 
            DEFAULT_CHARSET="ISO-8859-1")

dnl Default host
AC_ARG_WITH(default-host,
	    [  --with-default-host=HOST  Default host (localhost)], 
            DEFAULT_HOST="$withval",
            DEFAULT_HOST="localhost")

dnl Default port
AC_ARG_WITH(default-port,
            [  --with-default-port=PORT  Default port (2100)],
            DEFAULT_PORT="$withval",
            DEFAULT_PORT="2100")


CFLAGS="$CFLAGS $GLIB_CFLAGS"
LIBS="$LIBS $GLIB_LIBS -lncurses"


dnl Autoheader
AC_DEFINE_UNQUOTED(DEFAULT_PORT,      $DEFAULT_PORT,     Default MPD port)
AC_DEFINE_UNQUOTED(DEFAULT_PORT_STR, "$DEFAULT_PORT",    Default MPD port)
AC_DEFINE_UNQUOTED(DEFAULT_HOST,     "$DEFAULT_HOST",    Default MPD host)
AC_DEFINE_UNQUOTED(DEFAULT_CHARSET,  "$DEFAULT_CHARSET", Default charset)


AM_CONFIG_HEADER(config.h)

AC_OUTPUT(Makefile)

echo "
Configuration:  
  Install path:                       ${prefix}
  Enable debugging:                   ${enable_debug}
  Default MPD host:                   ${DEFAULT_HOST}
  Default MPD port:                   ${DEFAULT_PORT}
"
echo
