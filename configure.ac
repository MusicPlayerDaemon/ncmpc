AC_PREREQ(2.60)
AC_INIT(ncmpc, 0.20~git, max@duempel.org)
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_AUX_DIR(build)
AM_INIT_AUTOMAKE([foreign 1.10 dist-bzip2 subdir-objects])
AM_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl Check for programs
AC_PROG_CC_C99
AC_PROG_INSTALL

dnl =======================================================
dnl initialize variables
dnl =======================================================

dnl i18n
ALL_LINGUAS=`grep -v '^\#' po/LINGUAS`

set -- $CFLAGS


dnl
dnl ncmpc-mini
dnl

AC_ARG_ENABLE(mini,
    AS_HELP_STRING([--enable-mini],
		[Build ncmpc-mini, i.e. without all optional features (default: disabled)]),,
    [enable_mini=no])

AM_CONDITIONAL(NCMPC_MINI, test x$enable_mini = xyes)
if test "x$enable_mini" = xyes; then
    AC_DEFINE([NCMPC_MINI], [1], [Build ncmpc-mini, which disables lots of features])
    auto=no
    disable_mini=no
    auto_mini=no
    ncurses=ncurses
else
    auto=auto
    disable_mini=yes
    auto_mini=auto
    ncurses=auto
fi


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


dnl wide character support
AC_ARG_ENABLE([wide],
    AC_HELP_STRING([--enable-wide],
		[Enable wide character support]),,
                   [enable_wide=auto])


dnl ncurses
AC_ARG_WITH([ncurses],
	    AC_HELP_STRING([--with-ncurses], [compile/link with ncurses library] ),
	    [ncurses=ncurses])
AC_ARG_WITH(ncursesw,
	    AC_HELP_STRING([--with-ncursesw], [compile/link with wide-char ncurses library @<:@default@:>@]),
	    [ncurses=ncursesw])

if test "x$enable_wide" = "xno"; then
    if test "x$ncurses" = "xauto"; then
        dnl select narrow ncurses
        ncurses=ncurses
    fi

    if test "x$ncurses" = "xncursesw"; then
        AC_MSG_WARN([Narrow ncmpc linking against wide libncurses])
    fi
fi

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

if test "x$ncurses" = "xncursesw"; then
    AC_CHECK_HEADER([ncursesw/ncurses.h],
                    AC_DEFINE([HAVE_NCURSESW_NCURSES_H], [1], [ncursesw/ncurses.h]),
                    ,)
fi


dnl wide character support II

if test "x$enable_wide" = "xauto" ; then
    AC_CHECK_LIB([$ncurses], 
	         [wget_wch],
	         [enable_wide=yes],
	         [enable_wide=no])
fi

AC_MSG_CHECKING([wide character support])
AC_MSG_RESULT([$enable_wide])

if test "x$enable_wide" = "xyes" ; then
    AC_DEFINE([ENABLE_WIDE], [1], [Enable wide character support])
fi

dnl multi-byte character support

AC_ARG_ENABLE([multibyte],
	AC_HELP_STRING([--disable-multibyte],
		[Disable multibyte character support (default: enabled)]),,
		[enable_multibyte=$disable_mini])

if test x$enable_multibyte = xyes; then
	AC_DEFINE([ENABLE_MULTIBYTE], [1], [Enable multibyte character support])
else
	if test x$enable_wide = xyes; then
		AC_MSG_WARN(wide characters without multibyte characters makes little sense)
	fi
fi

dnl Check for glib-2.4
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.12],,
	[AC_MSG_ERROR([glib 2.12 is required])])

dnl Check for libmpdclient 2.2
PKG_CHECK_MODULES([LIBMPDCLIENT], [libmpdclient >= 2.2],,
	[AC_MSG_ERROR([libmpdclient2 is required])])


dnl i18n

AC_ARG_ENABLE([locale],
	AC_HELP_STRING([--disable-locale],
		[Disable locale support (default: auto)]),,
		[enable_locale=$auto_mini])

if test x$enable_locale = xyes; then
	AC_CHECK_HEADER([locale.h],,
		[AC_MSG_ERROR(locale.h is unavailable)])
fi

if test x$enable_locale = xauto; then
	AC_CHECK_HEADER([locale.h],
		[enable_locale=yes],
		[enable_locale=no])
fi

if test x$enable_locale = xyes; then
	AC_DEFINE([ENABLE_LOCALE], [1], [Locale support is enabled])
fi

if test x$enable_mini != xyes; then
	AM_NLS
else
	USE_NLS=no
fi

if test x$USE_NLS = xyes; then
   AM_GLIB_GNU_GETTEXT
   GETTEXT_PACKAGE=$PACKAGE
   AC_SUBST(GETTEXT_PACKAGE)
   AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], 
                      ["${GETTEXT_PACKAGE}"], 
                      [gettext domain])
	USE_NLS=$gt_cv_have_gettext
	if test x$USE_NLS != xyes; then
     AC_MSG_WARN([NLS support disabled!])
  fi
