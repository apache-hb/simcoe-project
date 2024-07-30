#!/usr/bin/python3

import os
import sys
import subprocess

builddir = sys.argv[1]
buildtype = sys.argv[2]
basedir = 'data/meson'

oracle_home = os.environ.get('ORACLE_HOME')
oracle_base = os.environ.get('ORACLE_BASE')

if __name__ == '__main__':
    args = [
        'meson', 'setup', builddir,
        '--native-file', f'{basedir}/base.ini',
        '--native-file', f'{basedir}/clang-cl.ini',
        '--native-file', f'{basedir}/{buildtype}.ini'
    ]

    if oracle_home and oracle_base:
        oracle_home = oracle_home.replace('\\', '/')
        oracle_base = oracle_base.replace('\\', '/')
        args.extend([
            f'-Doradb-oci:oracle_home={oracle_home}',
            f'-Doradb-oci:oracle_base={oracle_base}'
        ])

    args.extend(sys.argv[3:])

    subprocess.run(args).check_returncode()
