#!/usr/bin/python3

import os
import sys
import subprocess

builddir = sys.argv[1]
buildtype = sys.argv[2]
basedir = 'data/meson'

has_db2 = os.environ.get('DB2INSTANCE') is not None

def get_db2path():
    try:
        import winreg
        key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, 'SOFTWARE\\IBM\\DB2\\GLOBAL_PROFILE')
        db2path, _ = winreg.QueryValueEx(key, 'DB2PATH')
        return db2path
    except ImportError:
        return None

meson_path = [ 'meson' ]
if os.path.exists('K:/github/meson/meson.py'):
    meson_path = [ 'python', 'K:/github/meson/meson.py' ]

cc_file = 'clang-cl.ini'
# if on linux use clang.ini
if os.name == 'posix':
    cc_file = 'clang.ini'

if __name__ == '__main__':
    cwd = os.getcwd()
    args = [
        *meson_path, 'setup', builddir,
        '--native-file', f'{basedir}/base.ini',
        '--native-file', f'{basedir}/{cc_file}',
        '--native-file', f'{basedir}/{buildtype}.ini',
        '--prefix', f'{cwd}/install-{buildtype}'
    ]

    if has_db2:
        db2path = get_db2path()
        args.append(f'-Ddb2-client:home={db2path}')

    args.extend(sys.argv[3:])

    subprocess.run(args).check_returncode()
