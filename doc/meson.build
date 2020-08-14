if not get_option('html_manual') and not get_option('manual')
  subdir_done()
endif

sphinx = find_program('sphinx-build', required: get_option('documentation'))
if not sphinx.found()
  subdir_done()
endif

if get_option('html_manual')
  sphinx_output = custom_target(
    'HTML documentation',
    output: 'html',
    input: ['index.rst', 'conf.py'],
    command: [sphinx, '-q', '-b', 'html', '-d', '@OUTDIR@/html_doctrees', meson.current_source_dir(), '@OUTPUT@'],
    build_by_default: true,
    install: true,
    install_dir: join_paths(get_option('datadir'), 'doc', meson.project_name()),
  )

  custom_target(
    'upload',
    input: sphinx_output,
    output: 'upload',
    build_always_stale: true,
    command: [
      'rsync', '-vpruz', '--delete', '@INPUT@',
      'www.musicpd.org:/var/www/mpd/doc/ncmpc/',
      '--chmod=a+rX',
    ],
  )
endif

if get_option('manual')
  custom_target(
    'Manpage documentation',
    output: 'man1',
    input: ['index.rst', 'conf.py'],
    command: [sphinx, '-q', '-b', 'man', '-d', '@OUTDIR@/man_doctrees', meson.current_source_dir(), '@OUTPUT@'],
    build_by_default: true,
    install: true,
    install_dir: get_option('mandir'),
  )
endif