else
  AM_PO_SUBDIRS
fi

dnl enable colors
AC_MSG_CHECKING([color support])
AC_ARG_ENABLE([colors],
	AC_HELP_STRING([--enable-colors],
		[Enable color support]),,
	[enable_colors=$disable_mini])
AC_MSG_RESULT([$enable_colors])
if test "x$enable_colors" = "xyes" ; then
    AC_DEFINE([ENABLE_COLORS], [1], [Enable color support])
fi

dnl test for LIRC support

AC_ARG_ENABLE([lirc],
	AC_HELP_STRING([--enable-lirc],
		[Enable LIRC support]),,
	[enable_lirc=no])
if test x$enable_lirc = xyes; then
	PKG_CHECK_MODULES([LIBLIRCCLIENT], [liblircclient0],,
		[AC_MSG_ERROR([liblircclient0 not found])])
	AC_DEFINE([ENABLE_LIRC], [1], [Enable LIRC support])
fi

AM_CONDITIONAL(ENABLE_LIRC, test x$enable_lirc = xyes)


dnl Optional screen - help screen
AC_MSG_CHECKING([whether to include the help screen])
AC_ARG_ENABLE([help-screen],
	AS_HELP_STRING([--enable-help-screen],
		[Enable the help screen @<:@default=yes@:>@]),,
	[enable_help_screen=$disable_mini])
AC_MSG_RESULT([$enable_help_screen])
if test "x$enable_help_screen" = "xyes" ; then
	AC_DEFINE(ENABLE_HELP_SCREEN, 1, [Enable the help screen])
fi

AM_CONDITIONAL(ENABLE_HELP_SCREEN, test x$enable_help_screen = xyes)

dnl Optional screen - browse screen
#AC_MSG_CHECKING([whether to include the browse screen])r
#AC_ARG_ENABLE([browse-screen], 
#              AC_HELP_STRING([--enable-browse-screen],
#		[Enable the browse screen @<:@default=yes@:>@]),,
#	[enable_browse_screen=yes])
#AC_MSG_RESULT([$enable_browse_screen])
#if test "x$enable_browse_screen" = "xyes" ; then
#   AC_DEFINE(ENABLE_BROWSE_SCREEN, 1, [Enable the browse screen])
#fi

dnl Optional - curses getmouse support
AC_MSG_CHECKING([whether to include mouse support])
AC_ARG_ENABLE([mouse], 
              AC_HELP_STRING([--enable-mouse],
		[Enable curses getmouse support @<:@default=yes@:>@]),,
	[enable_mouse=$disable_mini])
AC_MSG_RESULT([$enable_mouse])
if test "x$enable_mouse" = "xyes" ; then
   AC_CHECK_LIB([$ncurses], 
	        [getmouse],
		[AC_DEFINE([HAVE_GETMOUSE], [1], [$ncurses - getmouse()])],
		[])
fi

dnl Optional screen - artist
AC_MSG_CHECKING([whether to include the artist screen])
AC_ARG_ENABLE([artist-screen], 
              AC_HELP_STRING([--enable-artist-screen],
		[Enable artist screen @<:@default=yes@:>@]),,
	[enable_artist_screen=$disable_mini])
AC_MSG_RESULT([$enable_artist_screen])
if test "x$enable_artist_screen" = "xyes" ; then
    AC_DEFINE(ENABLE_ARTIST_SCREEN, 1, [Enable artist screen])
fi

AM_CONDITIONAL(ENABLE_ARTIST_SCREEN, test x$enable_artist_screen = xyes)

dnl Optional screen - search
AC_MSG_CHECKING([whether to include the search screen])
AC_ARG_ENABLE([search-screen], 
              AC_HELP_STRING([--enable-search-screen],
		[Enable search screen (EXPERIMENTAL) @<:@default=yes@:>@]),,
	[enable_search_screen=$disable_mini])
AC_MSG_RESULT([$enable_search_screen])
if test "x$enable_search_screen" = "xyes" ; then
    AC_DEFINE(ENABLE_SEARCH_SCREEN, 1, [Enable search screen])
fi

AM_CONDITIONAL(ENABLE_SEARCH_SCREEN, test x$enable_search_screen = xyes)

dnl Optional screen - song viewer
AC_MSG_CHECKING([whether to include the song viewer screen])
AC_ARG_ENABLE([song-screen],
	AC_HELP_STRING([--enable-song-screen],
		[Enable song viewer screen @<:@default=yes@:>@]),,
	[enable_song_screen=$disable_mini])
AC_MSG_RESULT([$enable_song_screen])
if test "x$enable_song_screen" = "xyes" ; then
	AC_DEFINE(ENABLE_SONG_SCREEN, 1, [Enable song viewer screen])
