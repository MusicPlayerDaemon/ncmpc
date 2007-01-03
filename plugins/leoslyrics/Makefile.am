AM_CPPFLAGS =\
  $(GLIB_CFLAGS)\
  $(GTHREAD_CFLAGS)\
  $(libcurl_CFLAGS)\
  -I../../src

###----

plugin_headers =\
		 ../src_lyrics.h\
		 ../screen_lyrics.h\
		 ../easy_download.h\
		 ../options.h

###-----

ncmpc_modulesdir = @plugindir@

ncmpc_modules_LTLIBRARIES = libleoslyrics.la 

### libleoslyrics

libleoslyrics_la_LIBADD =\
  $(GLIB_LIBS)\
  $(libcurl_LIBS)

libleoslyrics_la_headers =\
			  easy_download.h\
			  $(plugins_headers)

libleoslyrics_la_SOURCES =\
			  easy_download.c\
			  lyrics_leoslyrics.c\
			  $(libleoslyrics_la_headers)

install:
	@$(MAKE)
	mkdir -p ${DESTDIR}${libdir}
	if test -f .libs/libleoslyrics.so; then cp .libs/libleoslyrics.so ${ncmpc_modulesdir}/lyrics_leoslyrics.so; fi

uninstall:
	@$(MAKE)
	rm -f ${ncmpc_modulesdir}/lyrics_leoslyrics.so
