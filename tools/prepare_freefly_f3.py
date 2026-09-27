#!/usr/bin/env python3
"""Materialize the reviewed F3 patch series before a source build.

The published DLL is the byte-identical user-tested binary. This preparation
step reproduces its changed translation units without changing compiler flags.
It stages and verifies every output before replacing any checkout file.
"""
from pathlib import Path
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = {
    'device.c': '77b91abb84075bf9829899f84c84f6c925e636823579d8854db9e804af9d26d4',
    'maindll.c': '2c1f3042811ef0bbaa7e78a8e13d52c9cdb131b2d152b537260c4400e0fe3b2f',
    'games/goldeneye.c': '07e09963232c029f930185a64379d3b474887b793020e599bb03b70f0a69e8d9',
    'games/goldeneye.mapmenu.h': 'b3d59bcbe494f245f608411c932ecd13240ad515df7c8ede3cadf9ae06b21573',
    'freefly_trace.h': '0f3eaa5c2c5ac38cf103f892020b122071ea57b5260daaf16a5f1e37a154cb41',
    'games/goldeneye.freeflytrace.h': 'e52250ad7fdf26147cd1ad4a748b36b13b69562181f98d95de79d618cba6ef73',
    'games/goldeneye.freeflyreport.h': 'c2df5f44ee61740ca0d39be613efb0937f9a2d97f8681e7ef3b3951eb8a1818c',
}

def matches(root: Path) -> bool:
    return all((root / name).is_file() and
        hashlib.sha256((root / name).read_bytes().replace(b'\r\n', b'\n')).hexdigest() == expected
        for name, expected in EXPECTED.items())

def main() -> None:
    if matches(ROOT):
        return
    # git apply is used only as a local patch reader; no fetch or push occurs.
    with tempfile.TemporaryDirectory(prefix='injector-f3-') as directory:
        stage = Path(directory)
        for name in ('device.c', 'maindll.c', 'games/goldeneye.c',
                     'games/goldeneye.mapmenu.h', 'tools/mapmaker-input-compat.patch',
                     'tools/freefly-trace.patch', 'tools/fix_freefly_precision.py'):
            target = stage / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes((ROOT / name).read_bytes().replace(b'\r\n', b'\n'))
        # A private staging repo prevents an unrelated parent repo affecting paths.
        subprocess.run(['git', 'init', '-q', str(stage)], check=True)
        for patch in ('mapmaker-input-compat.patch', 'freefly-trace.patch'):
            subprocess.run(['git', '-C', str(stage), 'apply', '--check', 'tools/' + patch], check=True)
            subprocess.run(['git', '-C', str(stage), 'apply', 'tools/' + patch], check=True)
        subprocess.run([sys.executable, str(stage / 'tools/fix_freefly_precision.py')], check=True)
        if not matches(stage):
            raise RuntimeError('Prepared source differs from approved F3; checkout not modified')
        for name in EXPECTED:
            target = ROOT / name
            target.parent.mkdir(parents=True, exist_ok=True)
            temporary = target.with_name(target.name + '.f3-tmp')
            shutil.copyfile(stage / name, temporary)
            os.replace(temporary, target)
    print('Prepared verified FreeFly-F3 source')

if __name__ == '__main__':
    try:
        main()
    except (OSError, subprocess.CalledProcessError, RuntimeError) as error:
        raise SystemExit(str(error))
