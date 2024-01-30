#!/usr/bin/python3

import sys
import os

# usage: compile_hlsl.py <dxc> <file> <output> <build_dir> <targets> <model> [--debug]
# <dxc> is the path to dxc.exe
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

def compile_hlsl(dxc, file, output, build_dir, targets, shader_model, debug, pdb_dir = None):
    if pdb_dir is None:
        pdb_dir = build_dir

    for target in targets.split(','):
        entry = f'{target}_main'
        target_model = f'{target}_{shader_model}'
        output_name = f'{output}.{target}.cso'
        args = [ dxc, '-T' + target_model, '-E' + entry, '-Fo' + output_name, '-WX', '-Ges' ]
        if debug:
            args += [ '/Zi', '-DDEBUG=1', '/Fd', f'{pdb_dir}\\' ]

        args += [ file ]

        result = os.system(' '.join(args))
        if result != 0:
            return result

    return 0

def main():
    dxc = sys.argv[1]
    file = sys.argv[2]
    output = sys.argv[3]
    build_dir = sys.argv[4]
    targets = sys.argv[5]
    shader_model = sys.argv[6]
    debug = '--debug' in sys.argv

    sys.exit(compile_hlsl(dxc, file, output, build_dir, targets, shader_model, debug))

if __name__ == '__main__':
    main()
