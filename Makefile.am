#
# $Id: Makefile.am,v 1.5 2004/03/16 23:49:14 kalle Exp $
#

bin_PROGRAMS = ncmpc
pkgdata_DATA =
docdir = $(prefix)/share/doc/$(PACKAGE)
doc_DATA = AUTHORS README ChangeLog
EXTRA_DIST = COPYING $(pkgdata_DATA) $(doc_DATA)

SUBDIRS = doc

EXTRA_ncmpc_SOURCES = langinfo.c

ncmpc_LDADD = $(LANGINFO_OBJS)
ncmpc_DEPENDENCIES = $(LANGINFO_OBJS)

ncmpc_headers = libmpdclient.h mpc.h options.h conf.h command.h screen.h \
              screen_utils.h screen_play.h screen_file.h screen_search.h \
	      screen_help.h list_window.h support.h 

ncmpc_SOURCES = libmpdclient.c main.c mpc.c options.c conf.c command.c \
		      screen.c screen_utils.c screen_play.c screen_file.c \
		      screen_search.c screen_help.c \
                      list_window.c support.c $(ncmpc_headers)




