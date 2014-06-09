SUBDIRS = \
  po

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign 1.11 dist-xz subdir-objects

AM_CPPFLAGS = \
	$(LIBMPDCLIENT_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(LIBLIRCCLIENT_CFLAGS) \
	-DLOCALE_DIR=\""$(datadir)/locale"\"\
	-DSYSCONFDIR=\""$(sysconfdir)"\"

ncmpc_headers = \
	src/gidle.h \
	src/mpdclient.h \
	src/playlist.h \
	src/filelist.h \
	src/options.h \
	src/conf.h \
	src/command.h \
	src/ncu.h \
	src/ncfix.h \
	src/player_command.h \
	src/window.h \
	src/title_bar.h \
	src/progress_bar.h \
	src/status_bar.h \
	src/screen.h \
	src/screen_status.h \
	src/screen_interface.h \
	src/screen_list.h \
	src/screen_find.h \
	src/screen_utils.h \
	src/screen_client.h \
	src/list_window.h \
	src/song_paint.h \
	src/colors.h \
	src/paint.h \
	src/hscroll.h \
	src/charset.h \
	src/match.h \
	src/wreadln.h \
	src/strfsong.h \
	src/utils.h \
	src/ncmpc.h \
	src/defaults.h \
	src/i18n.h \
	src/screen_help.h \
	src/screen_queue.h \
	src/screen_file.h \
	src/screen_artist.h \
	src/screen_browser.h \
	src/screen_search.h \
	src/screen_song.h \
	src/screen_keydef.h \
	src/screen_lyrics.h \
	src/screen_outputs.h \
	src/screen_chat.h \
	src/screen_text.h \
	src/plugin.h \
	src/lyrics.h \
	src/lirc.h \
	src/ncmpc_curses.h

bin_PROGRAMS = src/ncmpc

src_ncmpc_LDADD = \
	$(CURSES_LIB) \
	$(LIBMPDCLIENT_LIBS) \
	$(GLIB_LIBS) \
	$(LIBLIRCCLIENT_LIBS)

src_ncmpc_SOURCES = \
	src/Compiler.h \
	$(ncmpc_headers) \
	src/main.c \
	src/gidle.c \
	src/mpdclient.c \
	src/playlist.c \
	src/filelist.c \
	src/options.c \
	src/command.c \
	src/ncu.c \
	src/player_command.c \
	src/title_bar.c \
	src/progress_bar.c \
	src/status_bar.c \
	src/screen.c \
	src/screen_status.c \
	src/screen_list.c \
	src/screen_find.c \
	src/screen_utils.c \
	src/screen_client.c \
	src/screen_queue.c \
	src/screen_browser.c \
	src/screen_file.c \
	src/list_window.c \
	src/song_paint.c \
	src/colors.c \
	src/charset.c \
	src/wreadln.c \
	src/strfsong.c \
	src/utils.c

#
# Windows resource file
#

src/win/ncmpc_win32_rc.$(OBJEXT): src/win/ncmpc_win32_rc.rc
	$(WINDRES) -i $< -o $@

if HAVE_WINDOWS
noinst_DATA = src/win/ncmpc_win32_rc.rc

src_ncmpc_DEPENDENCIES = src/win/ncmpc_win32_rc.$(OBJEXT)
src_ncmpc_LDFLAGS = -Wl,src/win/ncmpc_win32_rc.$(OBJEXT)
endif

if NCMPC_MINI
else
src_ncmpc_SOURCES += \
	src/hscroll.c \
	src/match.c \
	src/conf.c
endif

if ENABLE_HELP_SCREEN
src_ncmpc_SOURCES += src/screen_help.c
endif

if ENABLE_ARTIST_SCREEN
src_ncmpc_SOURCES += src/screen_artist.c
endif

if ENABLE_SEARCH_SCREEN
src_ncmpc_SOURCES += src/screen_search.c
endif

if ENABLE_SONG_SCREEN
src_ncmpc_SOURCES += src/screen_song.c
endif

if ENABLE_KEYDEF_SCREEN
src_ncmpc_SOURCES += src/screen_keydef.c
endif

if ENABLE_PLUGIN_LIBRARY
src_ncmpc_SOURCES += src/plugin.c
endif

if ENABLE_SCREEN_TEXT
src_ncmpc_SOURCES += src/screen_text.c
endif

if ENABLE_LYRICS_SCREEN
src_ncmpc_SOURCES += src/screen_lyrics.c src/lyrics.c
endif

if ENABLE_OUTPUTS_SCREEN
src_ncmpc_SOURCES += src/screen_outputs.c
endif

if ENABLE_CHAT_SCREEN
src_ncmpc_SOURCES += src/screen_chat.c
endif

if ENABLE_LIRC
src_ncmpc_SOURCES += src/lirc.c
endif


# build the smallest possible ncmpc binary
src/ncmpc-tiny: $(filter-out %.h,$(src_ncmpc_SOURCES))
	$(COMPILE) --combine -fwhole-program $(src_ncmpc_LDFLAGS) $(src_ncmpc_LDADD) $(LIBS) $^ -o $@
	strip --strip-all $@

CLEANFILES = src/ncmpc-tiny

#
# test suite
#

check_PROGRAMS = \
	test/run_hscroll

test_run_hscroll_SOURCES = test/run_hscroll.c src/hscroll.c src/charset.c
test_run_hscroll_CPPFLAGS = -I$(srcdir)/src $(AM_CPPFLAGS)
test_run_hscroll_LDADD = @CURSES_LIB@ $(GLIB_LIBS)

#
# sparse
#

SPARSE = sparse
SPARSE_FLAGS = -Wdecl -Wdefault-bitfield-sign -Wdo-while -Wenum-mismatch \
	-Wnon-pointer-null -Wptr-subtraction-blows -Wreturn-void \
	-Wshadow -Wtypesign \
	-D__transparent_union__=unused

sparse-check:
	$(SPARSE) -I. -I/usr/lib/gcc/$(shell gcc -dumpmachine)/4.3/include -I/usr/lib/gcc/$(shell gcc -dumpmachine)/4.3/include-fixed $(AM_CPPFLAGS) $(SPARSE_FLAGS) $(src_ncmpc_SOURCES)

#
# lyrics plugins
#

lyrics_plugins = lyrics/10-hd.sh lyrics/20-lyricwiki.rb

if ENABLE_LYRICS_SCREEN

lyricsdir = $(lyrics_plugin_dir)
lyrics_SCRIPTS = $(lyrics_plugins)

endif

#
# Documentation
#

doc_DATA = \
	doc/config.sample doc/keys.sample doc/ncmpc.lirc \
	AUTHORS NEWS README
man_MANS = doc/ncmpc.1

if ENABLE_DOCUMENTATION
doc/api/html/index.html: doc/doxygen.conf
	@mkdir -p $(@D)
	$(DOXYGEN) $<

all-local: doc/api/html/index.html

clean-local:
	rm -rf doc/api

# copied from MPD, disabled  --jn
#---------------------------------------
#install-data-local: doc/api/html/index.html
#	$(mkinstalldirs) $(DESTDIR)$(docdir)/api/html
#	$(INSTALL_DATA) -c -m 644 doc/api/html/*.* \
#		$(DESTDIR)$(docdir)/api/html
#
#uninstall-local:
#	rm -f $(DESTDIR)$(docdir)/api/html/*.*
#---------------------------------------
endif

#
# Distribution
#

EXTRA_DIST = autogen.sh \
	$(lyrics_plugins) \
	$(man_MANS) $(doc_DATA) doc/doxygen.conf \
	src/win/ncmpc_win32_rc.rc.in
