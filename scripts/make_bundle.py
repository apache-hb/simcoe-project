#!/usr/bin/python3

# bundle assets into a folder
# usage: make_bundle.py <bundlefile> <inputdir> <outdir> <depfile>
# <bundlefile> is the input bundle description file
# <inputdir> is the input directory
# <outdir> is the output directory
# <depfile> is the output dependency file

import sys
import os
import json
import shutil
import argparse

class ShaderCompiler:
    # dxc is the path to dxc.exe
    # target_dir is the output directory
    # pdb_dir is the pdb directory
    # debug enables debug mode
    def __init__(self, dxc, target_dir, pdb_dir, debug):
        self.dxc = dxc
        self.target_dir = target_dir
        self.pdb_dir = pdb_dir
        self.debug = debug

        self.args = [ self.dxc, '/WX', '/Ges' ]
        if self.debug:
            self.args += [ '/Zi', '/Fd', self.pdb_dir + '\\' ]

    # file is the input file
    # name is the output name
    # model is the shader model to use
    # target is the shader target to compile for
    # entrypoint is the entry point to use
    def compile_hlsl(self, name, file, model, target, entrypoint):
        output = os.path.join(self.target_dir, f'{name}.{target}.cso')
        extra_args = [ '-T' + target + '_' + model, '-E' + entrypoint, '-Fo' + output ]

        return os.system(' '.join(self.args + extra_args + [ file ]))

argparser = argparse.ArgumentParser(description="bundle assets into a folder")
argparser.add_argument("--bundlefile", help="the input bundle description file")
argparser.add_argument("--inputdir", help="the input directory")
argparser.add_argument("--outdir", help="the output directory")
argparser.add_argument("--output", help="dummy output file")
argparser.add_argument("--depfile", help="the output dependency file")
argparser.add_argument("--dxc", help="the path to dxc.exe")
argparser.add_argument("--debug", help="enable debug mode", action="store_true")

def main():
    args = argparser.parse_args()

    bundlefile = args.bundlefile
    inputdir = args.inputdir

    # nest bundle dir inside outdir so we dont delete
    # the entire meson build dir
    outdir = os.path.join(args.outdir, 'bundle')
    depfile = args.depfile
    dxc = args.dxc

    if not os.path.exists(bundlefile):
        print(f"error: {bundlefile} does not exist")
        sys.exit(1)

    if not os.path.exists(inputdir):
        print(f"error: {inputdir} does not exist")
        sys.exit(1)

    # delete output directorys contents
    if os.path.exists(outdir):
        # cant delete the dir itself because meson is watching it
        for root, dirs, files in os.walk(outdir):
            for file in files:
                os.remove(os.path.join(root, file))
            for dir in dirs:
                shutil.rmtree(os.path.join(root, dir))

    os.makedirs(outdir, exist_ok=True)

    bundle = json.load(open(bundlefile, "r"))
    deps = []

    licensedir = os.path.join(outdir, "licenses")
    fontdir = os.path.join(outdir, "fonts")
    shaderdir = os.path.join(outdir, "shaders")
    pdbdir = os.path.join(outdir, "pdb")

    os.makedirs(licensedir, exist_ok=True)
    os.makedirs(fontdir, exist_ok=True)
    os.makedirs(shaderdir, exist_ok=True)
    os.makedirs(pdbdir, exist_ok=True)

    hlsl = ShaderCompiler(dxc, shaderdir, pdbdir, args.debug)

    licensedata = '# Third party licenses\n\n'
    licensedata += 'All third party licenses are included below as well as in the licenses directory.\n\n'

    for item in bundle['licenses']:
        # copy the license file to <outdir>/licenses
        itempath = os.path.join(inputdir, item['path'])
        targetpath = os.path.join(licensedir, item['id'].upper() + '.LICENSE.md')
        print(f"copying license {item['path']} to {targetpath}")
        deps.append(itempath)
        shutil.copy(itempath, targetpath)
        licensedata += f'## {item["name"]}\n\n'
        licensedata += f'Project url: {item["url"]}\n\n'
        licensedata += open(itempath, "r", encoding='utf8').read()
        licensedata += '\n'

    open(os.path.join(outdir, "LICENSES.md"), "w", encoding='utf8').write(licensedata)

    for item in bundle['fonts']:
        # copy the font file to <outdir>/fonts
        itempath = os.path.join(inputdir, item['path'])
        outpath = os.path.join(fontdir, item['name'] + '.ttf')
        deps.append(itempath)
        shutil.copy(itempath, outpath)
        # rename the font file to <name>.ttf

        print(f"copied font {item['path']} to {outpath}")

    for item in bundle['shaders']:
        name = item['name']
        path = item['path']

        itempath = os.path.join(inputdir, path)

        deps.append(itempath)

        for target in item['targets']:
            result = hlsl.compile_hlsl(item['name'], itempath, '6_0', target['target'], target['entry'])
            if result != 0:
                sys.exit(result)

            print(f"compiled shader {path} {target['entry']} to {path}/{item['name']}.{target['target']}.cso")

    open(depfile, "w").write(args.output + ': ' + ' '.join(deps))

    # touch output file
    open(args.output, "w").close()

if __name__ == "__main__":
    main()
