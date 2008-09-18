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
	screen_browser.h \
	lyrics.h \
	str_pool.h \
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
  options.c\
  conf.c\
  command.c\
  screen.c\
  screen_utils.c\
  screen_play.c\
	screen_browser.c\
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
	lyrics.c \
	str_pool.c

ncmpc_SOURCES+=${ncmpc_headers}


