SUBDIRS = \
  src\
  doc\
  po

ACLOCAL_AMFLAGS = -I m4
AUTOMAKE_OPTIONS = foreign 1.9 dist-bzip2

doc_DATA = \
  AUTHORS\
  README\
  NEWS

EXTRA_DIST = \
  $(doc_DATA)

# build the smalles possible ncmpc binary
ncmpc-tiny:
	$(MAKE) -C src $@

sparse-check:
	$(MAKE) -C src $@

#
# lyrics plugins
#

if ENABLE_LYRICS_SCREEN

lyrics_plugins = hd.sh leoslyrics.py lyricswiki.rb

EXTRA_DIST += $(addprefix lyrics/,$(lyrics_plugins))

install-exec-local:
	install -m 0755 -d $(DESTDIR)$(lyrics_plugin_dir)
	cd lyrics && install -m 0755 $(lyrics_plugins) $(DESTDIR)$(lyrics_plugin_dir)

endif
