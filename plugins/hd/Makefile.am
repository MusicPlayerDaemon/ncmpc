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

ncmpc_modules_LTLIBRARIES = libhd.la 

### libhd

libhd_la_LIBADD =\
  $(GLIB_LIBS)\
  $(libcurl_LIBS)

libhd_la_headers =\
			  $(plugins_headers)

libhd_la_SOURCES =\
			  lyrics_hd.c\
			  $(libhd_la_headers)
install:
	@$(MAKE)
	mkdir -p ${ncmpc_modulesdir}
	if test -f .libs/libhd.so; then cp .libs/libhd.so ${ncmpc_modulesdir}/lyrics_hd.so; fi

uninstall:
	@$(MAKE)
	rm -f ${ncmpc_modulesdir}/lyrics_hd.so
