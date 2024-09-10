libmpdclient_dep = dependency('libmpdclient', version: '>= 2.16')

client_sources = [
  'gidle.cxx',
  'mpdclient.cxx',
]

if async_connect
  client_sources += [
    '../net/SocketError.cxx',
    '../net/AsyncResolveConnect.cxx',
    '../event/net/ConnectSocket.cxx',
    'aconnect.cxx',
  ]
endif

client = static_library(
  'client',
  client_sources,
  include_directories: inc,
  dependencies: [
    libmpdclient_dep,
    event_dep,
  ],
)

client_dep = declare_dependency(
  link_with: client,
  dependencies: [
    libmpdclient_dep,
    event_dep,
  ],
)
