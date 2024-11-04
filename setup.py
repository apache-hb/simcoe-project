#!/usr/bin/python3

import os
import sys
import subprocess
import winreg

builddir = sys.argv[1]
buildtype = sys.argv[2]
basedir = 'data/meson'

has_db2 = os.environ.get('DB2INSTANCE') is not None

meson_path = [ 'meson' ]
if os.path.exists('K:/github/meson/meson.py'):
    meson_path = [ 'python', 'K:/github/meson/meson.py' ]

if __name__ == '__main__':
    args = [
        *meson_path, 'setup', builddir,
        '--native-file', f'{basedir}/base.ini',
        '--native-file', f'{basedir}/clang-cl.ini',
        '--native-file', f'{basedir}/{buildtype}.ini'
    ]

    if has_db2:
        key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, 'SOFTWARE\\IBM\\DB2\\GLOBAL_PROFILE')
        db2path, _ = winreg.QueryValueEx(key, 'DB2PATH')
        args.append(f'-Ddb2-client:home={db2path}')

    args.extend(sys.argv[3:])

    subprocess.run(args).check_returncode()
