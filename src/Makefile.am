#
# $Id$
#

bin_PROGRAMS = ncmpc

ncmpc_LDADD = $(GLIB_LIBS)

AM_CPPFLAGS = $(GLIB_CFLAGS) -DLOCALE_DIR=\""$(datadir)/locale"\" -DSYSCONFDIR=\""$(sysconfdir)"\"


ncmpc_headers = libmpdclient.h mpc.h options.h conf.h command.h screen.h \
                screen_utils.h screen_play.h screen_file.h screen_search.h \
	        screen_help.h list_window.h colors.h support.h \
                wreadln.h ncmpc.h

ncmpc_SOURCES = libmpdclient.c main.c mpc.c options.c conf.c command.c \
		screen.c screen_utils.c screen_play.c screen_file.c \
		screen_search.c screen_help.c screen_keydef.c screen_clock.c \
                list_window.c colors.c support.c wreadln.c $(ncmpc_headers)






