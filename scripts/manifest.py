#!/usr/bin/python3

# apply an application manifest to an executable
# usage: manifest.py <manifest> <exe> <output>
# <manifest> is the manifest file
# <exe> is the input executable
# <output> is the output executable

#
# we have this script because the manifest compiler modifies the executable file
# meson *really* doesnt like files being modified in place
# so we copy the input exe to the output, apply the manifest to the output, and
# that way meson is happy
#

import sys
import os
import shutil

def main():
    if len(sys.argv) != 4:
        print("usage: manifest.py <manifest> <exe> <output>")
        sys.exit(1)

    manifest = sys.argv[1]
    exe = sys.argv[2]
    output = sys.argv[3]

    if not os.path.exists(manifest):
        print(f"error: {manifest} does not exist")
        sys.exit(1)

    if not os.path.exists(exe):
        print(f"error: {exe} does not exist")
        sys.exit(1)

    if os.path.exists(output):
        os.remove(output)

    # copy input exe to output
    shutil.copyfile(exe, output)

    err = os.system(f"mt.exe -nologo -manifest {manifest} -validate_manifest")
    if err != 0:
        print("error: manifest validation failed")
        sys.exit(1)

    err = os.system(f"mt.exe -nologo -manifest {manifest} -outputresource:{output};1")
    if err != 0:
        print("error: manifest application failed")
        sys.exit(1)

    sys.exit(0)

if __name__ == "__main__":
    main()
