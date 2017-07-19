splitted_version = meson.project_version().split('.')

wconf = configuration_data()
wconf.set('VERSION_MAJOR', splitted_version[0])
wconf.set('VERSION_MINOR', splitted_version[1])
if splitted_version.length() >= 3
  wconf.set('VERSION_REVISION', splitted_version[2])
else
  wconf.set('VERSION_REVISION', '0')
endif
if splitted_version.length() >= 4
  wconf.set('VERSION_EXTRA', splitted_version[3])
else
  wconf.set('VERSION_EXTRA', '0')
endif
configure_file(input: 'ncmpc_win32_rc.rc.in', output: 'ncmpc_win32_rc.rc', configuration: wconf)

windows = import('windows')
sources += windows.compile_resources('ncmpc_win32_rc.rc')
