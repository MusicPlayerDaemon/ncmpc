executable(
  'run_hscroll',
  'run_hscroll.c',
  objects: ncmpc.extract_objects(
    'src/hscroll.c',
    'src/charset.c',
  ),
  include_directories: inc,
  dependencies: [
    curses_dep,
    glib_dep,
  ],
)
