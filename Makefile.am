#
# $Id: Makefile.am,v 1.5 2004/03/16 23:49:14 kalle Exp $
#

bin_PROGRAMS = mpc-ncurses
man_MANS = doc/ncmpc.1
pkgdata_DATA =
docdir = $(prefix)/share/doc/$(PACKAGE)
doc_DATA = AUTHORS README ChangeLog
EXTRA_DIST = COPYING $(pkgdata_DATA) $(man_MANS) $(doc_DATA)

mpc_ncurses_headers = libmpdclient.h mpc.h options.h command.h screen.h \
              screen_utils.h screen_play.h screen_file.h screen_search.h \
	      screen_help.h support.h

mpc_ncurses_SOURCES = libmpdclient.c main.c mpc.c options.c command.c \
		      screen.c screen_utils.c screen_play.c screen_file.c \
		      screen_search.c screen_help.c \
                      support.c $(mpc_ncurses_headers)




