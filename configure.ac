AC_PREREQ(2.60)
AC_INIT(ncmpc, 0.21~git, max@duempel.org)

VERSION_MAJOR=0
VERSION_MINOR=21
VERSION_REVISION=0
VERSION_EXTRA=0

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_AUX_DIR(build)
AM_INIT_AUTOMAKE([foreign 1.10 dist-bzip2 subdir-objects])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl Check for programs
AC_PROG_CC_C99
AC_PROG_INSTALL
AX_WITH_CURSES

AS_IF([ test "x$ax_cv_curses" = xno ],
	[AC_MSG_ERROR([No curses support detected.])
	])

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
		[Build ncmpc-mini, i.e. without all optional features @<:@default=no@:>@]),,
    [enable_mini=no])

AM_CONDITIONAL(NCMPC_MINI, test x$enable_mini = xyes)
if test "x$enable_mini" = xyes; then
    AC_DEFINE([NCMPC_MINI], [1], [Build ncmpc-mini, which disables lots of features])
    auto=no
    disable_mini=no
    auto_mini=no
else
    auto=auto
    disable_mini=yes
    auto_mini=auto
fi

AC_CANONICAL_HOST

case "$host_os" in
mingw32* | windows*)
	LIBS="$LIBS -lws2_32"

	AC_CONFIG_FILES([
		src/win/ncmpc_win32_rc.rc
	])
	AC_CHECK_TOOL(WINDRES, windres)
	HAVE_WINDOWS=1
	;;
*)
	AC_CHECK_FUNC([socket],
		[],
        	[AC_CHECK_LIB([socket],[socket],[LIBS="$LIBS -lsocket"],
		[AC_MSG_ERROR(No UNIX socket API found)])
		])

	AC_CHECK_FUNC([gethostbyname],
		[], 
		[AC_CHECK_LIB([nsl],[gethostbyname],[LIBS="$LIBS -lnsl"],
		[AC_MSG_ERROR(No UNIX gethostbyname API found)])
		])
esac
AM_CONDITIONAL([HAVE_WINDOWS], [test $HAVE_WINDOWS -eq 1])

dnl multi-byte character support

AC_ARG_ENABLE([multibyte],
	AS_HELP_STRING([--disable-multibyte],
		[Disable multibyte character support @<:@default=yes@:>@]),,
		[enable_multibyte=$disable_mini])

if test x$enable_multibyte = xyes; then
	AC_DEFINE([ENABLE_MULTIBYTE], [1], [Enable multibyte character support])
else
	if test "x$ax_cv_curses_enhanced" = xyes; then
		AC_MSG_WARN(wide characters without multibyte characters makes little sense)
	fi
fi

dnl Check for glib-2.12
PKG_CHECK_MODULES([GLIB], [glib-2.0 >= 2.12],,
	[AC_MSG_ERROR([glib 2.12 is required])])

dnl Check for libmpdclient 2.2
PKG_CHECK_MODULES([LIBMPDCLIENT], [libmpdclient >= 2.2],,
	[AC_MSG_ERROR([libmpdclient2 is required])])

dnl Check for libmpdclient 2.5 (the chat screen requires this version)
PKG_CHECK_EXISTS([libmpdclient >= 2.5],
	[have_libmpdclient_2_5=yes], [have_libmpdclient_2_5=no])

dnl i18n

