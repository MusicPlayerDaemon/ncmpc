ui = static_library(
  'ui',
  'ListCursor.cxx',
  'ListWindow.cxx',
  'TextListRenderer.cxx',
  include_directories: inc,
  dependencies: [
    curses_dep,
    pcre_dep,
  ],
)

ui_dep = declare_dependency(
  link_with: ui,
  dependencies: [
    curses_dep,
  ],
)
