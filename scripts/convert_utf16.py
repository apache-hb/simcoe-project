#!/usr/bin/python3

# convert a utf8 file to utf16
# usage: convert_utf16.py <input> <output>
# <input> is the input utf8 file
# <output> is the output utf16 file

import sys
import os

def main():
    if len(sys.argv) != 3:
        print("usage: convert_utf16.py <input> <output>")
        sys.exit(1)

    input = sys.argv[1]
    output = sys.argv[2]

    if not os.path.exists(input):
        print(f"error: {input} does not exist")
        sys.exit(1)

    if os.path.exists(output):
        os.remove(output)

    with open(input, "r", encoding="utf-8") as f:
        utf8 = f.read()

    with open(output, "w", encoding="utf-16-le") as f:
        f.write(utf8)

    sys.exit(0)

if __name__ == "__main__":
    main()
