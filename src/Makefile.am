#
# $Id$
#

bin_PROGRAMS = ncmpc

ncmpc_LDADD = $(GLIB_LIBS)

AM_CPPFLAGS = $(GLIB_CFLAGS) -DLOCALE_DIR=\""$(datadir)/locale"\" -DSYSCONFDIR=\""$(sysconfdir)"\"


ncmpc_headers = libmpdclient.h mpdclient.h options.h conf.h command.h \
                screen.h screen_utils.h list_window.h colors.h support.h \
                wreadln.h strfsong.h utils.h ncmpc.h

ncmpc_SOURCES = libmpdclient.c main.c mpdclient.c options.c conf.c command.c \
		screen.c screen_utils.c screen_play.c screen_file.c \
		screen_search.c screen_help.c screen_keydef.c screen_clock.c \
                list_window.c colors.c support.c wreadln.c strfsong.c utils.c \
                $(ncmpc_headers)






