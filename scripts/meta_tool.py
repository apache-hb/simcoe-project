#!/usr/bin/python3

# parse a meson introspection file and run the metatool for a specific header file

import sys
import os
import json
import argparse
import subprocess

argparser = argparse.ArgumentParser(description="parse a meson introspection file and run the metatool for a specific header file")
argparser.add_argument("--builddir", required=True, help="the meson build directory")
argparser.add_argument("--reflect", required=True, help="the header file to generate the introspection data for")
argparser.add_argument("--target", required=True, help="the target to use for generating the command")
argparser.add_argument("--metatool", required=True, help="path to the metatool executable")
argparser.add_argument("--header", required=True, help="header output file")
argparser.add_argument("--source", required=True, help="source output file")
args = argparser.parse_args()

def get_target_data(builddir, target):
    intro = json.load(open(os.path.join(builddir, "meson-info", "intro-targets.json")))

    for t in intro:
        if t['name'] == target:
            return t

    raise Exception(f"target {target} not found")

def get_target_args(builddir, target):
    def is_used_arg(arg):
        return (
            arg.startswith("-I") or
            arg.startswith("-D") or
            arg.startswith("/I") or
            arg.startswith("/D") or
            arg.startswith("/clang:-isystem") or
            arg.startswith("/clang:-I"))

    def sanitize_arg(arg):
        if arg.startswith("/clang:-isystem"):
            return arg.replace("/clang:-isystem", "-isystem")

        if arg.startswith("/clang:-I"):
            return arg.replace("/clang:-I", "-I")

        return arg

    data = get_target_data(builddir, target)

    # TODO: i hope this [0] is right
    params: list[str] = data['target_sources'][0]['parameters']
    return [sanitize_arg(arg) for arg in params if is_used_arg(arg)]

def main():
    flags = get_target_args(args.builddir, args.target)
    command = [args.metatool, args.reflect, args.header, args.source, '--', *flags]
    sys.exit(subprocess.run(command).returncode)

if __name__ == "__main__":
    main()
