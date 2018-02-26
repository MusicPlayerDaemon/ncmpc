project('ncmpc', 'cpp',
  version: '0.30',
  default_options: [
    'cpp_std=c++14'
  ],
  license: 'GPLv2+',
)

cc = meson.get_compiler('cpp')

conf = configuration_data()
conf.set_quoted('PACKAGE', meson.project_name())
conf.set_quoted('VERSION', meson.project_version())
conf.set_quoted('SYSCONFDIR', join_paths(get_option('prefix'), get_option('sysconfdir')))
conf.set_quoted('LOCALE_DIR', join_paths(get_option('prefix'), get_option('localedir')))

mini = get_option('mini')
conf.set('NCMPC_MINI', mini)

enable_locale = get_option('locale')
if enable_locale == 'false' or mini
  enable_locale = false
elif cc.has_header('locale.h')
  enable_locale = true
elif enable_locale == 'auto'
  enable_locale = false
else
  error('locale.h not found')
endif
conf.set('ENABLE_LOCALE', enable_locale)

enable_nls = get_option('nls')
if enable_nls == 'false' or mini
  enable_nls = false
elif cc.has_header('libintl.h')
  enable_nls = true
elif enable_nls == 'auto'
  enable_nls = false
else
  error('libintl.h not found')
endif
conf.set('ENABLE_NLS', enable_nls)
if enable_nls
  conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
  subdir('po')
endif

if get_option('tcp')
  conf.set('ENABLE_TCP', true)
  conf.set('HAVE_GETADDRINFO', cc.has_function('getaddrinfo'))
endif

async_connect = get_option('async_connect')
conf.set('ENABLE_ASYNC_CONNECT', async_connect)

conf.set('ENABLE_MULTIBYTE', get_option('multibyte'))

curses = get_option('curses')
if curses == 'ncursesw'
  curses_dep = dependency('ncursesw')
elif curses == 'ncurses'
  curses_dep = dependency('ncurses')
else
  curses_dep = dependency('ncursesw', required: false)
  if curses_dep.found()
    curses = 'ncursesw'
  else
    curses_dep = dependency('ncurses', required: false)

    if curses_dep.found()
      curses = 'ncurses'
    else
      curses_dep = cc.find_library('ncursesw', required: false)
      if not curses_dep.found()
        curses_dep = cc.find_library('ncurses')
      endif

      # TODO: find other curses implementation (pdcurses?)
    endif
  endif
endif

if curses == 'ncursesw'
  conf.set('HAVE_NCURSESW', true)
  curses_enhanced = true
  curses_color = true
  if cc.has_header('ncursesw/curses.h', dependencies: curses_dep)
    conf.set('HAVE_NCURSESW_CURSES_H', true)
  elif cc.has_header('ncursesw.h', dependencies: curses_dep)
    conf.set('HAVE_NCURSESW_H', true)
  else
    error('No ncursesw header found')
  endif
elif curses == 'ncurses'
  conf.set('HAVE_NCURSES', true)
  curses_enhanced = true
  curses_color = true
  if cc.has_header('ncurses/curses.h', dependencies: curses_dep)
    conf.set('HAVE_NCURSES_CURSES_H', true)
  elif cc.has_header('ncurses.h', dependencies: curses_dep)
    conf.set('HAVE_NCURSES_H', true)
  else
    error('No ncurses header found')
  endif
else
  if cc.has_header('curses.h', dependencies: curses_dep)
    conf.set('HAVE_CURSES_H', true)

    # TODO: test this with pdcurses
    curses_enhanced = cc.has_header_symbol('curses.h', '_XOPEN_CURSES', dependencies: curses_dep)
    curses_color = curses_enhanced or cc.has_header_symbol('curses.h', 'COLOR_PAIR', dependencies: curses_dep)
  else
    error('No curses header found')
  endif
endif
conf.set('HAVE_CURSES_ENHANCED', curses_enhanced)

enable_mouse = get_option('mouse')
if enable_mouse == 'false'
  enable_mouse = false
elif cc.has_function('getmouse', dependencies: curses_dep)
  enable_mouse = true
elif enable_mouse == 'auto'
  enable_mouse = false
else
  error('getmouse() not available')
endif
conf.set('HAVE_GETMOUSE', enable_mouse)

enable_lirc = get_option('lirc')
if enable_lirc == 'false'
  enable_lirc = false
  lirc_dep = []
else
  lirc_dep = dependency('lirc', required: false)
  if not lirc_dep.found()
    lirc_dep = dependency('liblircclient0', required: false)
  endif

  if lirc_dep.found()
    enable_lirc = true
  elif enable_lirc == 'auto'
    enable_lirc = false
  else
    error('liblirc not found')
  endif
endif
conf.set('ENABLE_LIRC', enable_lirc)

conf.set('ENABLE_COLORS', curses_color)

common_cflags = [
  # for getaddrinfo() and sigaction() with glibc
  '-D_GNU_SOURCE',
]

test_cflags = [
  '-Wall',
  '-Wextra',
  '-Wno-deprecated-declarations',
  '-Wshadow',
  '-Wpointer-arith',
  '-Wcast-qual',
  '-Wcast-align',
  '-Wwrite-strings',
  '-Wmissing-declarations', '-Wmissing-noreturn', '-Wmissing-format-attribute',
  '-Wredundant-decls', '-Wundef',
  '-fvisibility=hidden',
  '-fno-exceptions',
  '-fno-rtti',
]

test_ldflags = [
]

if get_option('buildtype') != 'debug'
  test_cflags += [
    '-ffunction-sections',
    '-fdata-sections',
  ]
  test_ldflags += [
    '-Wl,--gc-sections',
  ]
endif

foreach f: test_cflags
  if cc.has_argument(f)
    common_cflags += [ f ]
  endif
