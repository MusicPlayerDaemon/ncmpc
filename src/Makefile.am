#SUBDIRS = 

AM_CPPFLAGS = \
  $(GLIB_CFLAGS)\
  $(GTHREAD_CFLAGS)\
  $(GMODULE_CFLAGS)\
  -DLOCALE_DIR=\""$(datadir)/locale"\"\
  -DSYSCONFDIR=\""$(sysconfdir)"\"

ncmpc_LDADD = \
  $(GLIB_LIBS)\
  $(GTHREAD_LIBS)\
  $(GMODULE_LIBS)

ncmpc_headers = \
  libmpdclient.h\
	song.h \
  mpdclient.h\
  easy_download.h\
  options.h\
  conf.h\
  command.h\
  screen.h\
  screen_utils.h\
  list_window.h\
  colors.h\
  support.h\
  wreadln.h\
  strfsong.h\
  utils.h\
  ncmpc.h\
  screen_browse.h\
  src_lyrics.h \
	str_pool.h \
	gcc.h

# $Id$

bin_PROGRAMS = ncmpc

lyr_src=${lyr_src_fixed}

ncmpc_SOURCES = \
  libmpdclient.c\
	song.c \
  main.c\
  mpdclient.c\
  options.c\
  conf.c\
  command.c\
  easy_download.c\
  screen.c\
  screen_utils.c\
  screen_play.c\
  screen_file.c\
  screen_artist.c\
  screen_search.c\
  screen_help.c\
  screen_lyrics.c\
  screen_keydef.c\
  screen_clock.c\
  list_window.c\
  colors.c\
  support.c\
  wreadln.c\
  strfsong.c\
  utils.c\
  src_lyrics.c \
	str_pool.c

if LEOSLYRICS_FIXED
ncmpc_SOURCES+=lyrics_leoslyrics.c 
AM_CPPFLAGS+=${libcurl_CFLAGS}
ncmpc_LDADD+=${libcurl_LIBS}
endif

if HD_FIXED
ncmpc_SOURCES+=lyrics_hd.c 
endif

ncmpc_SOURCES+=${ncmpc_headers}


