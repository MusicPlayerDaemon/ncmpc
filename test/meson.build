executable(
  'run_hscroll',
  'run_hscroll.cxx',
  objects: ncmpc.extract_objects(
    'src/BasicMarquee.cxx',
    'src/util/LocaleString.cxx',
  ),
  include_directories: inc,
  dependencies: [
    fmt_dep,
  ],
)
