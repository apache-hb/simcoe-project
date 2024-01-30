#!/usr/bin/python3

# bundle assets into a folder
# usage: make_bundle.py <bundlefile> <inputdir> <outdir> <depfile>
# <bundlefile> is the input bundle description file
# <inputdir> is the input directory
# <outdir> is the output directory
# <depfile> is the output dependency file

from . import ShaderCompiler

import sys
import os
import json
import shutil
import argparse

argparser = argparse.ArgumentParser(description="bundle assets into a folder")
argparser.add_argument("bundlefile", help="the input bundle description file")
argparser.add_argument("inputdir", help="the input directory")
argparser.add_argument("outdir", help="the output directory")
argparser.add_argument("depfile", help="the output dependency file")
argparser.add_argument("dxc", help="the path to dxc.exe")
argparser.add_argument("--debug", help="enable debug mode", action="store_true")

def main():
    args = argparser.parse_args()

    bundlefile = args.bundlefile
    inputdir = args.inputdir
    outdir = args.outdir
    depfile = args.depfile
    dxc = args.dxc

    if not os.path.exists(bundlefile):
        print(f"error: {bundlefile} does not exist")
        sys.exit(1)

    if not os.path.exists(inputdir):
        print(f"error: {inputdir} does not exist")
        sys.exit(1)

    if os.path.exists(outdir):
        shutil.rmtree(outdir)

    os.makedirs(outdir)

    bundle = json.load(open(bundlefile, "r"))
    deps = []

    licensedir = os.path.join(outdir, "licenses")
    fontdir = os.path.join(outdir, "fonts")
    shaderdir = os.path.join(outdir, "shaders")
    pdbdir = os.path.join(outdir, "pdb")

    os.makedirs(licensedir)
    os.makedirs(fontdir)
    os.makedirs(shaderdir)
    os.makedirs(pdbdir)

    hlsl = ShaderCompiler(dxc, shaderdir, pdbdir, args.debug)

    licensedata = '# Third party licenses\n\n'

    for item in bundle['licenses']:
        # copy the license file to <outdir>/licenses
        itempath = os.path.join(inputdir, item['path'])
        deps.append(itempath)
        shutil.copy(itempath, licensedir)
        licensedata += f'## {item["name"]}\n'
        licensedata += f'Project url: {item["url"]}\n\n'
        licensedata += open(itempath, "r").read()

        print(f"copied license {item['path']} to {licensedir}")

    licensedata += '\n'

    open(os.path.join(outdir, "LICENSES.md"), "w").write(licensedata)

    for item in bundle['fonts']:
        # copy the font file to <outdir>/fonts
        basepath = os.path.basename(item['path'])
        itempath = os.path.join(inputdir, item['path'])
        deps.append(itempath)
        shutil.copy(itempath, fontdir)
        # rename the font file to <name>.ttf
        name = item['name']
        os.rename(os.path.join(fontdir, basepath), os.path.join(fontdir, name + '.ttf'))

        print(f"copied font {item['path']} to {fontdir}")

    for item in bundle['shaders']:
        name = item['name']
        path = item['path']

        itempath = os.path.join(inputdir, path)

        deps.append(itempath)

        for target in item['targets']:
            result = hlsl.compile_hlsl(itempath, '6_0', target['target'], target['entry'])
            if result != 0:
                sys.exit(result)

            print(f"compiled shader {path} {target['entry']} to {path}.{target['target']}.cso")

    open(depfile, "w").write('\n'.join(deps))

if __name__ == "__main__":
    main()
