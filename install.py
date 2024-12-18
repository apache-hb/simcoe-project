#!/usr/bin/python3

import os
import sys
import subprocess

builddir = sys.argv[1]

meson_path = [ 'meson' ]
if os.path.exists('K:/github/meson/meson.py'):
    meson_path = [ 'python', 'K:/github/meson/meson.py' ]

def install_component(tags: list[str], skip_subprojects = False):
    args = meson_path + [ 'install', '-C', builddir ]
    if skip_subprojects is True:
        args.append('--skip-subprojects')
    elif skip_subprojects is False:
        pass
    elif len(skip_subprojects) > 0:
        args.append('--skip-subprojects')
        args.append(','.join(skip_subprojects))
    args.append('--tags')
    args.append(','.join(tags))

    subprocess.run(args).check_returncode()

if __name__ == '__main__':
    install_component([ 'bin', 'runtime'], skip_subprojects = True)
    install_component([ 'legal' ])
    install_component([ 'bin', 'runtime' ], skip_subprojects = [ 'libxml2' ])
