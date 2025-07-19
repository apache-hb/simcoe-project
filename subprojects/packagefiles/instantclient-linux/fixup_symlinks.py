#!/usr/bin/env python3

import os
import sys
from pathlib import Path

if __name__ == '__main__':
    link = sys.argv[1]
    target = sys.argv[2]

    os.remove(link)
    os.symlink(target, link)
    print(Path(os.path.basename(link)).stem[3:], end='', flush=True)
