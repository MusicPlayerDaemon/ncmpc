project('ncmpc', 'cpp',
  version: '0.50',
  meson_version: '>= 0.60',
  default_options: [
    'cpp_std=c++2a',
    'warning_level=3',

    # If we build libmpdclient, it should be linked statically into
    # our executable.
    'libmpdclient:default_library=static',
  ],
  license: 'GPLv2+',
)

cc = meson.get_compiler('cpp')
compiler = cc

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

if mini
  enable_nls = false
else
  enable_nls = get_option('nls')
endif

intl_dep = dependency('intl', required: enable_nls)
enable_nls = intl_dep.found()
conf.set('ENABLE_NLS', enable_nls)
if enable_nls
  conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
  subdir('po')
endif

conf.set('HAVE_LOCALE_T', enable_locale and cc.has_header_symbol('locale.h', 'locale_t'))
iconv_dep = dependency('', required: false)
if enable_locale
  iconv_dep = dependency('iconv', required: get_option('iconv'))
  iconv_found = iconv_dep.found()
endif
conf.set('HAVE_ICONV', enable_locale and iconv_found)

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
      if curses_dep.found()
        curses = 'ncursesw'
      else
        curses_dep = cc.find_library('ncurses')
        curses = 'ncurses'
      endif

      # TODO: find other curses implementation (pdcurses?)
    endif
  endif
endif

if not cc.has_header('curses.h', dependencies: curses_dep)
  error('No curses header found')
endif

if curses == 'ncursesw'
  curses_enhanced = true
  curses_color = true
elif curses == 'ncurses'
  curses_enhanced = true
  curses_color = true

  # Manually override NCURSES_WIDECHAR; this suppresses definitions
  # for symbols which are not present in the non-wide ncurses build.
  # This macro really belongs in ncurses.pc, not here.
  curses_dep = declare_dependency(
    compile_args: '-DNCURSES_WIDECHAR=0',
    dependencies: curses_dep,
  )
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

common_cppflags = [
  # for getaddrinfo() and sigaction() with glibc
  '-D_GNU_SOURCE',
]

test_global_common_flags = [
  '-fvisibility=hidden',
]

test_common_flags = [
  '-Wcast-qual',
  '-Wcast-align',
  '-Wdouble-promotion',
  '-Wmissing-declarations',
  '-Wmissing-format-attribute',
  '-Wmissing-noreturn',
  '-Wredundant-decls',
  '-Wshadow',
  '-Wundef',
  '-Wvla',
  '-Wwrite-strings',

  # clang specific warning options:
  '-Wunreachable-code-aggressive',
  '-Wused-but-marked-unused',
]

test_global_cxxflags = test_global_common_flags + [
]

test_cxxflags = test_common_flags + [
  '-fno-threadsafe-statics',
  '-fmerge-all-constants',

  # a vtable without a dtor is just fine
  '-Wno-non-virtual-dtor',

  # the only warnings we got from this are from formatted error
  # messages, and their truncation is harmless
  '-Wno-format-truncation',

  # clang specific warning options:
  '-Wcomma',
  '-Wheader-hygiene',
  '-Winconsistent-missing-destructor-override',
]

test_ldflags = [
]

if get_option('buildtype') != 'debug'
  test_global_cxxflags += [
    '-ffunction-sections',
    '-fdata-sections',
  ]
  test_ldflags += [
    '-Wl,--gc-sections',
  ]
endif

add_global_arguments(common_cppflags, language: 'cpp')
add_global_arguments(cc.get_supported_arguments(test_global_cxxflags), language: 'cpp')
add_project_arguments(cc.get_supported_arguments(test_cxxflags), language: 'cpp')
add_project_link_arguments(cc.get_supported_link_arguments(test_ldflags), language: 'cpp')

is_linux = host_machine.system() == 'linux'
is_windows = host_machine.system() == 'windows'

thread_dep = dependency('threads')

m_dep = []
if not mini
  m_dep = cc.find_library('m', required: false)
endif

libmpdclient_dep = dependency('libmpdclient', version: '>= 2.16')

