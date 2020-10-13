event_features = configuration_data()
event_features.set('USE_EVENTFD', is_linux and get_option('eventfd'))
event_features.set('USE_SIGNALFD', is_linux and get_option('signalfd'))
event_features.set('USE_EPOLL', is_linux and get_option('epoll'))
configure_file(output: 'Features.h', configuration: event_features)

event_sources = []

if is_windows
  event_sources += 'PollGroupWinSelect.cxx'
elif is_linux and get_option('epoll')
  # epoll support is header-only
else
  event_sources += 'PollGroupPoll.cxx'
endif

event = static_library(
  'event',
  'SignalMonitor.cxx',
  'TimerEvent.cxx',
  'IdleEvent.cxx',
  'SocketEvent.cxx',
  'Loop.cxx',
  event_sources,
  include_directories: inc,
  dependencies: [
    boost_dep,
  ],
)

event_dep = declare_dependency(
  link_with: event,
  dependencies: [
    thread_dep,
    net_dep,
    system_dep,
    boost_dep,
  ],
)