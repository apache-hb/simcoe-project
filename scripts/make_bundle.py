#!/usr/bin/python3

# bundle assets into a folder

import sys
import os
import json
import shutil
import argparse
import tarfile
import configparser
from pathlib import Path

pj = os.path.join

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

class AtlasGenerator:
    def __init__(self, atlasgen, target_dir, input_dir):
        self.atlasgen = atlasgen
        self.target_dir = target_dir
        self.input_dir = input_dir

    def build_atlas(self, options):
        em_size = options.get('em_size', 64)
        atlas_path = os.path.join(self.target_dir, options['name'])
        fonts = ' -and '.join([ f'-font {it}' for it in self.get_font_files(options) ])
        cmd = f'{self.atlasgen} {fonts} -size {em_size} -format png -arfont {atlas_path}.arfont -imageout {atlas_path}.png'
        print(cmd)
        return os.system(cmd)

    def get_font_files(self, options):
        path = os.path.join(self.input_dir, options['path'])
        if 'files' in options:
            return [ os.path.join(path, file) for file in options['files'] ]
        else:
            return [ path ]

argparser = argparse.ArgumentParser(description="bundle assets into a folder")
argparser.add_argument("--bundle", help="the input bundle description file")
argparser.add_argument("--indir", help="the input directory")
argparser.add_argument("--outdir", help="the output directory")
argparser.add_argument("--output", help="output tar file")
argparser.add_argument("--depfile", help="the output dependency file")
argparser.add_argument("--config", help="config file with tool paths")
argparser.add_argument("--debug", help="enable debug mode", action="store_true")
args = argparser.parse_args()
bundlefile = args.bundle
inputdir = args.indir

bundle_dir = os.path.join(args.outdir, 'bundle')
redist_dir = os.path.join(args.outdir, 'redist')

deps = []

def copyfile(src, dst):
    print(f"copying {src} to {dst}")
    shutil.copy(src, dst)
    deps.append(src)

def copy_redist(outdir, indir, redists, debug_redists):
    os.makedirs(outdir, exist_ok=True)
    for redist in redists:
        copyfile(pj(indir, redist), outdir)

    if not args.debug:
        return

    for redist in debug_redists:
        copyfile(pj(indir, redist), outdir)

def copy_redist_files(outdir, config):
    agility_redist = config['redist']['agility']
    dxc_redist = config['redist']['dxcompiler']
    warp_redist = config['redist']['warp']
    pix_redist = config['redist']['winpixruntime']
    os.makedirs(outdir, exist_ok=True)

    copy_redist(pj(outdir, 'd3d12'), agility_redist,
        [ 'D3D12Core.dll' ],
        [ 'D3D12Core.pdb', 'D3D12SDKLayers.dll', 'd3d12SDKLayers.pdb' ]
    )

    copy_redist(outdir, dxc_redist,
        [ 'dxcompiler.dll', 'dxil.dll' ],
        [ 'dxcompiler.pdb', 'dxil.pdb' ]
    )

    copy_redist(outdir, warp_redist,
        [ ],
        [ 'd3d10warp.dll', 'd3d10warp.pdb' ]
    )

    copy_redist(outdir, pix_redist,
        [ ],
        [ 'WinPixEventRuntime.dll' ]
    )

def main():
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
    if os.path.exists(bundle_dir):
        # cant delete the dir itself because meson is watching it
        for root, dirs, files in os.walk(bundle_dir):
            for file in files:
                os.remove(os.path.join(root, file))
            for dir in dirs:
                shutil.rmtree(os.path.join(root, dir))

    copy_redist_files(redist_dir, config)

    os.makedirs(bundle_dir, exist_ok=True)

    bundle = json.load(open(bundlefile, "r"))

    licensedir = os.path.join(bundle_dir, "licenses")
    fontdir = os.path.join(bundle_dir, "fonts")
    texturedir = os.path.join(bundle_dir, "textures")
    shaderdir = os.path.join(bundle_dir, "shaders")
    pdbdir = os.path.join(bundle_dir, "pdb")

    os.makedirs(licensedir, exist_ok=True)
    os.makedirs(fontdir, exist_ok=True)
    os.makedirs(texturedir, exist_ok=True)
    os.makedirs(shaderdir, exist_ok=True)
    os.makedirs(pdbdir, exist_ok=True)

    hlsl = ShaderCompiler(dxc, shaderdir, pdbdir, args.debug)
    atlasgen = AtlasGenerator(msdf, fontdir, inputdir)

    licensedata = '# Third party licenses\n\n'
    licensedata += 'All third party licenses are included below as well as in the licenses directory.\n\n'

    for item in bundle['licenses']:
        # copy the license file to <bundle_dir>/licenses
        itempath = os.path.join(inputdir, item['path'])
        targetpath = os.path.join(licensedir, item['id'].upper() + '.LICENSE.md')
        copyfile(itempath, targetpath)
        licensedata += f'## {item["name"]}\n\n'
        licensedata += f'Project url: {item["url"]}\n\n'
        licensedata += open(itempath, "r", encoding='utf8').read()
        licensedata += '\n'

    open(os.path.join(bundle_dir, "LICENSES.md"), "w", encoding='utf8').write(licensedata)

    for item in bundle['fonts']:
        # copy the font file to <bundle_dir>/fonts
        fonts = atlasgen.get_font_files(item)
        for font in fonts:
            outpath_ttf = os.path.join(fontdir, os.path.basename(font))
            copyfile(font, outpath_ttf)

        print(f"generating msdf atlas for {item['name']}")
        result = atlasgen.build_atlas(item)
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
        tar.add(bundle_dir, arcname=os.path.basename(bundle_dir))

if __name__ == "__main__":
    main()
