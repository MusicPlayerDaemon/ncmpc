page_sources = [
  'Page.cxx',
  'ProxyPage.cxx',
]

if need_screen_text
  page_sources += 'TextPage.cxx'
endif

page = static_library(
  'page',
  page_sources,
  include_directories: inc,
  dependencies: [
    ui_dep,
  ],
)

page_dep = declare_dependency(
  link_with: page,
  dependencies: [
    ui_dep,
  ],
)
