#!/usr/bin/env python3

import os, sys, shutil, subprocess

global_options = ['--werror']

flavors = {
    'debug': {
        'options': [
            '-Ddocumentation=true',
            '-Dcurses=ncursesw',
            '-Dmouse=true',
            '-Dlirc=true',
            '-Dlyrics_screen=true',
            '-Dchat_screen=true',
        ],
    },

    'clang': {
        'options': [
            '-Dcurses=ncursesw',
            '-Dmouse=true',
            '-Dlirc=true',
            '-Dlyrics_screen=true',
            '-Dchat_screen=true',
            '-Ddocumentation=false',
        ],
        'env': {
            'CC': 'clang',
            'CXX': 'clang++',
            'LDFLAGS': '-fuse-ld=lld',
        },
    },

    'release': {
        'options': [
            '--buildtype', 'debugoptimized',
            '-Db_ndebug=true',
            '-Db_lto=true',
            '-Dcurses=ncursesw',
            '-Dmouse=true',
            '-Ddocumentation=false',
        ],
        'env': {
            'LDFLAGS': '-fuse-ld=gold -Wl,--gc-sections,--icf=all',
        },
    },

    'llvm': {
        'options': [
            '--buildtype', 'debugoptimized',
            '-Db_ndebug=true',
            '-Db_lto=true',
            '-Dcurses=ncursesw',
            '-Dmouse=true',
            '-Ddocumentation=false',
        ],
        'env': {
            'CC': 'clang',
            'CXX': 'clang++',
            'LDFLAGS': '-fuse-ld=lld',
        },
    },

    'mini': {
        'options': [
            '--buildtype', 'minsize',
            '-Db_ndebug=true',
            '-Db_lto=true',
            '-Dlirc=false',
            '-Dcurses=ncurses',
            '-Dcolor=false',
            '-Dmouse=false',
            '-Dmultibyte=false',
            '-Dlocale=false',
            '-Dnls=false',
            '-Dtcp=false',
            '-Dasync_connect=false',
            '-Dmini=true',
            '-Ddocumentation=false',
        ],
        'env': {
            'LDFLAGS': '-fuse-ld=gold -Wl,--gc-sections,--icf=all',
        },
    },
}

project_name = 'ncmpc'
source_root = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]) or '.', '..'))
output_path = os.path.join(source_root, 'output')
prefix_root = '/usr/local/stow'

for name, data in flavors.items():
    print(name)
    build_root = os.path.join(output_path, name)

    env = os.environ.copy()
    if 'env' in data:
        env.update(data['env'])

    cmdline = [
        'meson', source_root, build_root,
    ] + global_options

    if 'options' in data:
        cmdline.extend(data['options'])

    prefix = os.path.join(prefix_root, project_name + '-' + name)

    if 'arch' in data:
        prefix = os.path.join(prefix, data['arch'])
        cmdline += ('--cross-file', os.path.join(source_root, 'build', name, 'cross-file.txt'))

    cmdline += ('--prefix', prefix)

    try:
        shutil.rmtree(build_root)
    except:
        pass

    subprocess.check_call(cmdline, env=env)