endforeach

add_global_arguments(common_cflags, language: 'cpp')

foreach f: test_ldflags
  if cc.has_argument(f)
    add_global_link_arguments(f, language: 'cpp')
  endif
endforeach

glib_dep = dependency('glib-2.0', version: '>= 2.30')
libmpdclient_dep = dependency('libmpdclient', version: '>= 2.9')

inc = include_directories(
  'src',

  # for the generated config.h
  '.',
)

sources = []

need_screen_text = false
need_plugin_library = false

if host_machine.system() != 'windows'
  sources += [
    'src/signals.cxx',
  ]
endif

if not mini
  sources += [
    'src/db_completion.cxx',
    'src/xterm_title.cxx',
    'src/BasicMarquee.cxx',
    'src/hscroll.cxx',
    'src/match.cxx',
    'src/conf.cxx',
  ]
endif

if async_connect
  sources += [
    'src/net/socket.cxx',
    'src/net/resolver.cxx',
    'src/net/async_connect.cxx',
    'src/net/async_rconnect.cxx',
    'src/aconnect.cxx',
  ]
endif

if enable_lirc
  sources += [
    'src/lirc.cxx',
  ]
endif

enable_help_screen = get_option('help_screen') and not mini
conf.set('ENABLE_HELP_SCREEN', enable_help_screen)
if enable_help_screen
  sources += ['src/screen_help.cxx']
endif

enable_artist_screen = get_option('artist_screen') and not mini
conf.set('ENABLE_ARTIST_SCREEN', enable_artist_screen)
if enable_artist_screen
  sources += ['src/screen_artist.cxx']
endif

enable_search_screen = get_option('search_screen') and not mini
conf.set('ENABLE_SEARCH_SCREEN', enable_search_screen)
if enable_search_screen
  sources += ['src/screen_search.cxx']
endif

enable_song_screen = get_option('song_screen') and not mini
conf.set('ENABLE_SONG_SCREEN', enable_song_screen)
if enable_song_screen
  sources += ['src/screen_song.cxx']
endif

enable_keydef_screen = get_option('key_screen') and not mini
conf.set('ENABLE_KEYDEF_SCREEN', enable_keydef_screen)
if enable_keydef_screen
  sources += ['src/screen_keydef.cxx']
endif

enable_lyrics_screen = get_option('lyrics_screen') and not mini
conf.set('ENABLE_LYRICS_SCREEN', enable_lyrics_screen)
if enable_lyrics_screen
  sources += ['src/screen_lyrics.cxx', 'src/lyrics.cxx']

  lyrics_plugin_dir = get_option('lyrics_plugin_dir')
  if lyrics_plugin_dir == ''
    lyrics_plugin_dir = join_paths(get_option('prefix'), get_option('libdir'), meson.project_name(), 'lyrics')
  endif

  conf.set_quoted('LYRICS_PLUGIN_DIR', lyrics_plugin_dir)
  install_data(
    'lyrics/10-hd.sh',
    'lyrics/20-lyricwiki.rb',
    install_dir: lyrics_plugin_dir)

  need_screen_text = true
  need_plugin_library = true
endif

enable_outputs_screen = get_option('outputs_screen') and not mini
conf.set('ENABLE_OUTPUTS_SCREEN', enable_outputs_screen)
if enable_outputs_screen
  sources += ['src/screen_outputs.cxx']
endif

enable_chat_screen = get_option('chat_screen') and not mini
conf.set('ENABLE_CHAT_SCREEN', enable_chat_screen)
if enable_chat_screen
  sources += ['src/screen_chat.cxx']
  need_screen_text = true
endif

if need_screen_text
  sources += ['src/TextPage.cxx']
endif

if need_plugin_library
  if host_machine.system() == 'windows'
    error('Plugins not yet compatible with Windows')
  endif

  sources += ['src/plugin.cxx']
endif

if host_machine.system() == 'windows'
  subdir('src/win')
endif

ncmpc = executable('ncmpc',
  'src/Main.cxx',
  'src/gidle.cxx',
  'src/mpdclient.cxx',
  'src/callbacks.cxx',
  'src/Queue.cxx',
  'src/filelist.cxx',
  'src/options.cxx',
  'src/command.cxx',
  'src/keyboard.cxx',
  'src/ncu.cxx',
  'src/player_command.cxx',
  'src/TitleBar.cxx',
  'src/ProgressBar.cxx',
  'src/StatusBar.cxx',
  'src/screen.cxx',
  'src/screen_init.cxx',
  'src/screen_paint.cxx',
  'src/screen_utils.cxx',
  'src/screen_status.cxx',
  'src/screen_list.cxx',
  'src/screen_find.cxx',
  'src/screen_client.cxx',
  'src/screen_queue.cxx',
  'src/screen_browser.cxx',
  'src/screen_file.cxx',
  'src/ListWindow.cxx',
  'src/save_playlist.cxx',
  'src/song_paint.cxx',
  'src/colors.cxx',
  'src/charset.cxx',
  'src/wreadln.cxx',
  'src/strfsong.cxx',
  'src/time_format.cxx',
  'src/utils.cxx',
  sources,
  include_directories: inc,
  dependencies: [
    glib_dep,
    curses_dep,
    lirc_dep,
    libmpdclient_dep,
  ],
  install: true
)

configure_file(output: 'config.h', configuration: conf)

docdir = join_paths(get_option('datadir'), 'doc', meson.project_name())
install_data('AUTHORS', 'COPYING', 'NEWS', 'README.rst',
  'doc/config.sample',
  'doc/keys.sample',
  'doc/ncmpc.lirc',
  install_dir: docdir)

if not mini
  subdir('test')
endif

subdir('doc')