if not mini
  pcre_dep = dependency('libpcre2-8', required: get_option('regex'))

  if pcre_dep.found()
    pcre_dep = declare_dependency(
      compile_args: '-DPCRE2_CODE_UNIT_WIDTH=0',
      dependencies: pcre_dep,
    )
  endif

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
    'src/ConfigFile.cxx',
    'src/ConfigParser.cxx',
    'src/TableLayout.cxx',
    'src/TablePaint.cxx',
    'src/TableGlue.cxx',
  ]

  if host_machine.system() != 'windows'
    sources += [
      'src/XdgBaseDirectory.cxx',
    ]
  endif
endif

if async_connect
  sources += [
    'src/net/SocketError.cxx',
    'src/net/AsyncResolveConnect.cxx',
    'src/event/net/ConnectSocket.cxx',
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

enable_playlist_editor = get_option('playlist_editor') and not mini
conf.set('ENABLE_PLAYLIST_EDITOR', enable_playlist_editor)
if enable_playlist_editor
  sources += ['src/EditPlaylistPage.cxx']
endif

enable_song_screen = get_option('song_screen') and not mini
conf.set('ENABLE_SONG_SCREEN', enable_song_screen)
if enable_song_screen
  sources += ['src/SongPage.cxx']
endif

enable_keydef_screen = get_option('key_screen') and not mini
conf.set('ENABLE_KEYDEF_SCREEN', enable_keydef_screen)
if enable_keydef_screen
  sources += ['src/KeyDefPage.cxx']
endif

enable_lyrics_screen = get_option('lyrics_screen') and not mini
conf.set('ENABLE_LYRICS_SCREEN', enable_lyrics_screen)
if enable_lyrics_screen
  sources += [
    'src/LyricsPage.cxx',
    'src/LyricsLoader.cxx',
    'src/LyricsCache.cxx',
  ]

  lyrics_plugin_dir = get_option('lyrics_plugin_dir')
  if lyrics_plugin_dir == ''
    lyrics_plugin_dir = join_paths(get_option('prefix'), get_option('libdir'), meson.project_name(), 'lyrics')
  endif

  conf.set_quoted('LYRICS_PLUGIN_DIR', lyrics_plugin_dir)
  install_data(
    'lyrics/20-azlyrics.py',
    'lyrics/25-musixmatch.py',
    'lyrics/30-karaoke_texty.py',
    'lyrics/40-tekstowo.py',
    'lyrics/50-genius.py',
    'lyrics/51-supermusic.py',
    'lyrics/52-zeneszoveg.py',
    'lyrics/60-google.py',
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

if need_plugin_library
  if host_machine.system() == 'windows'
    error('Plugins not yet compatible with Windows')
  endif

  sources += ['src/plugin.cxx']
endif

if host_machine.system() == 'windows'
  subdir('src/win')
endif

subdir('src/util')
subdir('src/co')
subdir('src/lib/fmt')
subdir('src/io')
subdir('src/system')
subdir('src/net')
subdir('src/event')
subdir('src/ui')
subdir('src/dialogs')
subdir('src/page')

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
  'src/AsyncUserInput.cxx',
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
  'src/screen_client.cxx',
  'src/QueuePage.cxx',
  'src/FileListPage.cxx',
  'src/FileBrowserPage.cxx',
  'src/save_playlist.cxx',
  'src/SongRowPaint.cxx',
  'src/BasicColors.cxx',
  'src/CustomColors.cxx',
  'src/Styles.cxx',
  'src/charset.cxx',
  'src/Completion.cxx',
  'src/strfsong.cxx',
  'src/time_format.cxx',
  sources,
  include_directories: inc,
  dependencies: [
    util_dep,
    coroutines_dep,
    thread_dep,
    event_dep,
    pcre_dep,
    intl_dep,
    iconv_dep,
    m_dep,
    curses_dep,
    lirc_dep,
    libmpdclient_dep,
    fmt_dep,
    ui_dep,
    dialogs_dep,
    page_dep,
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

if get_option('test')
  subdir('test')
endif

subdir('doc')
