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

bin_PROGRAMS = src/ncmpc

src_ncmpc_LDADD = \
	$(CURSES_LIB) \
	$(LIBMPDCLIENT_LIBS) \
	$(GLIB_LIBS) \
	$(LIBLIRCCLIENT_LIBS)

src_ncmpc_SOURCES = \
	src/Compiler.h \
	src/ncmpc.h \
	src/defaults.h \
	src/ncmpc_curses.h \
	src/i18n.h \
	src/main.c \
	src/gidle.c src/gidle.h \
	src/mpdclient.c src/mpdclient.h \
	src/playlist.c src/playlist.h \
	src/filelist.c src/filelist.h \
	src/options.c src/options.h \
	src/command.c src/command.h \
	src/ncfix.h \
	src/ncu.c src/ncu.h \
	src/player_command.c src/player_command.h \
	src/title_bar.c src/title_bar.h \
	src/progress_bar.c src/progress_bar.h \
	src/status_bar.c src/status_bar.h \
	src/screen.c src/screen.h \
	src/screen_interface.h \
	src/screen_utils.c src/screen_utils.h \
	src/screen_status.c src/screen_status.h \
	src/screen_list.c src/screen_list.h \
	src/screen_find.c src/screen_find.h \
	src/screen_client.c src/screen_client.h \
	src/screen_queue.c src/screen_queue.h \
	src/screen_browser.c src/screen_browser.h \
	src/screen_file.c src/screen_file.h \
	src/list_window.c src/list_window.h \
	src/paint.h \
	src/song_paint.c src/song_paint.h \
	src/colors.c src/colors.h \
	src/charset.c src/charset.h \
	src/window.h \
	src/wreadln.c src/wreadln.h \
	src/strfsong.c src/strfsong.h \
	src/utils.c src/utils.h

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
	src/hscroll.c src/hscroll.h \
	src/match.c src/match.h \
	src/conf.c src/conf.h
endif

if ENABLE_HELP_SCREEN
src_ncmpc_SOURCES += src/screen_help.c src/screen_help.h
endif

if ENABLE_ARTIST_SCREEN
src_ncmpc_SOURCES += src/screen_artist.c src/screen_artist.h
endif

if ENABLE_SEARCH_SCREEN
src_ncmpc_SOURCES += src/screen_search.c src/screen_search.h
endif

if ENABLE_SONG_SCREEN
src_ncmpc_SOURCES += src/screen_song.c src/screen_song.h
endif

if ENABLE_KEYDEF_SCREEN
src_ncmpc_SOURCES += src/screen_keydef.c src/screen_keydef.h
endif

if ENABLE_PLUGIN_LIBRARY
src_ncmpc_SOURCES += src/plugin.c src/plugin.h
endif

if ENABLE_SCREEN_TEXT
src_ncmpc_SOURCES += src/screen_text.c src/screen_text.h
endif

if ENABLE_LYRICS_SCREEN
src_ncmpc_SOURCES += \
	src/screen_lyrics.c src/screen_lyrics.h \
	src/lyrics.c src/lyrics.h
endif

if ENABLE_OUTPUTS_SCREEN
src_ncmpc_SOURCES += src/screen_outputs.c src/screen_outputs.h
endif

if ENABLE_CHAT_SCREEN
src_ncmpc_SOURCES += src/screen_chat.c src/screen_chat.h
endif

if ENABLE_LIRC
src_ncmpc_SOURCES += src/lirc.c src/lirc.h
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
