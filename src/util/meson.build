util_sources = []

if has_charset
  util_sources += 'LocaleConvert.cxx'
endif

util = static_library(
  'util',
  util_sources,
  'LocaleString.cxx',
  'LocaleExtract.cxx',
  'LocaleWord.cxx',
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
