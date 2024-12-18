#!/usr/bin/python3

# generate exports for dstorage and agility sdk

import argparse
from jinja2 import Environment, FileSystemLoader

path = os.path.dirname(os.path.realpath(__file__))
env = Environment(loader=FileSystemLoader(f'{path}\\templates'))

argparser = argparse.ArgumentParser(description="generate export symbols for the agility sdk")

argparser.add_argument("--agility-version", required=True, help="version of the agility sdk")
argparser.add_argument("--agility-dlldir", required=True, help="directory containing the agility sdk dlls")
argparser.add_argument("--dstorage-dlldir", required=True, help="directory containing the dstorage sdk dlls")
argparser.add_argument("--output", required=True, help="output file")


def main():
    args = argparser.parse_args()

    file_template = env.get_template('exports.cpp.jinja2')
    data = {
        'agility': { 'version': args.agility_version, 'directory': args.agility_dlldir },
        'dstorage': { 'directory': args.dstorage_dlldir }
    }

    with open(args.output, 'w') as f:
        f.write(file_template.render(data))

if __name__ == '__main__':
    main()
