import os
import sys

n = len(sys.argv)

if n < 2:
    print("Usage: embed.py <filename> <output_dir>")
    sys.exit(1)

filename = sys.argv[1]
output_dir = sys.argv[2]

with open(filename, "rb") as f:
    print('reading file: ' + filename)
    data = f.read()
    print(f"read {len(data)} bytes")
    fileslug = os.path.basename(filename).replace('.', '_')
    hpath = f"{output_dir}/{fileslug}.h"
    cpath = f"{output_dir}/{fileslug}.c"
    print (f'writing {hpath} and {cpath}')
    with open(hpath, "w") as hfile, open(cpath, "w") as cfile:
        hfile.write("#ifndef __RESOURCE_" + fileslug.upper() + "_H__\n")
        hfile.write("#define __RESOURCE_" + fileslug.upper() + "_H__\n")
        hfile.write(f"extern unsigned const char {fileslug}_data[];\n")
        hfile.write(f"extern unsigned const int {fileslug}_size;\n")
        hfile.write("#endif\n")
        cfile.write(f"#include \"{fileslug}.h\"\n")
        cfile.write(f"unsigned const char {fileslug}_data[] = {{")
        for i in range(0, len(data)):
            cfile.write("0x%02x," % data[i])
        cfile.write("0x00 };\n")
        cfile.write(f"unsigned const int {fileslug}_size = {len(data)+1};\n")
