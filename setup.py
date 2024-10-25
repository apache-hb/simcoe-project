#!/usr/bin/python3

import os
import sys
import subprocess

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

    # TODO: this is a bit fudged, but db2 doesnt provide either
    # a redist package i can wrap, or an environment variable
    # with its base path. so we just assume the default install
    if has_db2:
        args.append(f'-Ddb2-client:home=C:/Program Files/IBM/SQLLIB')

    args.extend(sys.argv[3:])

    subprocess.run(args).check_returncode()
