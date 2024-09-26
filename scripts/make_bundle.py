#!/usr/bin/python3

# bundle assets into a folder

import sys
import os
import json
import shutil
import argparse
import tarfile
import configparser
import subprocess
import hashlib
from typing import Iterable

pj = os.path.join

argparser = argparse.ArgumentParser(description="bundle assets into a folder")
argparser.add_argument("--desc", help="the input bundle description file")
argparser.add_argument("--dir", help="the input directory")
argparser.add_argument("--outdir", help="the output directory")
argparser.add_argument("--bundle", help="output tar file")
argparser.add_argument("--depfile", help="the output dependency file")
argparser.add_argument("--config", help="config file with tool paths")
argparser.add_argument("--debug", help="enable debug mode", action="store_true")
argparser.add_argument("--root", help="source root directory")
args = argparser.parse_args()

bundlefile = args.desc
inputdir = args.dir
outputdir = args.outdir
outputfile = args.bundle
depfile = args.depfile
rootdir = args.root

bundle_dir = os.path.join(outputdir, 'bundle')

class Logger:
    def __init__(self, path):
        self.file = open(path, "w")

    def write(self, msg):
        print(msg)
        self.file.write(msg + '\n')
        self.file.flush()

    def split(self, level, msg):
        for line in msg.split('\n'):
            text = line.strip(' \t\r\n')
            if not text:
                continue

            self.write(f'{level}: {text}')

    def info(self, msg):
        self.split('info', msg)

log = Logger('bundle.log')

log.info(
f'''bundle description: {bundlefile}
input directory: {inputdir}
output directory: {outputdir}
output bundle: {outputfile}
dependency file: {depfile}
''')

class DepFile:
    def __init__(self, path):
        self.deps = list()
        self.seen = set()
        self.path = path

    def add(self, dep):
        if dep in self.seen:
            return
        self.seen.add(dep)
        self.deps.append(dep)

    def write(self):
        log.info(f"writing depfile to {self.path}")
        open(self.path, "w").write(outputdir + ": " + " ".join(self.deps))

deps = DepFile(depfile)

class Config:
    def __init__(self, path):
        self.config = configparser.ConfigParser()
        self.config.read(path)

    def get_tool(self, name):
        it = self.config['tools'][name]
        deps.add(it)
        return it

    def get_redist(self, name):
        return self.config['redist'][name]

config = Config(args.config)

def hash_file(path):
    with open(path, "rb") as file:
        return hashlib.file_digest(file, 'sha256').hexdigest()

def tool_exit(code):
    if code == 0:
        return

    deps.write()
    sys.exit(code)

def flatten(items):
    """Yield items from any nested iterable; see Reference."""
    for x in items:
        if isinstance(x, Iterable) and not isinstance(x, (str, bytes)):
            for sub_x in flatten(x):
                yield sub_x
        else:
            yield x

def run_command(cmd):
    # flatten the command list and convert everything to strings
    # makes this function more ergenomic to use
    command = [ str(each) for each in flatten(cmd) ]

    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if result.returncode != 0:
        print(result.stdout.decode('utf-8'))
        print(result.stderr.decode('utf-8'))
        log.info(f'exit code: {result.returncode}')
    return result.returncode

# https://stackoverflow.com/questions/5920643/add-an-item-between-each-item-already-in-the-list
def intersperse(lst, item):
    result = [item] * (len(lst) * 2 - 1)
    result[0::2] = lst
    return result

def copyfile(src, dst):
    deps.add(src)
    log.info(f"copying {src} to {dst}\n")
    shutil.copy(src, dst)

def copy_redist(outdir, name, redists, debug_redists):
    indir = config.get_redist(name)
    os.makedirs(outdir, exist_ok=True)
    for redist in redists:
        copyfile(pj(indir, redist), outdir)

    if not args.debug:
        return

    for redist in debug_redists:
        copyfile(pj(indir, redist), outdir)

class ShaderCompiler:
    # dxc is the path to dxc.exe
    # target_dir is the output directory
    # pdb_dir is the pdb directory
    # debug enables debug mode
    def __init__(self, target_dir, pdb_dir, debug):
        self.dxc = config.get_tool('dxc')
        self.target_dir = target_dir
        self.pdb_dir = pdb_dir
        self.debug = debug

        # TODO: this include path is a bit of a hack but meson is just so shit
        # at producing build directories with any rigid structure
        self.args = [ self.dxc, '/WX', '/Ges', '-I' + rootdir + '\\src\\common\\draw\\include\\draw' ]
        if self.debug:
            self.args += [ '/Zi', '/Fd', self.pdb_dir + '\\' ]

    # file is the input file
    # name is the output name
    # model is the shader model to use
    # target is the shader target to compile for
    # entrypoint is the entry point to use
    def compile_hlsl(self, name, file, model, target, entrypoint, enable_16bit = False, defines = []):
        output = os.path.join(self.target_dir, f'{name}.{target}.cso')
        extra_args = [
            '-T' + target + '_' + model,
            '-E' + entrypoint,
            '-Fo' + output
        ]

        if enable_16bit:
            extra_args += [ '-enable-16bit-types' ]

        for define in defines:
            extra_args += [ '-D', define ]

        return run_command([ self.args, extra_args, file ])

