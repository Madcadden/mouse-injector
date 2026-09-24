#!/usr/bin/env python3
"""Compile the actual INI functions and verify old/new profile round trips.

Only the Windows UI/device edges are stubbed; the parser, save path, reset and
preset logic are extracted unchanged from maindll.c. No ROM is needed.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'maindll.c').read_text()
start = source.index('static void INI_Load(const HWND hW, const int loadplayer)\n{')
end = source.index('static void UpdateControllerStatus(void)\n{', start)
functions = source[start:end]
header = (ROOT / 'global.h').read_text().split('// plugin spec')[0]
vkeys = (ROOT / 'vkey.h').read_text().split('struct VKeyInfo')[0]
harness = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#define _WIN32_WINNT 0x0600

typedef void *HWND;
#define MB_ICONERROR 0
#define MB_OK 0
'''+header+vkeys+r'''
static const char *inifilepathdefault;
static const wchar_t inifilepath[] = L"";
static int messages;
static int defaultmouse = 0, defaultkeyboard = 1, lastinputbutton;
static struct PROFILE_STRUCT PROFILE[ALLPLAYERS];
static int overridefov=60, overrideratiowidth=16, overrideratioheight=9;
static int geshowcrosshair, bypassviewmodelfovtweak, mouselockonfocus;
static int mouseunlockonloss=1, mousetogglekey=52;
static void MessageBoxA(HWND w, const char *m, const char *t, int flags) {
    (void)w; (void)m; (void)t; (void)flags; messages++;
}
static FILE *_wfopen(const wchar_t *p, const wchar_t *mode) {
    (void)p; (void)mode; return NULL;
}
static const char *DEV_Name(int device) { return device < 2 ? "connected" : NULL; }
static int DEV_Type(int device) { return device == 0 ? MOUSETYPE : KEYBOARDTYPE; }
static void INI_Save(HWND);
static void INI_Reset(int);
static void INI_SetConfig(int, int);
'''+functions+r'''
int main(int argc, char **argv) {
    if(argc != 4) return 2;
    INI_Reset(ALLPLAYERS);
    INI_SetConfig(PLAYER1, WASD);
    inifilepathdefault = argv[1];
    if(strcmp(argv[3], "default"))
        INI_Load(NULL, !strcmp(argv[3], "one") ? PLAYER2 : ALLPLAYERS);
    inifilepathdefault = argv[2];
    INI_Save(NULL);
    printf("%d\n", messages);
    return 0;
}
'''

GLOBALS = [97, 21, 9, 1, 1, 1, 0, 53]
SETTING_DEFAULTS = [0,20,0,3,0,0,1,1,0,1]
WASD = [87,83,65,68,1,2,82,81,69,13,17,0,10,11,38,40,37,39]
ESDF = [69,68,83,70,1,2,84,87,82,13,65,0,10,11,38,40,37,39]
DPAD = [ord(k) for k in 'IKJL']
EXTRA = DPAD + [ord('U'), ord('O')]


def fixture(buttons, configs=(3,3,3,3), debug=False):
    # Each player and binding bank is distinct, so offset shifts are visible.
    primary = [[120 + p*buttons+b for b in range(buttons)] for p in range(4)]
    secondary = [[35 + p*buttons+b for b in range(buttons)] for p in range(4)]
    settings = [[configs[p], 31+p, p, 5+p, p%2, 1, 0, 1, 0, 1] for p in range(4)]
    flat = sum(primary, []) + sum(secondary, []) + sum(settings, []) + GLOBALS
    if debug:
        flat += [1]
    return flat, primary, secondary, settings


def flattened(primary, secondary, settings, globals_):
    return sum(primary, []) + sum(secondary, []) + sum(settings, []) + globals_


with tempfile.TemporaryDirectory(prefix='ini-dpad-') as temporary:
    temp = Path(temporary)
    (temp / 'harness.c').write_text(harness)
    for decomp in (0,1):
        exe = temp / f'ini-{decomp}'
        subprocess.run(['cc','-std=c99','-fgnu89-inline','-O2','-Wall','-Wextra',
                        '-Werror','-Wno-parentheses',f'-DPD_DECOMP={decomp}',
                        '-I',str(ROOT),str(temp/'harness.c'),'-o',str(exe)],check=True)
        count = 24
        newdefaults = [0]*6 if decomp else EXTRA

        def run(values, mode='all'):
            ini = temp / 'input.ini'
            out = temp / 'output.ini'
            contents = '\n'.join(map(str, values))
            ini.write_text(contents)
            result = subprocess.run([str(exe),str(ini),str(out),mode],
                                    text=True,capture_output=True,check=True)
            assert result.stdout.strip() == '0', result.stdout
            assert ini.read_text() == contents, 'Loading unexpectedly rewrote the INI'
            result = list(map(int,out.read_text().splitlines()))
            assert len(result) == count*8+48
            return result

        # Explicitly cleared D-pad/L/R assignments and custom primary/secondary keys
        # survive save/reload, without being replaced with preset defaults.
        values,p,s,settings = fixture(count)
        p[0][18:] = [0]*(count-18)
        s[0][18:] = [0]*(count-18)
        values = flattened(p,s,settings,GLOBALS)
        assert run(values) == values
        assert run(run(values)) == values

        # Presets keep the pre-existing 18 defaults and append only their own
        # D-pad defaults. PD_DECOMP must retain its 24-button legacy behavior.
        values,p,s,settings = fixture(count, (1,2,0,3))
        p[0],p[1] = WASD+newdefaults, ESDF+newdefaults
        s[0],s[1] = [0]*count,[0]*count
        assert run(values) == flattened(p,s,settings,GLOBALS)

        # Revert one player cannot replace another player's controls or globals.
        values,p,s,settings = fixture(count)
        basep = [WASD+newdefaults] + [[0]*count for _ in range(3)]
        bases = [[0]*count for _ in range(4)]
        basesettings = [SETTING_DEFAULTS.copy() for _ in range(4)]
        basesettings[0][0] = 1
        basep[1],bases[1],basesettings[1] = p[1],s[1],settings[1]
        assert run(values, 'one') == flattened(basep,bases,basesettings,[60,16,9,0,0,0,1,52])

        if decomp:
            continue
        for debug in (False,True):
            # Both 192-line and 193-line variants must preserve all existing
            # settings, custom keys and global preferences using old offsets.
            values,p,s,settings = fixture(18, (3,0,1,2), debug)
            p[0][0] = ord('I')   # existing primary use prevents new I default
            s[0][1] = ord('K')   # existing secondary use prevents new K default
            values = flattened(p,s,settings,GLOBALS) + ([1] if debug else [])
            expectedp = [p[0]+[0,0,ord('J'),ord('L'),ord('U'),ord('O')], p[1]+[0]*6,
                         WASD+EXTRA, ESDF+EXTRA]
            expecteds = [s[0]+[0]*6, s[1]+[0]*6, [0]*24, [0]*24]
            expected = flattened(expectedp,expecteds,settings,GLOBALS)
            assert run(values) == expected
            assert run(run(values)) == expected

        # Typical all-custom profiles with free I/K/J/L/U/O gain the six usable
        # mappings, with no changes to their original 18 actions.
        values,p,s,settings = fixture(18)
        for player in range(4):
            s[player] = [40]*18
        values = flattened(p,s,settings,GLOBALS)
        expected = flattened([row+EXTRA for row in p], [row+[0]*6 for row in s], settings, GLOBALS)
        assert run(values) == expected

        # The last test build saved 224 lines. Keep its custom/cleared D-pad
        # controls exactly, add only missing L/R shoulders, and do not duplicate
        # existing U/O keys in either binding bank.
        values,p,s,settings = fixture(22)
        p[0][18:] = [0]*4
        s[0][18:] = [0]*4
        p[2][0] = ord('U')
        s[3][1] = ord('U')
        values = flattened(p,s,settings,GLOBALS)
        expectedp = [p[0]+[ord('U'),ord('O')], p[1]+[ord('U'),ord('O')],
                     p[2]+[0,0], p[3]+[0,ord('O')]]
        expected = flattened(expectedp,[row+[0,0] for row in s],settings,GLOBALS)
        assert run(values) == expected
        assert run(run(values)) == expected

        # Presets in the 22-button layout obtain U/O; disabled profiles remain
        # disabled and gain no binding.
        values,p,s,settings = fixture(22, (1,2,0,3))
        expectedp = [WASD+EXTRA, ESDF+EXTRA, p[2]+[0,0], p[3]+[ord('U'),ord('O')]]
        expecteds = [[0]*24,[0]*24,s[2]+[0,0],s[3]+[0,0]]
        assert run(values) == flattened(expectedp,expecteds,settings,GLOBALS)

        # Custom mouse-capture keys are global: newly added U/O (or D-pad keys
        # in a v0.1 profile) must not also trigger mouse capture.
        for oldcount in (18,22,23):
            for capture in (ord('U'), ord('O'), ord('I')):
                values,p,s,settings = fixture(oldcount)
                s = [[40]*oldcount for _ in range(4)]
                globals_ = GLOBALS[:-1] + [capture]
                values = flattened(p,s,settings,globals_)
                defaults = EXTRA[oldcount-18:]
                added = [0 if key == capture else key for key in defaults]
                expected = flattened([row+added for row in p],
                                     [row+[0]*len(added) for row in s], settings, globals_)
                assert run(values) == expected

        # A 232-line profile already has L: retain its cleared/custom primary
        # and secondary bindings, add only R, and protect existing O mappings.
        values,p,s,settings = fixture(23)
        s = [[40]*23 for _ in range(4)]
        p[0][22] = s[0][22] = 0
        p[1][22],s[1][22] = ord('V'),ord('X')
        p[2][0] = ord('O')
        s[3][1] = ord('O')
        values = flattened(p,s,settings,GLOBALS)
        expectedp = [p[0]+[ord('O')], p[1]+[ord('O')], p[2]+[0], p[3]+[0]]
        expected = flattened(expectedp,[row+[0] for row in s],settings,GLOBALS)
        assert run(values) == expected
        assert run(run(values)) == expected

print('INI controls tests passed: 192/193/224/232-line migration, 240-line normal/PD_DECOMP round trips, preset defaults, primary/secondary/capture conflicts, cleared keys, per-player revert, PD_DECOMP unchanged.')
