#!/usr/bin/python3

# parse a meson introspection file and run the metatool for a specific header file

import sys
import os
import json
import argparse
import subprocess
import itertools
from jinja2 import Environment, FileSystemLoader

path = os.path.dirname(os.path.realpath(__file__))
env = Environment(loader=FileSystemLoader(f'{path}\\templates'))

argparser = argparse.ArgumentParser(description="parse a meson introspection file and run the metatool for a specific header file")
argparser.add_argument("--builddir", required=True, help="meson build directory")
argparser.add_argument("--sourcedir", required=True, help="current source directory")
argparser.add_argument("--reflect", required=True, help="header file to generate introspection data for")
argparser.add_argument("--metatool", required=True, help="path to metatool executable")
argparser.add_argument("--header", required=True, help="header output file")
argparser.add_argument("--source", required=True, help="source output file")
args = argparser.parse_args()

def get_target_data(builddir, sourcedir, source):
    intro = json.load(open(os.path.join(builddir, "meson-info", "intro-targets.json")))

    for t in intro:
        if t['defined_in'] != os.path.join(sourcedir, "meson.build"):
            continue

        generated_sources = []
        for lang in t['target_sources']:
            generated_sources += lang.get('generated_sources', [])

        generated_sources = [os.path.realpath(path) for path in generated_sources]

        if source in generated_sources:
            return t

    raise Exception(f"target generating {source} not found in {sourcedir}")

def get_target_args(builddir, sourcedir, source):
    def is_used_arg(arg):
        prefixes = ["-I", "-D", "/I", "/D", "/clang:-isystem", "/clang:-I"]
        return any(arg.startswith(prefix) for prefix in prefixes)

    def sanitize_arg(arg):
        prefixes = ["/clang:-isystem", "/clang:-I"]
        if any(arg.startswith(prefix) for prefix in prefixes):
            return arg.replace("/clang:", "")

        return arg

    data = get_target_data(builddir, sourcedir, source)
    cpp = [it['parameters'] for it in data['target_sources'] if it.get('language', '') == 'cpp']
    return [sanitize_arg(arg) for arg in itertools.chain(*cpp) if is_used_arg(arg)]

def main():
    sourcedir = os.path.realpath(args.sourcedir)
    source = os.path.realpath(args.source)
    flags = get_target_args(args.builddir, sourcedir, source)
    command = [args.metatool, args.reflect, '--', *flags]
    p = subprocess.run(command, capture_output=True, text=True)
    if p.returncode != 0:
        sys.exit(p.returncode)

    data = json.loads(p.stdout)
    header_template = env.get_template("header.meta.hpp.jinja2")
    source_template = env.get_template("source.meta.cpp.jinja2")

    header_content = header_template.render(data)
    source_content = source_template.render(data)

    with open(args.header, "w") as f:
        f.write(header_content)

    with open(args.source, "w") as f:
        f.write(source_content)

if __name__ == "__main__":
    main()
