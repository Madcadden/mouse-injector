#!/usr/bin/env python3
"""Cross-build the unmodified 32-bit Mouse Injector project with Zig 0.13.0.

This changes no source files. Pass --source, --zig and --output explicitly.
The resulting DLL is a release candidate pending Windows/emulator testing.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--zig', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--pd-decomp', action='store_true')
    parser.add_argument('--speedrun', action='store_true')
    args = parser.parse_args()
    source, zig, output = args.source.resolve(), args.zig.resolve(), args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    sources = ['maindll.c', 'device.c', 'discord.c', 'manymouse/manymouse.c',
               'manymouse/windows_wminput.c', 'games/game.c', 'games/goldeneye.c',
               'games/perfectdark.c']
    tracked = sorted(p for p in source.rglob('*') if p.is_file() and
                     p.suffix.lower() in {'.c', '.h', '.rc'})
    snapshot = {p.relative_to(source).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in tracked}
    commands = []
    log = output / 'build.log'
    flags = ['-target', 'x86-windows-gnu', '-std=c11', '-O2', '-fno-strict-aliasing', f'-DPD_DECOMP={int(args.pd_decomp)}',
             '-Wall', '-Wextra', '-Wno-parentheses', '-Wno-strict-aliasing',
             '-Wno-unused-parameter']
    if args.speedrun:
        flags.append('-DSPEEDRUN_BUILD')
    with log.open('w') as stream:
        def run(cmd):
            commands.append([str(x) for x in cmd])
            stream.write('\n$ ' + subprocess.list2cmdline([str(x) for x in cmd]) + '\n')
            stream.flush()
            subprocess.run(cmd, cwd=source, stdout=stream, stderr=stream, check=True)
        objects = []
        for name in sources:
            obj = output / (name.replace('/', '_') + '.obj')
            run([zig, 'cc', *flags, '-c', name, '-o', obj])
            objects.append(obj)
        resource = output / 'ui.res'
        rc_source = 'ui/ui.perfectdark.rc' if args.pd_decomp else 'ui/ui.rc'
        run([zig, 'rc', '/i', zig.parent / 'lib/include', '/fo', resource,
             '/:auto-includes', 'gnu', rc_source])
        name = ('Mouse_Injector_pddecomp.dll' if args.pd_decomp else
                'Mouse_Injector_Speedrun.dll' if args.speedrun else 'Mouse_Injector.dll')
        dll = output / name
        run([zig, 'cc', '-target', 'x86-windows-gnu', '-shared', *objects, resource,
             '-o', dll, '-s', '-luser32', '-lgdi32', '-lcomctl32'])
    after = {p.relative_to(source).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
             for p in tracked}
    if after != snapshot:
        raise RuntimeError('Source changed while building; rebuild to obtain a consistent candidate')
    manifest = {'toolchain': subprocess.check_output([zig, 'version'], text=True).strip(),
                'target': 'x86-windows-gnu', 'sources_sha256': snapshot,
                'commands': commands, 'output_sha256': hashlib.sha256(dll.read_bytes()).hexdigest(),
                'output_bytes': dll.stat().st_size,
                'validation': 'Build only; requires Windows and emulator gameplay validation.'}
    (output / 'build-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    print(json.dumps({'dll': str(dll), 'sha256': manifest['output_sha256'],
                      'bytes': manifest['output_bytes'], 'manifest': str(output / 'build-manifest.json')}))


if __name__ == '__main__':
    main()
