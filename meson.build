project('ncmpc', 'cpp',
  version: '0.33',
  meson_version: '>= 0.47',
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
if enable_locale.disabled() or mini
  enable_locale = false
elif cc.has_header('locale.h')
  enable_locale = true
elif enable_locale.auto()
  enable_locale = false
else
  error('locale.h not found')
endif
conf.set('ENABLE_LOCALE', enable_locale)

enable_nls = get_option('nls')
if enable_nls.disabled() or mini
  enable_nls = false
elif cc.has_header('libintl.h')
  enable_nls = true
elif enable_nls.auto()
  enable_nls = false
else
  error('libintl.h not found')
endif
conf.set('ENABLE_NLS', enable_nls)
if enable_nls
  conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
  subdir('po')
endif

conf.set('HAVE_LOCALE_T', enable_locale and cc.has_header_symbol('locale.h', 'locale_t'))
conf.set('HAVE_ICONV', enable_locale and cc.has_function('iconv'))

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

if not cc.has_header('curses.h', dependencies: curses_dep)
  error('No curses header found')
endif

if curses == 'ncursesw'
  conf.set('HAVE_NCURSESW', true)
  curses_enhanced = true
  curses_color = true
elif curses == 'ncurses'
  conf.set('HAVE_NCURSES', true)
  curses_enhanced = true
  curses_color = true
else
  # TODO: test this with pdcurses
  curses_enhanced = cc.has_header_symbol('curses.h', '_XOPEN_CURSES', dependencies: curses_dep)
  curses_color = curses_enhanced or cc.has_header_symbol('curses.h', 'COLOR_PAIR', dependencies: curses_dep)
endif
conf.set('HAVE_CURSES_ENHANCED', curses_enhanced)

enable_mouse = get_option('mouse')
if enable_mouse.disabled()
  enable_mouse = false
elif cc.has_function('getmouse', dependencies: curses_dep)
  enable_mouse = true
elif enable_mouse.auto()
  enable_mouse = false
else
  error('getmouse() not available')
endif
conf.set('HAVE_GETMOUSE', enable_mouse)

lirc_dep = dependency('lirc', required: get_option('lirc'))
conf.set('ENABLE_LIRC', lirc_dep.found())

conf.set('ENABLE_COLORS', curses_color and get_option('colors'))

common_cflags = [
  # for getaddrinfo() and sigaction() with glibc
  '-D_GNU_SOURCE',

  # no header bloat for the iostream library we don't use
  '-DBOOST_NO_IOSTREAM',

  # avoid the runtime dependency on libboost_system
  '-DBOOST_ERROR_CODE_HEADER_ONLY',

  # disable deprecated boost::system features
  '-DBOOST_SYSTEM_NO_DEPRECATED',
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
  '-Wno-non-virtual-dtor',
  '-fvisibility=hidden',

  # the only warnings we got from this are from formatted error
  # messages, and their truncation is harmless
  '-Wno-format-truncation',
]

if get_option('buildtype') != 'debug'
  test_cflags += [
    '-ffunction-sections',
    '-fdata-sections',
  ]
endif

foreach f: test_cflags
  if cc.has_argument(f)
    common_cflags += [ f ]
  endif
endforeach

add_global_arguments(common_cflags, language: 'cpp')

thread_dep = dependency('threads')
boost_dep = dependency('boost', version: '>= 1.62')
libmpdclient_dep = dependency('libmpdclient', version: '>= 2.9')

if not mini
  pcre_dep = dependency('libpcre', required: get_option('regex'))
  conf.set('HAVE_PCRE', pcre_dep.found())
else
  pcre_dep = declare_dependency()
endif

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
    'src/conf.cxx',
  ]
endif

if async_connect
  sources += [
    'src/net/AsyncConnect.cxx',
    'src/net/AsyncResolveConnect.cxx',
    'src/aconnect.cxx',
  ]
endif

if lirc_dep.found()
  sources += [
    'src/lirc.cxx',
  ]
endif

enable_help_screen = get_option('help_screen') and not mini
conf.set('ENABLE_HELP_SCREEN', enable_help_screen)
if enable_help_screen
  sources += ['src/HelpPage.cxx']
endif

enable_library_screen = get_option('library_screen') and not mini
conf.set('ENABLE_LIBRARY_PAGE', enable_library_screen)
if enable_library_screen
  sources += [
    'src/LibraryPage.cxx',
    'src/TagListPage.cxx',
    'src/TagFilter.cxx',
  ]
endif

enable_search_screen = get_option('search_screen') and not mini
conf.set('ENABLE_SEARCH_SCREEN', enable_search_screen)
if enable_search_screen
  sources += ['src/SearchPage.cxx']
endif

enable_song_screen = get_option('song_screen') and not mini
conf.set('ENABLE_SONG_SCREEN', enable_song_screen)
if enable_song_screen
  sources += ['src/SongPage.cxx']
endif

enable_keydef_screen = get_option('key_screen') and not mini
conf.set('ENABLE_KEYDEF_SCREEN', enable_keydef_screen)
if enable_keydef_screen
  sources += ['src/screen_keydef.cxx']
endif

enable_lyrics_screen = get_option('lyrics_screen') and not mini
conf.set('ENABLE_LYRICS_SCREEN', enable_lyrics_screen)
if enable_lyrics_screen
  sources += ['src/LyricsPage.cxx', 'src/lyrics.cxx']

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
  sources += ['src/OutputsPage.cxx']
endif

enable_chat_screen = get_option('chat_screen') and not mini
conf.set('ENABLE_CHAT_SCREEN', enable_chat_screen)
if enable_chat_screen
  sources += ['src/ChatPage.cxx']
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
  'src/Instance.cxx',
  'src/gidle.cxx',
  'src/mpdclient.cxx',
  'src/callbacks.cxx',
  'src/Queue.cxx',
  'src/filelist.cxx',
  'src/Options.cxx',
  'src/Command.cxx',
  'src/Bindings.cxx',
  'src/GlobalBindings.cxx',
  'src/keyboard.cxx',
  'src/KeyName.cxx',
  'src/Match.cxx',
  'src/ncu.cxx',
  'src/player_command.cxx',
  'src/DelayedSeek.cxx',
  'src/TabBar.cxx',
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
  'src/QueuePage.cxx',
  'src/FileListPage.cxx',
  'src/FileBrowserPage.cxx',
  'src/ProxyPage.cxx',
  'src/ListWindow.cxx',
  'src/TextListRenderer.cxx',
  'src/save_playlist.cxx',
  'src/song_paint.cxx',
  'src/BasicColors.cxx',
  'src/CustomColors.cxx',
  'src/Styles.cxx',
  'src/charset.cxx',
  'src/wreadln.cxx',
  'src/Completion.cxx',
  'src/strfsong.cxx',
  'src/time_format.cxx',
  'src/util/LocaleString.cxx',
  'src/util/StringStrip.cxx',
  'src/util/StringUTF8.cxx',
  'src/util/UriUtil.cxx',
  sources,
  include_directories: inc,
  dependencies: [
    thread_dep,
    boost_dep,
    pcre_dep,
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
