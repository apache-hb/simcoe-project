#!/usr/bin/python3

from subprocess import run
from sys import argv
import os

# usage: compile_hlsl.py <file> <output> <build_dir> <targets> <model> [--debug]
# <file> is the input file
# <output> is the output file
# <build_dir> is the build directory. we sneak around meson and generate pdb files here
# <targets> is the shader targets, comma separated
#           vs,ps,gs,hs,ds,cs
# <model> is the model to use, 6_0 to 6_6

#
# input shaders are expected to conform to naming their entry points as
# <target>_main, where <target> is one of the targets above
# if multiple targets are specified a shader will be compiled for each
#

dxc = 'dxc.exe'
file = argv[1]
output = argv[2]
build_dir = argv[3]
targets = argv[4]
shader_model = argv[5]
debug = '--debug' in argv

cwd = os.getcwd()

for target in targets.split(','):
    entry = f'{target}_main'
    target_model = f'{target}_{shader_model}'
    output_name = f'{output}.{target}.cso'
    args = [ dxc, '-T' + target_model, '-E' + entry, '-Fo' + output_name, '-WX', '-Ges' ]
    if debug:
        args += [ '/Zi', '-DDEBUG=1', '/Fd', f'{build_dir}\\' ]

    args += [ file ]

    result = run(args)
    if result.returncode != 0:
        exit(result.returncode)
