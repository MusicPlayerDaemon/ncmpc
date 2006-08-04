AM_CPPFLAGS = \
  $(GLIB_CFLAGS)\
  $(GTHREAD_CFLAGS)\
  $(libcurl_CFLAGS)\
  -DLOCALE_DIR=\""$(datadir)/locale"\"\
  -DSYSCONFDIR=\""$(sysconfdir)"\"

ncmpc_headers = \
  libmpdclient.h\
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
  screen_browse.h

# $Id$

bin_PROGRAMS = ncmpc

ncmpc_SOURCES = \
  libmpdclient.c\
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
  $(ncmpc_headers)

ncmpc_LDADD = \
  $(GLIB_LIBS)\
  $(GTHREAD_LIBS)\
  $(libcurl_LIBS)
