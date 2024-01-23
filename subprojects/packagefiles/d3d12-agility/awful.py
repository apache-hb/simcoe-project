#!/usr/bin/python3

# usage: awful.py <input> <output>
# <input> is the input file
# <output> is the output file

# awful.py is a hack to remove the throws from `d3dx12_property_format_table.cpp`
# so that it can be compiled without exceptions

from sys import argv
import os

def main():
    if len(argv) != 3:
        print('usage: awful.py <input> <output>')
        exit(1)

    src = argv[1]
    dst = argv[2]

    if not os.path.exists(src):
        print(f'error: {src} does not exist')
        exit(1)

    if os.path.exists(dst):
        os.remove(dst)

    with open(src, 'r') as f:
        text = f.read().replace('throw E_FAIL;', 'do { assert(false); __assume(0); } while(0);')

    with open(dst, 'w') as f:
        f.write(text)

if __name__ == '__main__':
    main()