AC_ARG_ENABLE([locale],
	AS_HELP_STRING([--disable-locale],
		[Disable locale support @<:@default=auto@:>@]),,
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
AC_ARG_ENABLE([colors],
	AS_HELP_STRING([--enable-colors],
		[Enable color support]),
		[enable_colors=$enableval],
		[enable_colors=auto])
AC_MSG_CHECKING([whether to include color support])
AS_IF([ test "x$enable_colors" = xyes || test "x$enable_colors" = xauto ],
	[AS_IF([ test "x$disable_mini" = xyes],
		[AS_IF([ test "x$ax_cv_curses_color" = xyes ],
			[AC_MSG_RESULT([yes])
				AC_DEFINE([ENABLE_COLORS], [1], [Enable color support])],
				[AS_IF([ test "x$enable_colors" = xyes ],
					AC_MSG_ERROR(["No color support found in curses library"])
					AC_MSG_RESULT([no]),
					AC_MSG_RESULT([no])
				)
			])
		],
		[AC_MSG_RESULT([no])
		AC_MSG_WARN(["Colors disabled due to mini mode being enabled."])]
	)],
	[AC_MSG_RESULT([no])])

dnl test for LIRC support

AC_ARG_ENABLE([lirc],
	AS_HELP_STRING([--enable-lirc],
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

dnl Optional - curses getmouse support

dnl AC_CHECK_LIB depends on being able to prepend a '-l', remove the '-l' from CURSES_LIB first
AC_CHECK_LIB([$(expr substr $CURSES_LIB 3 99)],
		[getmouse],
		[ax_cv_curses_mouse=yes],
		[ax_cv_curses_mouse=no])

AC_ARG_ENABLE([mouse], 
	AS_HELP_STRING([--enable-mouse],
		[Enable curses getmouse support @<:@default=yes@:>@]),
		[enable_mouse=$enableval],
		[enable_mouse=auto])

AC_MSG_CHECKING([whether to include mouse support])

AS_IF([ test "x$enable_mouse" = xyes || test "x$enable_mouse" = xauto ],
	[AS_IF([ test "x$disable_mini" = xyes],
		[AS_IF([ test "x$ax_cv_curses_mouse" = xyes ],
			[AC_MSG_RESULT([yes])
			AC_DEFINE([HAVE_GETMOUSE], [1], [Enable mouse support])],
			[AC_MSG_RESULT([no])
			AS_IF([ test "x$enable_mouse" = xyes ],
				[AC_MSG_ERROR(["No mouse support found in curses library"])
				])
			])
		],
		[AC_MSG_RESULT([no])
		AC_MSG_WARN("Mouse disabled due to mini mode being enabled.")]
	)],
	[AC_MSG_RESULT([no])])

dnl Optional screen - artist
AC_MSG_CHECKING([whether to include the artist screen])
AC_ARG_ENABLE([artist-screen], 
              AS_HELP_STRING([--enable-artist-screen],
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
              AS_HELP_STRING([--enable-search-screen],
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
	AS_HELP_STRING([--enable-song-screen],
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
              AS_HELP_STRING([--enable-key-screen],
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
              AS_HELP_STRING([--enable-lyrics-screen],
		[Enable lyrics screen @<:@default=no@:>@]),
	[enable_lyrics_screen=$enableval],
	[enable_lyrics_screen=no])

case "$host_os" in
mingw32* | windows*)
	AS_IF([ test "x$enable_lyrics_screen" = xyes ],
		[AC_MSG_ERROR([Lyrics screen is not yet compatible with windows.])
		])
esac

if test "x$enable_lyrics_screen" = "xyes" ; then
    AC_DEFINE(ENABLE_LYRICS_SCREEN, 1, [Enable lyrics screen])
fi

AM_CONDITIONAL(ENABLE_LYRICS_SCREEN, test x$enable_lyrics_screen = xyes)

AC_MSG_RESULT([$enable_lyrics_screen])

AC_ARG_WITH([lyrics-plugin-dir],
        AS_HELP_STRING([[--with-lyrics-plugin-dir[=DIRECTORY]]],
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
	AS_HELP_STRING([--enable-outputs-screen],
		[Enable outputs screen @<:@default=yes@:>@]),,
	[enable_outputs_screen=$disable_mini])

if test "x$enable_outputs_screen" = "xyes" ; then
	AC_DEFINE(ENABLE_OUTPUTS_SCREEN, 1, [Enable outputs screen])
fi

AM_CONDITIONAL(ENABLE_OUTPUTS_SCREEN, test x$enable_outputs_screen = xyes)
AC_MSG_RESULT([$enable_outputs_screen])


dnl Optional screen - client-to-client chat
AC_MSG_CHECKING([whether to include the chat screen])
AC_ARG_ENABLE([chat-screen],
              AC_HELP_STRING([--enable-chat-screen],
		[Enable chat screen @<:@default=no@:>@]),,
	[enable_chat_screen=no])
if test "x$enable_chat_screen" = "xauto"; then
	enable_chat_screen=$have_libmpdclient_2_5
fi
AC_MSG_RESULT([$enable_chat_screen])
if test "x$enable_chat_screen" = "xyes" ; then
	if test "x$have_libmpdclient_2_5" = "xno"; then
		AC_MSG_ERROR(The chat screen requires at least libmpclient 2.5)
	fi
	AC_DEFINE(ENABLE_CHAT_SCREEN, 1, [Enable chat screen])
fi
AM_CONDITIONAL(ENABLE_CHAT_SCREEN, test x$enable_chat_screen = xyes)


dnl
dnl Windows OS Resource File
dnl
AC_SUBST(VERSION_MAJOR)
AC_SUBST(VERSION_MINOR)
AC_SUBST(VERSION_REVISION)
AC_SUBST(VERSION_EXTRA)

dnl
dnl build options
dnl

AC_ARG_ENABLE(werror,
	AS_HELP_STRING([--enable-werror],
		[Treat warnings as errors @<:@default=no@:>@]),,
	enable_werror=no)

if test "x$enable_werror" = xyes; then
	AX_APPEND_COMPILE_FLAGS([-Werror])
fi

AC_ARG_ENABLE(debug,
	AS_HELP_STRING([--enable-debug],
		[Enable debugging @<:@default=no@:>@]),,
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
dnl autogenerated documentation
dnl

AC_ARG_ENABLE(documentation,
	AS_HELP_STRING([--enable-documentation],
		[build doxygen documentation @<:@default=no@:>@]),,
	enable_documentation=no)

if test x$enable_documentation = xyes; then
	AC_PATH_PROG(DOXYGEN, doxygen)
	if test x$DOXYGEN = x; then
		AC_MSG_ERROR([doxygen not found])
	fi
	AC_SUBST(DOXYGEN)
fi

AM_CONDITIONAL(ENABLE_DOCUMENTATION, test x$enable_documentation = xyes)

dnl
dnl CFLAGS
dnl

AC_SUBST(AM_CFLAGS)

AX_APPEND_COMPILE_FLAGS([-Wall])
AX_APPEND_COMPILE_FLAGS([-Wextra])
AX_APPEND_COMPILE_FLAGS([-Wno-deprecated-declarations])
AX_APPEND_COMPILE_FLAGS([-Wmissing-prototypes])
AX_APPEND_COMPILE_FLAGS([-Wshadow])
AX_APPEND_COMPILE_FLAGS([-Wpointer-arith])
AX_APPEND_COMPILE_FLAGS([-Wstrict-prototypes])
AX_APPEND_COMPILE_FLAGS([-Wcast-qual])
AX_APPEND_COMPILE_FLAGS([-Wwrite-strings])


dnl
dnl Generate output files
dnl

AC_CONFIG_FILES([Makefile po/Makefile.in po/Makefile])
AC_OUTPUT(doc/doxygen.conf)
