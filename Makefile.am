SUBDIRS = \
  po

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign 1.9 dist-bzip2

AM_CPPFLAGS = \
	$(LIBMPDCLIENT_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(LIBLIRCCLIENT_CFLAGS) \
	-DLOCALE_DIR=\""$(datadir)/locale"\"\
	-DSYSCONFDIR=\""$(sysconfdir)"\"

ncmpc_headers = \
	src/resolver.h \
	src/gidle.h \
	src/mpdclient.h \
	src/playlist.h \
	src/filelist.h \
	src/options.h \
	src/conf.h \
	src/command.h \
	src/ncu.h \
	src/player_command.h \
	src/window.h \
	src/title_bar.h \
	src/progress_bar.h \
	src/status_bar.h \
	src/screen.h \
	src/screen_message.h \
	src/screen_interface.h \
	src/screen_list.h \
	src/screen_find.h \
	src/screen_utils.h \
	src/screen_client.h \
	src/list_window.h \
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
	src/screen_play.h \
	src/screen_file.h \
	src/screen_artist.h \
	src/screen_browser.h \
	src/screen_search.h \
	src/screen_song.h \
	src/screen_keydef.h \
	src/screen_lyrics.h \
	src/screen_outputs.h \
	src/screen_text.h \
	src/plugin.h \
	src/lyrics.h \
	src/str_pool.h \
	src/lirc.h

bin_PROGRAMS = src/ncmpc

src_ncmpc_LDADD = \
	$(LIBMPDCLIENT_LIBS) \
	$(GLIB_LIBS) \
	$(LIBLIRCCLIENT_LIBS)

src_ncmpc_SOURCES = \
	$(ncmpc_headers) \
	src/resolver.c \
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
	src/screen_message.c \
	src/screen_list.c \
	src/screen_find.c \
	src/screen_utils.c \
	src/screen_client.c \
	src/screen_play.c \
	src/screen_browser.c \
	src/screen_file.c \
	src/list_window.c \
	src/colors.c \
	src/charset.c \
	src/wreadln.c \
	src/strfsong.c \
	src/utils.c \
	src/str_pool.c

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

if ENABLE_LIRC
src_ncmpc_SOURCES += src/lirc.c
endif


# build the smalles possible ncmpc binary
src/ncmpc-tiny: $(filter-out %.h,$(src_ncmpc_SOURCES))
	$(COMPILE) --combine -fwhole-program $(src_ncmpc_LDFLAGS) $(src_ncmpc_LDADD) $(LIBS) $^ -o $@
	strip --strip-all $@

CLEANFILES = src/ncmpc-tiny

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

lyrics_plugins = lyrics/01-hd.sh lyrics/02-lyricwiki.rb lyrics/03-leoslyrics.py

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

#
# Distribution
#

EXTRA_DIST = autogen.sh \
	$(lyrics_plugins) \
	$(man_MANS) $(doc_DATA)
