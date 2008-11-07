SUBDIRS = \
  src\
  doc\
  po

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = gnu 1.9 dist-bzip2

# $Id$

docdir = $(prefix)/share/doc/$(PACKAGE)

doc_DATA = \
  AUTHORS\
  README\
  NEWS

EXTRA_DIST = \
  $(doc_DATA)

sparse-check:
	$(MAKE) -C src $@

#
# lyrics plugins
#

if ENABLE_LYRICS_SCREEN

lyrics_plugins = hd.sh leoslyrics.py lyricswiki.rb

install-exec-local:
	install -m 0755 -d $(DESTDIR)$(lyrics_plugin_dir)
	cd lyrics && install -m 0755 $(lyrics_plugins) $(DESTDIR)$(lyrics_plugin_dir)

endif
