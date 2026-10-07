#!/usr/bin/env python3
"""Full production driver GX/PD input regression with bounded host RDRAM.

Usage: python tests/test_goldeneye_x_input.py GX.z64 PD-USA-Rev1.z64
Optional: --baseline-source /path/to/old/games/perfectdark.c

The real GE, PD and GAME driver implementations execute on host C. Only emulator
memory access is replaced. Decoded ROM code/data are authentic; player/menu state
is a synthetic fixture. This does not run game frames or test Windows devices.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[1]

MEMORY = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
extern uint32_t *test_rom, test_ram[0x800000 / 4];
extern size_t test_rom_bytes;
extern unsigned int test_forbid_writes, test_writes, test_reads;
#define WITHINRANGE(at) (rdramptr && (((at) & 0xFF800000U) == 0x80000000U))
static int test_mapped(unsigned int at) {
    return WITHINRANGE(at) && rdramptr[at >> 12];
}
static unsigned int EMU_ReadROM(unsigned int at) {
    assert(!(at & 3) && at + 4 <= test_rom_bytes); return test_rom[at / 4];
}
static void EMU_WriteROM(unsigned int at, unsigned int v) {
    assert(!test_forbid_writes && !(at & 3) && at + 4 <= test_rom_bytes);
    test_writes++; test_rom[at / 4] = v;
}
static int EMU_ReadInt(unsigned int at) {
    test_reads++;
    return !(at & 3) && test_mapped(at) ? (int)test_ram[(at & 0x7FFFFF) / 4] : (int)0xABADC0DE;
}
static short EMU_ReadShort(unsigned int at) {
    if((at & 1) || !test_mapped(at)) return (short)0xABAD;
    return (short)((unsigned int)EMU_ReadInt(at & ~3U) >> ((at & 2) ? 0 : 16));
}
static void EMU_WriteInt(unsigned int at, int value) {
    assert(!test_forbid_writes && !(at & 3) && test_mapped(at));
    test_writes++; test_ram[(at & 0x7FFFFF) / 4] = (unsigned int)value;
}
static float EMU_ReadFloat(unsigned int at) {
    union { uint32_t u; float f; } value; value.u = (uint32_t)EMU_ReadInt(at); return value.f;
}
static void EMU_WriteFloat(unsigned int at, float f) {
    union { uint32_t u; float f; } value; value.f = f; EMU_WriteInt(at, value.u);
}
'''

HARNESS = r'''
uint32_t *test_rom, test_ram[0x800000 / 4];
static uint32_t original_ram[0x800000 / 4];
size_t test_rom_bytes;
unsigned int test_forbid_writes, test_writes, test_reads;
BUTTONS CONTROLLER[4];
struct PROFILE_STRUCT PROFILE[4];
struct DEVICE_STRUCT DEVICE[4];
const unsigned char **rdramptr, **romptr;
static const unsigned char *test_pages[0x100000];
int mousetoggle = 1, emuoverclock = 1, overridefov = 90;
int overrideratiowidth = 16, overrideratioheight = 9, geshowcrosshair;
int bypassviewmodelfovtweak = 1;

static size_t load_words(const char *path, uint32_t *destination, size_t limit) {
    FILE *file = fopen(path, "rb"); assert(file);
    fseek(file, 0, SEEK_END); size_t bytes = (size_t)ftell(file); rewind(file);
    assert(bytes <= limit && !(bytes & 3));
    for(size_t i = 0; i < bytes / 4; i++) {
        unsigned char word[4]; assert(fread(word, 1, 4, file) == 4);
        destination[i] = (uint32_t)word[0] << 24 | (uint32_t)word[1] << 16 | (uint32_t)word[2] << 8 | word[3];
    }
    fclose(file); return bytes;
}
static void reset(void) {
    GAME_Quit(); assert(!GAME_Name() && !pdcompat.valid && !pdlastprobe && !pdhookstate);
    test_forbid_writes = 0; test_writes = test_reads = 0;
    memcpy(test_ram, original_ram, sizeof(test_ram));
    memset(test_pages, 0, sizeof(test_pages));
    for(unsigned int page = 0x80000; page < 0x80800; page++)
        test_pages[page] = (const unsigned char *)test_ram + (page - 0x80000) * 0x1000;
    rdramptr = test_pages;
    memset(PROFILE, 0, sizeof(PROFILE)); memset(DEVICE, 0, sizeof(DEVICE));
    memset(CONTROLLER, 0, sizeof(CONTROLLER));
    for(int player = 0; player < 4; player++) {
        EMU_WriteInt(0x80070750 + player * 4, 0);
        EMU_WriteInt(0x8009A024 + player * 4, 0);
    }
    EMU_WriteInt(0x8009A26C, 0); EMU_WriteInt(0x80084014, 0);
    EMU_WriteInt(0x800ACBA4, 0); EMU_WriteInt(0x800624E4, 1);
    test_writes = test_reads = 0;
}
static void load_fixture(const char *rom, const char *ram) {
    GAME_Quit(); romptr = NULL;
    free(test_rom); test_rom = calloc(1, 64 * 1024 * 1024); assert(test_rom);
    test_rom_bytes = load_words(rom, test_rom, 64 * 1024 * 1024);
    assert(load_words(ram, original_ram, sizeof(original_ram)) == sizeof(original_ram));
    romptr = (const unsigned char **)test_rom; reset();
}
static void selected(void) {
    assert(GAME_Status()); assert(GAME_Name() && !strcmp(GAME_Name(), "Perfect Dark"));
    assert(pdcompat.valid && pdcompat.camera == 0x8009A26C && pdcompat.players == 0x8009A024);
    assert(pdcompat.menu == 0x80070750 && pdcompat.pause == 0x80084014);
}
static void controls(int count) {
    for(int player = 0; player < 4; player++) {
        const unsigned int base = 0x80500000 + player * 0x8000;
        memset(test_ram + (base & 0x7FFFFF) / 4, 0, 0x8000);
        EMU_WriteInt(pdcompat.players + player * 4, base);
        EMU_WriteFloat(base + PD_camx, 45.f); EMU_WriteFloat(base + PD_camy, 0.f);
        EMU_WriteFloat(base + PD_fov, 60.f);
        PROFILE[player].SETTINGS[CONFIG] = player < count ? WASD : DISABLED;
        PROFILE[player].SETTINGS[SENSITIVITY] = 40;
    }
}
static void press(int count) {
    for(int player = 0; player < count; player++) {
        DEVICE[player].XPOS = (player + 1) * 10; DEVICE[player].YPOS = (player + 1) * 5;
        const int buttons[] = {FORWARDS, STRAFELEFT, FIRE, AIM, RELOAD, ACCEPT, CANCEL, START, D_UP, L_SHOULDER, R_SHOULDER};
        for(unsigned int i = 0; i < sizeof(buttons) / sizeof(buttons[0]); i++)
            DEVICE[player].BUTTONPRIM[buttons[i]] = 1;
    }
}
static void button_assert(int count, int pressed) {
    for(int player = 0; player < 4; player++) {
        const int expected = player < count && pressed;
        assert(CONTROLLER[player].U_CBUTTON == expected && CONTROLLER[player].L_CBUTTON == expected);
        assert(CONTROLLER[player].Z_TRIG == expected && CONTROLLER[player].R_TRIG == expected);
        assert(CONTROLLER[player].A_BUTTON == expected && CONTROLLER[player].B_BUTTON == expected);
        assert(CONTROLLER[player].START_BUTTON == expected && CONTROLLER[player].U_DPAD == expected);
        assert(CONTROLLER[player].L_TRIG == expected);
#ifndef SPEEDRUN_BUILD
        assert(CONTROLLER[player].RELOAD_HACK == expected);
#else
        assert(!CONTROLLER[player].RELOAD_HACK);
#endif
    }
}
static void menu_and_gameplay(int count) {
    selected(); controls(count); press(count);
    EMU_WriteInt(pdcompat.camera, 0);
    for(int player = 0; player < 4; player++) EMU_WriteInt(pdcompat.menu + player * 4, 0);
    test_forbid_writes = 1; GAME_Inject(); test_forbid_writes = 0;
    button_assert(count, 1);
    memset(DEVICE, 0, sizeof(DEVICE));
    test_forbid_writes = 1; GAME_Inject(); test_forbid_writes = 0;
    button_assert(count, 0);
    EMU_WriteInt(pdcompat.camera, 1);
    for(int player = 0; player < 4; player++) EMU_WriteInt(pdcompat.menu + player * 4, 1);
    press(count); GAME_Inject(); button_assert(count, 1);
    for(int player = 0; player < 4; player++) {
        const unsigned int base = 0x80500000 + player * 0x8000;
        const float movement = player < count ? (float)(player + 1) : 0.f;
        assert(EMU_ReadFloat(base + PD_camx) == 45.f + movement);
        assert(EMU_ReadFloat(base + PD_camy) == -movement * 0.5f);
    }
    memset(DEVICE, 0, sizeof(DEVICE)); GAME_Inject(); button_assert(count, 0);
    /* Pause and death must retain buttons but suppress camera writes. */
    press(count); EMU_WriteInt(pdcompat.pause, 1);
    test_forbid_writes = 1; GAME_Inject(); test_forbid_writes = 0; button_assert(count, 1);
    EMU_WriteInt(pdcompat.pause, 0);
    for(int player = 0; player < count; player++) EMU_WriteInt(0x80500000 + player * 0x8000 + PD_deathflag, 1);
    test_forbid_writes = 1; GAME_Inject(); test_forbid_writes = 0; button_assert(count, 1);
    memset(DEVICE, 0, sizeof(DEVICE)); GAME_Inject(); button_assert(count, 0);
}
static void boot_and_reopen(void) {
    for(int cycle = 0; cycle < 2; cycle++) {
        reset(); selected();
        for(unsigned int i = 0; i < PDS_COUNT; i++) assert(pdcompat.matches[i] == pd_signatures[i].retail);
        EMU_WriteInt(pdcompat.stage, 0); test_writes = 0;
        GAME_Inject(); assert(test_writes && pdhookstate);
        assert((unsigned int)EMU_ReadInt(0x802C07B8) == 0x0BC69E62);
#ifndef SPEEDRUN_BUILD
        assert((unsigned int)EMU_ReadInt(PD_controlstyle) == 0x34020001);
        assert((unsigned int)EMU_ReadInt(PD_defaultfov) == 0x3C0142B4);
#endif
        EMU_WriteInt(pdcompat.stage, 1);
        menu_and_gameplay(1); menu_and_gameplay(4);
        GAME_Quit(); assert(!GAME_Name() && !pdcompat.valid && !pdlastprobe && !pdhookstate);
        for(int player = 0; player < 4; player++) assert(!playerbase[player]);
    }
}
static void rejected(void) {
    test_writes = 0; test_forbid_writes = 1;
    assert(!GAME_Status()); assert(!GAME_Name()); GAME_Inject();
    assert(test_writes == 0); test_forbid_writes = 0;
}
static void malformed(void) {
    const unsigned int core[] = {PDS_CAMERA, PDS_MENU, PDS_PAUSE, PDS_TITLE, PDS_MP};
    for(unsigned int i = 0; i < sizeof(core) / sizeof(core[0]); i++) {
        unsigned int at = pd_signatures[core[i]].retail;
        reset(); EMU_WriteInt(at, 0); rejected();
        reset(); memcpy(test_ram + 0x600000 / 4, test_ram + (at & 0x7FFFFF) / 4, pd_signatures[core[i]].count * 4); rejected();
    }
    reset(); memset(test_ram, 0, sizeof(test_ram)); rejected();
    reset(); test_pages[0x8009A] = NULL; rejected();
    reset(); unsigned int at = pd_signatures[PDS_CAMERA].retail;
    EMU_WriteInt(at, (EMU_ReadInt(at) & 0xFFFF0000U) | 0x7000);
    EMU_WriteInt(at + 4, EMU_ReadInt(at + 4) & 0xFFFF0000U); rejected();
    reset(); EMU_WriteInt(0x8009A26C, 8); rejected();
    reset(); EMU_WriteInt(0x80070750, 2); rejected();
    reset(); EMU_WriteInt(0x80084014, 2); rejected();
    /* A renamed unrelated game must not trigger a full PD RAM scan. */
    reset(); unsigned int header[3] = {EMU_ReadROM(0x20), EMU_ReadROM(0x24), EMU_ReadROM(0x28)};
    unsigned int cart = EMU_ReadROM(0x3C);
    EMU_WriteROM(0x20, 0x556E7265); EMU_WriteROM(0x24, 0x6C617465); EMU_WriteROM(0x28, 0x64202020);
    EMU_WriteROM(0x3C, 0x58584500); test_reads = 0;
    assert(!PD_ResolveCompatibility() && test_reads == 0); rejected();
    for(unsigned int i = 0; i < 3; i++) EMU_WriteROM(0x20 + i * 4, header[i]);
    EMU_WriteROM(0x3C, cart); reset(); selected();
}
int main(int argc, char **argv) {
    assert(argc == 6);
    load_fixture(argv[1], argv[2]);
    if(!strcmp(argv[5], "baseline")) {
        rejected(); assert(!pdcompat.valid && !pdlastprobe);
        PROFILE[0].SETTINGS[CONFIG] = WASD; DEVICE[0].BUTTONPRIM[START] = 1;
        GAME_Inject(); assert(!CONTROLLER[0].START_BUTTON);
        puts("BASELINE REPRODUCED: authentic GoldenEye X is rejected before PD discovery; Start is not routed");
    } else {
        boot_and_reopen(); malformed();
        puts("PASS GoldenEye X: real GAME/PD dispatch, two boot/reopen cycles, menu press/release, one/four-player mouse/buttons, pause/death guards, malformed/duplicate/missing/unmapped globals, unrelated-header no-scan guard");
    }
    load_fixture(argv[3], argv[4]); boot_and_reopen();
    puts("PASS Perfect Dark USA Rev1: same boot/reopen, menu, gameplay and input guards");
    GAME_Quit(); free(test_rom); return 0;
}
'''


def fixture(path):
    rom = path.read_bytes()
    lib = rom[0x1050:0x3050] + zlib.decompress(rom[0x3055:], -15)
    data = zlib.decompress(rom[0x39855:], -15)
    game = bytearray()
    for index in range(0, 0x4000, 4):
        offset = 0x4fc40 + int.from_bytes(rom[0x4fc40 + index:0x4fc44 + index], 'big') + 2
        if rom[offset:offset + 2] != b'\x11\x73':
            break
        part = zlib.decompress(rom[offset + 5:offset + 0x1000], -15)
        game.extend(part)
        if len(part) != 0x1000:
            break
    assert (len(lib), len(data), len(game)) == (0x58f90, 0x30e40, 0x1b99e0)
    ram = bytearray(0x800000)
    for at, part in [(0x1050, lib), (0x59fe0, data), (0x220000, game)]:
        ram[at:at + len(part)] = part
    return ram


def instrument(source):
    # Preserve every driver function, using the normal repository headers and a
    # bounded emulator interface. Separate translation units retain static scope.
    return source.replace('#include "memory.h"', '#include "fixture_memory.h"')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('gx', type=Path)
    parser.add_argument('pd', type=Path)
    parser.add_argument('--baseline-source', type=Path)
    args = parser.parse_args()
    args.gx, args.pd = args.gx.resolve(), args.pd.resolve()
    results = []
    with tempfile.TemporaryDirectory(prefix='gx-full-driver-') as work:
        work = Path(work)
        (work / 'fixture_memory.h').write_text(MEMORY)
        (work / 'ge.c').write_text(instrument((ROOT / 'games/goldeneye.c').read_text()))
        (work / 'gx-ram.bin').write_bytes(fixture(args.gx))
        (work / 'pd-ram.bin').write_bytes(fixture(args.pd))
        versions = [('candidate', ROOT / 'games/perfectdark.c')]
        if args.baseline_source:
            versions.insert(0, ('baseline', args.baseline_source.resolve()))
        for version, pd_source in versions:
            game = (ROOT / 'games/game.c').read_text().replace('#include "game.h"', '')
            (work / 'test.c').write_text(instrument(pd_source.read_text()) + '\n' + game + '\n' + HARNESS)
            for mode, defines in [('normal', []), ('speedrun', ['-DSPEEDRUN_BUILD'])]:
                binary = work / 'test'
                subprocess.run(['cc', '-std=c11', '-O2', '-Dinline=static inline', *defines,
                                '-I', str(work), '-I', str(ROOT / 'games'), str(work / 'ge.c'),
                                str(work / 'test.c'), '-lm', '-o', str(binary)], check=True)
                run = subprocess.run([str(binary), str(args.gx), str(work / 'gx-ram.bin'),
                                      str(args.pd), str(work / 'pd-ram.bin'), version],
                                     capture_output=True, text=True)
                if run.returncode:
                    raise RuntimeError(f'{version}/{mode} failed ({run.returncode}): {run.stdout}\n{run.stderr}')
                results.append({'version': version, 'mode': mode, 'result': run.stdout.strip(),
                                'perfectdark_c_sha256': hashlib.sha256(pd_source.read_bytes()).hexdigest()})
    print(json.dumps({'gx_rom_sha256': hashlib.sha256(args.gx.read_bytes()).hexdigest(),
                      'pd_rom_sha256': hashlib.sha256(args.pd.read_bytes()).hexdigest(),
                      'tests': results,
                      'limits': 'Production C drivers and dispatch with bounded synthetic RDRAM/player/input state and authentic decoded GX/PD code/data. No game frames or live Windows device input executed.'}, indent=2))


if __name__ == '__main__':
    main()
