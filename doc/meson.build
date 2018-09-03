sphinx = find_program('sphinx-build', required: enable_documentation == 'true')
if sphinx.found()
  custom_target(
    'HTML documentation',
    output: 'html',
    input: ['index.rst', 'conf.py'],
    command: [sphinx, '-q', '-b', 'html', '-d', '@OUTDIR@/doctrees', meson.current_source_dir(), '@OUTPUT@'],
    build_by_default: true,
    install: true,
    install_dir: join_paths(get_option('datadir'), 'doc', meson.project_name()),
  )

  custom_target(
    'Manpage documentation',
    output: 'man',
    input: ['index.rst', 'conf.py'],
    command: [sphinx, '-q', '-b', 'man', '-d', '@OUTDIR@/doctrees', meson.current_source_dir(), '@OUTPUT@/man1'],
    build_by_default: true,
    install: true,
    install_dir: get_option('datadir'),
  )
endif

doxygen = find_program('doxygen', required: enable_documentation == 'true')
if doxygen.found()
  dconf = configuration_data()
  dconf.set('VERSION', meson.project_version())
  dconf.set('abs_top_srcdir', meson.source_root())

  doxyfile = configure_file(input: 'doxygen.conf.in',
                            output: 'doxygen.conf',
                            configuration: dconf)

  datadir = join_paths(get_option('datadir'), 'doc', 'spede')

  html_target = custom_target(
    'apidocs',
    input: doxyfile,
    output: 'api',
    command: [doxygen, doxyfile],
    install: true,
    install_dir: docdir,
  )
endif