class AtlasGenerator:
    def __init__(self, target_dir, input_dir):
        self.atlasgen = config.get_tool('msdf-atlas-gen')
        self.target_dir = target_dir
        self.input_dir = input_dir
        self.args = [ self.atlasgen, '-format', 'png' ]

    def build_atlas(self, options):
        files = self.get_font_files(options)
        em_size = options.get('em_size', 64)
        atlas_path = os.path.join(self.target_dir, options['name'])
        cmd = [
            self.args,
            intersperse([ ['-font', it] for it in files ], '-and'),
            '-size', em_size,
            '-arfont', f'{atlas_path}.arfont',
            '-imageout', f'{atlas_path}.png'
        ]
        return run_command(cmd)

    # if `files` is not present in the options,
    # it is assumed that `path` is the file
    # otherwise `path` is the directory and each file in `files` is appended to it
    def get_font_files(self, options):
        path = os.path.join(self.input_dir, options['path'])
        if 'files' in options:
            return [ os.path.join(path, file) for file in options['files'] ]
        else:
            return [ path ]

def copy_redist_files():
    redist_dir = pj(outputdir, 'redist')
    os.makedirs(redist_dir, exist_ok=True)

    copy_redist(pj(redist_dir, 'd3d12'), 'agility',
        [ 'D3D12Core.dll' ],
        [ 'D3D12Core.pdb', 'D3D12SDKLayers.dll', 'd3d12SDKLayers.pdb' ]
    )

    copy_redist(redist_dir, 'dxcompiler',
        [ 'dxcompiler.dll', 'dxil.dll' ],
        [ ] # TODO: its a bit of a pain to get the pdb files now that they're a seperate package
        #[ 'dxcompiler.pdb', 'dxil.pdb' ]
    )

    copy_redist(redist_dir, 'warp',
        [ ],
        [ 'd3d10warp.dll', 'd3d10warp.pdb' ]
    )

    copy_redist(redist_dir, 'winpixruntime',
        [ ],
        [ 'WinPixEventRuntime.dll' ]
    )

    copy_redist(redist_dir, 'dstorage',
        [ 'dstorage.dll', 'dstoragecore.dll' ],
        [ ]
    )

def main():
    deps.add(bundlefile)
    if not os.path.exists(bundlefile):
        log.info(f"error: {bundlefile} does not exist")
        tool_exit(1)

    if not os.path.exists(inputdir):
        log.info(f"error: {inputdir} does not exist")
        tool_exit(1)

    # delete output directorys contents
    if os.path.exists(bundle_dir):
        # cant delete the dir itself because meson is watching it
        for root, dirs, files in os.walk(bundle_dir):
            for file in files:
                os.remove(os.path.join(root, file))
            for dir in dirs:
                shutil.rmtree(os.path.join(root, dir))

    copy_redist_files()

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

    hlsl = ShaderCompiler(shaderdir, pdbdir, args.debug)
    atlasgen = AtlasGenerator(fontdir, inputdir)

    licensedata = '# Third party licenses\n\n'
    licensedata += 'All third party licenses are included below as well as in the licenses directory.\n\n'

    license_dirty = False

    for item in bundle['licenses']:
        # copy the license file to <bundle_dir>/licenses
        itempath = os.path.join(inputdir, item['path'])
        targetpath = os.path.join(licensedir, item['id'].upper() + '.LICENSE.md')
        copyfile(itempath, targetpath)
        licensedata += f'## {item["name"]}\n\n'
        licensedata += f'Project url: {item["url"]}\n\n'
        licensedata += open(itempath, "r", encoding='utf8').read()
        licensedata += '\n'

    if license_dirty:
        open(os.path.join(bundle_dir, "LICENSES.md"), "w", encoding='utf8').write(licensedata)

    for item in bundle['fonts']:
        # copy the font file to <bundle_dir>/fonts
        fonts = atlasgen.get_font_files(item)
        for font in fonts:
            outpath_ttf = os.path.join(fontdir, os.path.basename(font))
            copyfile(font, outpath_ttf)

        result = atlasgen.build_atlas(item)
        tool_exit(result)

    for item in bundle['shaders']:
        name = item['name']
        path = item['path']

        itempath = os.path.join(inputdir, path)

        deps.add(itempath)

        for target in item['targets']:
            result = hlsl.compile_hlsl(
                name = name,
                file = itempath,
                model = target.get('model', '6_0'),
                target = target['target'],
                entrypoint = target['entry'],
                enable_16bit = target.get('16bit_types', False),
                defines = target.get('defines', []))
            tool_exit(result)

    compressor = config.get_tool('compressonator')
    for texture in bundle['textures']:
        itempath = os.path.join(inputdir, texture['path'])
        mips = texture.get('mips', 1)

        deps.add(itempath)

        outpath = os.path.join(texturedir, texture['name'] + '.dds')
        result = run_command([compressor, '-fd', 'BC7', itempath, outpath, '-miplevels', mips])
        tool_exit(result)

    deps.write()

    with tarfile.open(outputfile, "w", format=tarfile.USTAR_FORMAT) as tar:
        tar.add(bundle_dir, arcname=os.path.basename(bundle_dir))

if __name__ == "__main__":
    main()
