util_test_sources = []
if has_charset
  util_test_sources += 'TestLocaleConvert.cxx'
endif

test(
  'TestUtil',
  executable(
    'TestUtil',
    'TestException.cxx',
    'TestIntrusiveList.cxx',
    'TestIntrusiveTreeSet.cxx',
    'TestFNVHash.cxx',
    'TestStaticVector.cxx',
    'TestStringStrip.cxx',
    util_test_sources,
    include_directories: inc,
    dependencies: [gtest, util_dep],
  ),
)
