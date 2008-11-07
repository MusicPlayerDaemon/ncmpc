#SUBDIRS = 

AM_CPPFLAGS = \
  $(GLIB_CFLAGS)\
  $(GTHREAD_CFLAGS)\
  -DLOCALE_DIR=\""$(datadir)/locale"\"\
  -DSYSCONFDIR=\""$(sysconfdir)"\"

ncmpc_LDADD = \
  $(GLIB_LIBS)\
  $(GTHREAD_LIBS)

ncmpc_headers = \
  libmpdclient.h\
	resolver.h \
	song.h \
  mpdclient.h\
	playlist.h \
	filelist.h \
  options.h\
  conf.h\
  command.h\
	ncu.h \
  screen.h\
	screen_list.h \
  screen_utils.h\
  list_window.h\
  colors.h\
  support.h\
	charset.h \
  wreadln.h\
  strfsong.h\
  utils.h\
  ncmpc.h\
	defaults.h \
	i18n.h \
	screen_browser.h \
	lyrics.h \
	str_pool.h \
	lirc.h \
	gcc.h

# $Id$

bin_PROGRAMS = ncmpc

lyr_src=${lyr_src_fixed}

ncmpc_SOURCES = \
  libmpdclient.c\
	resolver.c \
	song.c \
  main.c\
  mpdclient.c\
	playlist.c \
	filelist.c \
  options.c\
  conf.c\
  command.c\
	ncu.c \
  screen.c\
	screen_list.c \
  screen_utils.c\
  screen_play.c\
	screen_browser.c\
  screen_file.c\
  screen_help.c\
  list_window.c\
  colors.c\
  support.c\
	charset.c \
  wreadln.c\
  strfsong.c\
  utils.c\
	str_pool.c

if ENABLE_ARTIST_SCREEN
ncmpc_SOURCES += screen_artist.c
endif

if ENABLE_SEARCH_SCREEN
ncmpc_SOURCES += screen_search.c
endif

if ENABLE_KEYDEF_SCREEN
ncmpc_SOURCES += screen_keydef.c
endif

if ENABLE_LYRICS_SCREEN
ncmpc_SOURCES += screen_lyrics.c lyrics.c
endif

if ENABLE_LIRC
ncmpc_SOURCES += lirc.c
endif

ncmpc_SOURCES+=${ncmpc_headers}


#
# sparse
#

SPARSE = sparse
SPARSE_FLAGS = -Wdecl -Wdefault-bitfield-sign -Wdo-while -Wenum-mismatch \
	-Wnon-pointer-null -Wptr-subtraction-blows -Wreturn-void \
	-Wshadow -Wtypesign \
	-D__transparent_union__=unused

sparse-check:
	for i in $(ncmpc_SOURCES); \
	do \
		$(SPARSE) -I. -I.. -I/usr/lib/gcc/$(shell gcc -dumpmachine)/4.3/include -I/usr/lib/gcc/$(shell gcc -dumpmachine)/4.3/include-fixed $(AM_CPPFLAGS) $(SPARSE_FLAGS) $(srcdir)/$$i || exit; \
	done
