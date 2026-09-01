time = static_library(
  'time',
  'Convert.cxx',
  'Format.cxx',
  include_directories: inc,
  dependencies: [
    fmt_dep,
    intl_dep,
  ],
)

time_dep = declare_dependency(
  link_with: time,
)
