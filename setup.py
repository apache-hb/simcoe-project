#!/usr/bin/python3

import os
import sys
import subprocess

builddir = sys.argv[1]
buildtype = sys.argv[2]
basedir = 'data/meson'

oracle_home = os.environ.get('ORACLE_HOME')

if __name__ == '__main__':
    args = [
        'meson', 'setup', builddir,
        '--native-file', f'{basedir}/base.ini',
        '--native-file', f'{basedir}/clang-cl.ini',
        '--native-file', f'{basedir}/{buildtype}.ini'
    ]

    if oracle_home:
        oracle_home = oracle_home.replace('\\', '/')
        args.append(f'-Dorcl-oci:home={oracle_home}')

    args.extend(sys.argv[3:])

    subprocess.run(args).check_returncode()
