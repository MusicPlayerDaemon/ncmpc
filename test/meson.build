executable(
  'run_hscroll',
  'run_hscroll.cxx',
  objects: ncmpc.extract_objects(
    'src/hscroll.cxx',
    'src/charset.cxx',
  ),
  include_directories: inc,
  dependencies: [
    curses_dep,
    glib_dep,
  ],
)
