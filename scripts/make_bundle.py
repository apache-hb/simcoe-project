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
args = argparser.parse_args()

bundlefile = args.desc
inputdir = args.dir
outputdir = args.outdir
outputfile = args.bundle
depfile = args.depfile

bundle_dir = os.path.join(outputdir, 'bundle')
redist_dir = os.path.join(outputdir, 'redist')

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

class FileCache:
    def __init__(self, path):
        self.path = path
        if os.path.exists(path):
            log.info(f"loading cache from {path}")
            self.cache = json.load(open(path, "r"))
        else:
            log.info(f"cache file {path} does not exist, creating a new one")
            self.cache = {}
        # dict[str, (str, int)]
        # where the key is the relative file path
        # and the value is a tuple of the hash and the last modified time.
        # if the last modified times dont match, then the hash is compared
        # if that is also different, then the file is considered dirty

    def write_cache(self):
        log.info(f"saving cache to {self.path}")
        json.dump(self.cache, open(self.path, "w"))

    def update_entry(self, path, hash, lasttime):
        if path not in self.cache:
            self.cache[path] = (hash, lasttime)
        else:
            oldhash, _ = self.cache[path]
            self.cache[path] = (hash if hash is not None else oldhash, lasttime)

    def is_file_dirty(self, path):
        entry = self.cache.get(path)
        if entry is None:
            self.update_entry(path, None, os.path.getmtime(path))
            return True

        lasttime = os.path.getmtime(path)
        if lasttime == entry[1]:
            return False

        self.update_entry(path, None, lasttime)

        filehash = None
        if entry[0] is not None:
            filehash = hash_file(path)
            if filehash == entry[0]:
                return False

        # only calculate the hash if we really need to
        filehash = hash_file(path) if filehash is None else filehash
        self.update_entry(path, filehash, lasttime)
        return True

    def any_dirty(self, paths):
        return any(self.is_file_dirty(path) for path in paths)

cache = FileCache(pj(outputdir, 'cache.json'))

def tool_exit(code):
    if code == 0:
        return

    deps.write()
    cache.write_cache()
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
    log.info(f'executing: {command}')
    with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT) as proc:
        proc.communicate(timeout=15)
        log.info(f'exit code: {proc.returncode}')
        return proc.returncode

# https://stackoverflow.com/questions/5920643/add-an-item-between-each-item-already-in-the-list
def intersperse(lst, item):
    result = [item] * (len(lst) * 2 - 1)
    result[0::2] = lst
    return result

def copyfile(src, dst):
    deps.add(src)
    if not cache.is_file_dirty(src):
        return

    log.info(f"copying {src} to {dst}\n")
    shutil.copy(src, dst)

def copy_redist(outdir, indir, redists, debug_redists):
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

        self.args = [ self.dxc, '/WX', '/Ges' ]
        if self.debug:
            self.args += [ '/Zi', '/Fd', self.pdb_dir + '\\' ]

    # file is the input file
    # name is the output name
    # model is the shader model to use
    # target is the shader target to compile for
    # entrypoint is the entry point to use
    def compile_hlsl(self, name, file, model, target, entrypoint):
        if not cache.is_file_dirty(file):
            return 0

        output = os.path.join(self.target_dir, f'{name}.{target}.cso')
        extra_args = [ '-T' + target + '_' + model, '-E' + entrypoint, '-Fo' + output ]

        return run_command([ self.args, extra_args, file ])

class AtlasGenerator:
    def __init__(self, target_dir, input_dir):
        self.atlasgen = config.get_tool('msdf-atlas-gen')
        self.target_dir = target_dir
        self.input_dir = input_dir
        self.args = [ self.atlasgen, '-format', 'png' ]

    def build_atlas(self, options):
        files = self.get_font_files(options)
        if not cache.any_dirty(files):
            return 0

        em_size = options.get('em_size', 64)
        atlas_path = os.path.join(self.target_dir, options['name'])
        cmd = [
            self.args,
            intersperse([ f'-font {it}' for it in files ], '-and'),
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

def copy_redist_files(outdir, config):
    agility = config.get_redist('agility')
    warp = config.get_redist('warp')
    dxcompiler = config.get_redist('dxcompiler')
    winpixruntime = config.get_redist('winpixruntime')
    dstorage = config.get_redist('dstorage')
    os.makedirs(outdir, exist_ok=True)

    copy_redist(pj(outdir, 'd3d12'), agility,
        [ 'D3D12Core.dll' ],
        [ 'D3D12Core.pdb', 'D3D12SDKLayers.dll', 'd3d12SDKLayers.pdb' ]
    )

    copy_redist(outdir, dxcompiler,
        [ 'dxcompiler.dll', 'dxil.dll' ],
        [ 'dxcompiler.pdb', 'dxil.pdb' ]
    )

    copy_redist(outdir, warp,
        [ ],
        [ 'd3d10warp.dll', 'd3d10warp.pdb' ]
    )

    copy_redist(outdir, winpixruntime,
        [ ],
        [ 'WinPixEventRuntime.dll' ]
    )

    copy_redist(pj(outdir, 'dstorage'), dstorage,
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

    hlsl = ShaderCompiler(shaderdir, pdbdir, args.debug)
    atlasgen = AtlasGenerator(fontdir, inputdir)

    licensedata = '# Third party licenses\n\n'
    licensedata += 'All third party licenses are included below as well as in the licenses directory.\n\n'

    license_dirty = False

    for item in bundle['licenses']:
        # copy the license file to <bundle_dir>/licenses
        itempath = os.path.join(inputdir, item['path'])
        if cache.is_file_dirty(itempath):
            license_dirty = True

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
            result = hlsl.compile_hlsl(name, itempath, '6_0', target['target'], target['entry'])
            tool_exit(result)

    compressor = config.get_tool('compressonator')
    for texture in bundle['textures']:
        itempath = os.path.join(inputdir, texture['path'])
        if not cache.is_file_dirty(itempath):
            continue
        mips = texture.get('mips', 1)

        deps.add(itempath)

        outpath = os.path.join(texturedir, texture['name'] + '.dds')
        result = run_command([compressor, '-fd', 'BC7', itempath, outpath, '-miplevels', mips])
        tool_exit(result)

    deps.write()
    cache.write_cache()

    with tarfile.open(outputfile, "w", format=tarfile.USTAR_FORMAT) as tar:
        tar.add(bundle_dir, arcname=os.path.basename(bundle_dir))

if __name__ == "__main__":
    main()
