util = static_library(
  'util',
  'LocaleString.cxx',
  'Exception.cxx',
  'PrintException.cxx',
  'StringCompare.cxx',
  'StringStrip.cxx',
  'StringUTF8.cxx',
  'UriUtil.cxx',
  include_directories: inc,
)

util_dep = declare_dependency(
  link_with: util,
)
