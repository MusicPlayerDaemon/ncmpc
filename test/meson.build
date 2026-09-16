executable(
  'run_hscroll',
  'run_hscroll.cxx',
  objects: ncmpc.extract_objects(
    'src/BasicMarquee.cxx',
  ),
  include_directories: inc,
  dependencies: [
    util_dep,
    fmt_dep,
  ],
)

gtest = dependency('gtest', main: true,
                   include_type: 'system',
                   disabler: true,
                   required: get_option('test'))

subdir('util')
subdir('co')