fi

AM_CONDITIONAL(ENABLE_SONG_SCREEN, test x$enable_song_screen = xyes)
  
dnl Optional screen - key editor
AC_MSG_CHECKING([whether to include the key editor screen])
AC_ARG_ENABLE([key-screen], 
              AC_HELP_STRING([--enable-key-screen],
		[Enable key editor screen @<:@default=yes@:>@]),,
	[enable_key_screen=$disable_mini])
AC_MSG_RESULT([$enable_key_screen])
if test "x$enable_key_screen" = "xyes" ; then
    AC_DEFINE(ENABLE_KEYDEF_SCREEN, 1, [Enable key editor screen])
fi

AM_CONDITIONAL(ENABLE_KEYDEF_SCREEN, test x$enable_key_screen = xyes)

dnl Optional screen - lyrics
AC_MSG_CHECKING([whether to include the lyrics screen]) 
AC_ARG_ENABLE([lyrics-screen], 
              AC_HELP_STRING([--enable-lyrics-screen],
		[Enable lyrics screen @<:@default=no@:>@]),,
	[enable_lyrics_screen=no])
if test "x$enable_lyrics_screen" = "xyes" ; then
    AC_DEFINE(ENABLE_LYRICS_SCREEN, 1, [Enable lyrics screen])
fi

AM_CONDITIONAL(ENABLE_LYRICS_SCREEN, test x$enable_lyrics_screen = xyes)

AC_MSG_RESULT([$enable_lyrics_screen])

AC_ARG_WITH([lyrics-plugin-dir],
        AC_HELP_STRING([[--with-lyrics-plugin-dir[=DIRECTORY]]],
                       [Directory where lyrics plugins are stored @<:@default=PREFIX/lib/ncmpc/lyrics@:>@]),
       [lyrics_plugin_dir=$withval],
       [lyrics_plugin_dir=""])
if test "x${lyrics_plugin_dir}" = "x"; then
    if test "x${prefix}" = "xNONE"; then
        lyrics_plugin_dir="${ac_default_prefix}/lib/ncmpc/lyrics"
    else
        lyrics_plugin_dir="${prefix}/lib/ncmpc/lyrics"
    fi
fi
AC_DEFINE_UNQUOTED([LYRICS_PLUGIN_DIR], ["$lyrics_plugin_dir"],
                  [Directory to search for lyrics plugins])
AC_SUBST(lyrics_plugin_dir)

AM_CONDITIONAL(ENABLE_PLUGIN_LIBRARY, test x$enable_lyrics_screen = xyes)
AM_CONDITIONAL(ENABLE_SCREEN_TEXT, test x$enable_lyrics_screen = xyes)

dnl Optional screen - outputs
AC_MSG_CHECKING([whether to include the outputs screen])
AC_ARG_ENABLE([outputs-screen],
	AC_HELP_STRING([--enable-outputs-screen],
		[Enable outputs screen @<:@default=yes@:>@]),,
	[enable_outputs_screen=$disable_mini])
if test "x$enable_outputs_screen" = "xyes" ; then
	AC_DEFINE(ENABLE_OUTPUTS_SCREEN, 1, [Enable outputs screen])
fi

AM_CONDITIONAL(ENABLE_OUTPUTS_SCREEN, test x$enable_outputs_screen = xyes)


dnl
dnl build options
dnl

AC_ARG_ENABLE(werror,
	AS_HELP_STRING([--enable-werror],
		[Treat warnings as errors (default: disabled)]),,
	enable_werror=no)

if test "x$enable_werror" = xyes; then
	AM_CFLAGS="$AM_CFLAGS -Werror"
fi

AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug],
		[Enable debugging (default: disabled)]),,
	enable_debug=no)

if test "x$enable_debug" = xno; then
	AM_CFLAGS="$AM_CFLAGS -DNDEBUG"
fi

AC_ARG_ENABLE(test,
	AS_HELP_STRING([--enable-test],
		[build the test programs @<:@default=no@:>@]),,
	enable_test=$enable_debug)

AM_CONDITIONAL(ENABLE_TEST, test "x$enable_test" = xyes)


dnl
dnl CFLAGS
dnl

AC_SUBST(AM_CFLAGS)

CHECK_CFLAG([-Wall])
CHECK_CFLAG([-Wextra])
CHECK_CFLAG([-Wno-deprecated-declarations])
CHECK_CFLAG([-Wmissing-prototypes])
CHECK_CFLAG([-Wshadow])
CHECK_CFLAG([-Wpointer-arith])
CHECK_CFLAG([-Wstrict-prototypes])
CHECK_CFLAG([-Wcast-qual])
CHECK_CFLAG([-Wwrite-strings])


dnl
dnl Generate output files
dnl

AC_CONFIG_FILES([Makefile po/Makefile.in po/Makefile])
AC_OUTPUT
