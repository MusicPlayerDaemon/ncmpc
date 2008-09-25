SUBDIRS = \
  src\
  doc\
  po

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS =   gnu 1.6

# $Id$

docdir = $(prefix)/share/doc/$(PACKAGE)

doc_DATA = \
  AUTHORS\
  README\
  NEWS

EXTRA_DIST = \
  $(doc_DATA)


#
# lyrics plugins
#

if LYRICS

lyrics_plugin_dir = @lyrics_plugin_dir@
lyrics_plugins = hd.sh leoslyrics.py lyricswiki.rb

install-exec-local:
	install -m 0755 -d $(DESTDIR)$(lyrics_plugin_dir)
	install -m 0755 $(addprefix lyrics/,$(lyrics_plugins)) $(DESTDIR)$(lyrics_plugin_dir)

endif
