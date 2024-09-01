net_sources = []

if host_machine.system() != 'windows'
  net_sources += 'LocalSocketAddress.cxx'
endif

net = static_library(
  'net',
  net_sources,
  'HostParser.cxx',
  'Resolver.cxx',
  'AddressInfo.cxx',
  'AllocatedSocketAddress.cxx',
  'LocalSocketAddress.cxx',
  'IPv4Address.cxx',
  'IPv6Address.cxx',
  'SocketAddress.cxx',
  'SocketDescriptor.cxx',
  include_directories: inc,
  dependencies: [
    fmt_dep,
  ],
)

net_dep = declare_dependency(
  link_with: net,
  dependencies: [
    system_dep,
    io_dep,
  ],
)
