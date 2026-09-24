#!/usr/bin/env python3
"""Validate PD discovery against a user-supplied NTSC-final ROM and malformed/moved images.
Usage: python tests/test_pd_compat.py '/path/to/Perfect Dark (USA) (Rev 1).z64'
No ROM bytes are included in the source distribution.
"""
from pathlib import Path
import subprocess
import sys
import tempfile
import zlib

rom = Path(sys.argv[1]).read_bytes()
if rom[0x10:0x18] != bytes.fromhex('41f2b98fb458b466'):
    raise SystemExit('This regression fixture requires the original Perfect Dark USA Rev 1 ROM.')
lib = rom[0x1050:0x3050] + zlib.decompress(rom[0x3055:], -15)
data = zlib.decompress(rom[0x39855:], -15)
game = bytearray()
for index in range(0, 0x4000, 4):
    offset = 0x4fc40 + int.from_bytes(rom[0x4fc40+index:0x4fc44+index], 'big') + 2
    if rom[offset:offset+2] != b'\x11\x73':
        break
    part = zlib.decompress(rom[offset+5:offset+0x1000], -15)
    game.extend(part)
    if len(part) != 0x1000:
        break
assert len(lib) == 0x58f90 and len(data) == 0x30e40 and len(game) == 0x1b99e0
with tempfile.TemporaryDirectory(prefix='pd-compat-') as temp:
    temp = Path(temp)
    for name, content in [('lib',lib),('data',data),('game',game)]:
        (temp / name).write_bytes(content)
    executable = temp / 'pd_compat_test'
    subprocess.run(['cc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', str(Path(__file__).with_name('pd_compat_test.c')), '-o', str(executable)], check=True)
    subprocess.run([str(executable),str(temp/'lib'),str(temp/'data'),str(temp/'game')], check=True)
