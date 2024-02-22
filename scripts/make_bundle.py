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
import tarfile
import configparser
from pathlib import Path

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
argparser.add_argument("--bundle", help="the input bundle description file")
argparser.add_argument("--indir", help="the input directory")
argparser.add_argument("--outdir", help="the output directory")
argparser.add_argument("--output", help="output tar file")
argparser.add_argument("--depfile", help="the output dependency file")
argparser.add_argument("--config", help="config file with tool paths")
argparser.add_argument("--debug", help="enable debug mode", action="store_true")

def main():
    args = argparser.parse_args()

    bundlefile = args.bundle
    inputdir = args.indir

    # nest bundle dir inside outdir so we dont delete
    # the entire meson build dir
    outdir = os.path.join(args.outdir, 'bundle')
    depfile = args.depfile
    config = configparser.ConfigParser()
    config.read(args.config)
    dxc = config['tools']['dxc']
    msdf = config['tools']['msdf'] # msdf atlas generator
    compressor = config['tools']['compressor'] # compressonator cli

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
    texturedir = os.path.join(outdir, "textures")
    shaderdir = os.path.join(outdir, "shaders")
    pdbdir = os.path.join(outdir, "pdb")

    os.makedirs(licensedir, exist_ok=True)
    os.makedirs(fontdir, exist_ok=True)
    os.makedirs(texturedir, exist_ok=True)
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
        outpath_ttf = os.path.join(fontdir, item['name'] + '.ttf')
        print(f"copying font {item['path']} to {outpath_ttf}")
        deps.append(itempath)
        shutil.copy(itempath, outpath_ttf)

        em_size = item.get('em_size', 64)

        atlas_path = os.path.join(fontdir, item['name'])
        print(f"generating msdf atlas for {item['name']} ({em_size=})")
        result = os.system(f'{msdf} -font {itempath} -size {em_size} -format png -imageout {atlas_path}.png -json {atlas_path}.json')
        if result != 0:
            sys.exit(result)

    for item in bundle['shaders']:
        name = item['name']
        path = item['path']

        itempath = os.path.join(inputdir, path)

        deps.append(itempath)

        for target in item['targets']:
            result = hlsl.compile_hlsl(name, itempath, '6_0', target['target'], target['entry'])
            if result != 0:
                sys.exit(result)

            print(f"compiled shader {path} {target['entry']} to {path}/{name}.{target['target']}.cso")

    for texture in bundle['textures']:
        itempath = os.path.join(inputdir, texture['path'])
        deps.append(itempath)

        mips = texture.get('mips', 1)

        outpath = os.path.join(texturedir, texture['name'] + '.dds')
        print(f"compressing texture {texture['path']} into {outpath} {mips=}")
        os.system(f'{compressor} -fd BC7 {itempath} {outpath} -miplevels {mips}')

    open(depfile, "w").write(args.output + ': ' + ' '.join(deps))

    with tarfile.open(args.output, "w", format=tarfile.USTAR_FORMAT) as tar:
        tar.add(outdir, arcname=os.path.basename(outdir))

if __name__ == "__main__":
    main()
