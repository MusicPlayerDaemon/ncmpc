libavahi_client = dependency('avahi-client', required: get_option('zeroconf'))
if not libavahi_client.found()
  avahi_dep = dependency('', required: false)
  subdir_done()
endif

avahi = static_library(
  'avahi',
  'Client.cxx',
  'Error.cxx',
  'Explorer.cxx',
  'Poll.cxx',
  include_directories: inc,
  dependencies: [
    libavahi_client,
    time_dep,
  ],
)

avahi_dep = declare_dependency(
  link_with: avahi,
  dependencies: [
    event_dep,
    net_dep,
  ],
)
