#!/usr/bin/env python3
"""Execute the production GE resolver/patch functions with bounded host memory.

Usage: python tests/test_goldeneye_resolver.py retail.z64 plus.z64
Requires a native C compiler; does not execute a ROM or claim gameplay testing.
"""
from pathlib import Path
import subprocess
import tempfile
import sys

root = Path(__file__).resolve().parents[1]
source = (root / 'games/goldeneye.c').read_text()
source = source.replace('#include "../global.h"', '#include "' + str(root / 'global.h') + '"')
source = source.replace('#include "../maindll.h"', '#include "' + str(root / 'maindll.h') + '"')
source = source.replace('#include "game.h"', '#include "' + str(root / 'games/game.h') + '"')
source = source.replace('#include "memory.h"', r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
static uint32_t *test_rom;
static size_t test_rom_bytes;
static uint32_t test_ram[0x800000 / 4];
static unsigned int EMU_ReadROM(unsigned int at) { assert(!(at & 3) && at + 4 <= test_rom_bytes); return test_rom[at / 4]; }
static void EMU_WriteROM(unsigned int at, unsigned int v) { assert(!(at & 3) && at + 4 <= test_rom_bytes); test_rom[at / 4] = v; }
static int EMU_ReadInt(unsigned int at) { return ((at & 0xFF800003U) == 0x80000000U) ? (int)test_ram[(at & 0x7FFFFF) / 4] : (int)0xABADC0DE; }
static void EMU_WriteInt(unsigned int at, int v) { assert((at & 0xFF800003U) == 0x80000000U); test_ram[(at & 0x7FFFFF) / 4] = (unsigned int)v; }
static float EMU_ReadFloat(unsigned int at) { union { uint32_t u; float f; } v; v.u = (uint32_t)EMU_ReadInt(at); return v.f; }
static void EMU_WriteFloat(unsigned int at, float f) { union { uint32_t u; float f; } v; v.f = f; EMU_WriteInt(at, v.u); }
''')

harness = r'''
BUTTONS CONTROLLER[4];
struct PROFILE_STRUCT PROFILE[4];
struct DEVICE_STRUCT DEVICE[4];
const unsigned char **rdramptr, **romptr;
int emuoverclock, overridefov = 90, overrideratiowidth = 16, overrideratioheight = 9, geshowcrosshair, bypassviewmodelfovtweak;
static void loadrom(const char *name)
{
    GE_Quit(); romptr = 0;
    FILE *f = fopen(name, "rb"); assert(f);
    fseek(f, 0, SEEK_END); test_rom_bytes = (size_t)ftell(f); rewind(f);
    free(test_rom); test_rom = malloc(test_rom_bytes); assert(test_rom);
    for(size_t i = 0; i < test_rom_bytes / 4; i++)
    {
        unsigned char b[4]; assert(fread(b, 1, 4, f) == 4);
        test_rom[i] = (uint32_t)b[0] << 24 | (uint32_t)b[1] << 16 | (uint32_t)b[2] << 8 | b[3];
    }
    fclose(f); romptr = (const unsigned char **)test_rom; memset(test_ram, 0, sizeof(test_ram)); GE_Quit();
}
#ifndef SPEEDRUN_BUILD
static void duplicate_words(unsigned int from, unsigned int to, unsigned int count)
{
    assert(from + count * 4 <= test_rom_bytes && to + count * 4 <= test_rom_bytes);
    memcpy(test_rom + to / 4, test_rom + from / 4, count * 4);
}
#endif
static void assert_globals(const GE_ADDRESS_PROFILE *p, int plus)
{
    assert(p->menupage == (plus ? 0x8002A8F0U : 0x8002A8C0U));
    assert(p->bonddata == (plus ? 0x8007F410U : 0x80079EE0U));
    assert(p->camera == (plus ? 0x80036834U : 0x80036494U));
    assert(p->pause == (plus ? 0x80048810U : 0x80048370U));
    assert(p->matchended == (plus ? 0x80091D00U : 0x8008C700U));
    assert(p->exit == p->camera + 0x1C && p->tankflag == p->camera - 0x4C);
}
int main(int argc, char **argv)
{
    assert(argc == 3);
    for(int image = 1; image < argc; image++)
    {
        const int plus = image == 2;
        GE_ADDRESS_PROFILE addresses;
        GE_HACK_PROFILE *p;
        unsigned int allowed[80], allowedcount = 0;
        uint32_t *original, *savedram;
        loadrom(argv[image]);
        assert(GE_ResolveAddressProfile(&addresses));
        assert_globals(&addresses, plus);
        p = GE_GetHackProfile();
        assert(p->fov[0] == (plus ? 0xD54C0U : 0xB78BCU));
        assert(p->fov[1] == (plus ? 0xD54E0U : 0xB78DCU));
        assert(p->fov[2] == (plus ? 0xF0528U : 0xCF838U));
        assert(p->aimvalid);
        assert(p->controlstyle == (plus ? 0xFAFF4U : 0xD98FCU));
        assert(p->reversepitch == (plus ? 0U : 0xD9970U));
        assert(p->itemtable == (plus ? 0x80033CC0U : 0x80033924U));
        assert(p->defaultstats == (plus ? 0x80032830U : 0x80032494U));
        assert(p->zoomspeed == (plus ? 0x8004F658U : 0x8004F1A8U));
        assert(p->pickup == (plus ? 0x80055BE8U : 0x800532E0U));
        assert(p->showcrosshair == (plus ? 0U : 0x9F128U));
        original = malloc(test_rom_bytes); savedram = malloc(sizeof(test_ram));
        assert(original && savedram);
        memcpy(original, test_rom, test_rom_bytes);
        for(unsigned int i = 0; i < 27; i++) allowed[allowedcount++] = p->aimaddress[i];
#ifndef SPEEDRUN_BUILD
        {
            const GE_RELOAD_HACK_PROFILE *reload = GE_GetReloadHackProfile();
            assert((reload != 0) == !plus);
            if(reload) for(unsigned int i = 0; i < 23; i++) allowed[allowedcount++] = reload->patchaddress[i];
        }
        for(unsigned int i = 0; i < 3; i++) allowed[allowedcount++] = p->fov[i];
        allowed[allowedcount++] = p->controlstyle;
        if(p->reversepitch) allowed[allowedcount++] = p->reversepitch;
        if(p->ratiocrosshair) allowed[allowedcount++] = p->ratiocrosshair;
#endif
        if(p->showcrosshair) allowed[allowedcount++] = p->showcrosshair;
        if(!plus) allowed[allowedcount++] = GE_crosshairimage;
        geshowcrosshair = 1;
        overrideratiowidth = 21;
        /* Seed the exact data values which used to provoke boot-time writes. */
        EMU_WriteInt(p->pickup, 0xBF490FDB);
        EMU_WriteInt(p->zoomspeed, 0x3F68BA2E);
        if(p->ratio) EMU_WriteInt(p->ratio, 0x3FE38E39);
        CONTROLLER[0].Z_TRIG = CONTROLLER[0].R_TRIG = 1;
        memcpy(savedram, test_ram, sizeof(test_ram));
        GE_PrepareROM();
        assert(memcmp(savedram, test_ram, sizeof(test_ram)) == 0);
        for(size_t i = 0; i < test_rom_bytes / 4; i++)
        {
            if(test_rom[i] != original[i])
            {
                unsigned int found = 0;
                for(unsigned int j = 0; j < allowedcount; j++) if(allowed[j] == i * 4) found = 1;
                assert(found);
            }
        }
        for(unsigned int i = 0; i < 27; i++) assert(EMU_ReadROM(p->aimaddress[i]) == p->aimcode[i]);
#ifndef SPEEDRUN_BUILD
        for(unsigned int i = 0; i < 3; i++) assert(EMU_ReadROM(p->fov[i]) == 0x3C0142B4);
        assert(EMU_ReadROM(p->controlstyle) == 0x34020001);
        if(!plus)
        {
            const unsigned int expected[27] = {0,0,0x0BC1E66B,0x460C5100,0x0BC1E66F,0x460E3280,0x0BC1E673,0x460C4100,0x0BC1E677,0x460E5200,0x8C590124,0x53200001,0xE4440FF0,0x0BC19F34,0x8C590124,0x53200001,0xE44A0FF4,0x0BC19F39,0x8C590124,0x53200001,0xE4441004,0x0BC19F9C,0x8C590124,0x53200001,0xE4481008,0x0BC19FA1,0};
            assert(memcmp(expected, p->aimcode, sizeof(expected)) == 0);
        }
        else
        {
            /* Model Plus copying its code into 0x80600000 after HookROM. */
            memcpy(test_ram + 0x600000 / 4, test_rom + 0x34B30 / 4, 0x180000 - 0x34B30);
            for(unsigned int i = 0; i < 3; i++)
                assert((unsigned int)EMU_ReadInt(0x80600000 + p->fov[i] - 0x34B30) == 0x3C0142B4);
            for(unsigned int i = 0; i < 27; i++)
                assert((unsigned int)EMU_ReadInt(0x80600000 + p->aimaddress[i] - 0x34B30) == p->aimcode[i]);
        }
        /* Shared/non-contiguous stats records must each be adjusted once. */
        for(unsigned int item = 0; item < 33; item++)
        {
            const unsigned int entry = p->itemtable + item * 0x38;
            const unsigned int stats = p->defaultstats + (item / 2) * 0x80;
            EMU_WriteInt(entry + 8, item == 0 ? 1 : 0);
            EMU_WriteInt(entry + 12, stats);
            EMU_WriteFloat(stats + 4, 1.f);
            EMU_WriteFloat(stats + 8, 2.f);
            EMU_WriteFloat(stats + 12, 3.f);
        }
        CONTROLLER[0].Z_TRIG = CONTROLLER[0].R_TRIG = 0;
        GE_InjectHacks();
        for(unsigned int item = 0; item < 17; item++)
        {
            const unsigned int stats = p->defaultstats + item * 0x80;
            assert(fabsf(EMU_ReadFloat(stats + 8) - (2.f - 30.f / 9.f)) < 0.0001f);
            assert(fabsf(EMU_ReadFloat(stats + 12) - (3.f + 30.f / 2.75f)) < 0.0001f);
        }
        memcpy(savedram, test_ram, sizeof(test_ram));
        GE_InjectHacks();
        assert(memcmp(savedram, test_ram, sizeof(test_ram)) == 0);
#else
        for(unsigned int i = 0; i < 3; i++) assert(EMU_ReadROM(p->fov[i]) == 0x3C014270);
        assert(EMU_ReadROM(p->controlstyle) == 0x8DC22A58);
#endif
        /* Stop/Play must restore and rescan the same allocation without a
         * disk reload, including a changed setting for the next start. */
        {
            const unsigned char **sameallocation = romptr;
            GE_Quit();
            assert(memcmp(test_rom, original, test_rom_bytes) == 0);
            assert(romptr == sameallocation && ge_ownedcount == 0);
            overridefov = 100;
            p = GE_GetHackProfile();
            assert(p->aimvalid && p->fov[0] && p->fovword == 0x3C014270 && p->viewfov == 60);
            memset(test_ram, 0, sizeof(test_ram));
            GE_PrepareROM();
#ifndef SPEEDRUN_BUILD
            for(unsigned int i = 0; i < 3; i++) assert(EMU_ReadROM(p->fov[i]) == 0x3C0142C8);
#endif
            /* A third party's later edit is not ours to undo. */
            const unsigned int foreign = p->aimaddress[0];
            EMU_WriteROM(foreign, 0x12345678);
            GE_Quit();
            assert(EMU_ReadROM(foreign) == 0x12345678);
            for(size_t i = 0; i < test_rom_bytes / 4; i++)
                if(i * 4 != foreign) assert(test_rom[i] == original[i]);
            assert(ge_ownedcount == 0);
            overridefov = 90;
        }
        free(original); free(savedram);
#ifndef SPEEDRUN_BUILD
        /* Ambiguous FOV init is skipped independently of other features. */
        loadrom(argv[image]); p = GE_GetHackProfile();
        {
            GE_HACK_PROFILE test = {0};
            duplicate_words(p->fov[2], 0x1F0000, 15);
            GE_ResolveOptionalHacks(&test);
            assert(test.fov[0] == 0 && test.aimvalid && test.controlstyle);
        }
        /* Foreign target changes after resolution abort the entire FOV group. */
        loadrom(argv[image]); p = GE_GetHackProfile();
        EMU_WriteROM(p->fov[2], 0x12345678);
        GE_PrepareROM();
        assert(EMU_ReadROM(p->fov[0]) == 0x3C014270 && EMU_ReadROM(p->fov[1]) == 0x3C014270 && EMU_ReadROM(p->fov[2]) == 0x12345678);
        /* Corrupt/duplicated startup globals cannot trigger a retail fallback. */
        loadrom(argv[image]); EMU_WriteROM(0x34B30, 0);
        assert(!GE_ResolveAddressProfile(&addresses)); assert(!GE_Status());
        original = malloc(test_rom_bytes); assert(original); memcpy(original, test_rom, test_rom_bytes);
        GE_PrepareROM(); assert(!memcmp(original, test_rom, test_rom_bytes)); free(original);
        loadrom(argv[image]); duplicate_words(plus ? 0x116498 : 0xF25E8, 0x1E0000, 12);
        assert(!GE_ResolveAddressProfile(&addresses));
        loadrom(argv[image]); duplicate_words(plus ? 0x11BC90 : 0xF6B38, 0x1E0000, 7);
        assert(!GE_ResolveAddressProfile(&addresses));
        loadrom(argv[image]);
        EMU_WriteROM((plus ? 0x11BC90 : 0xF6B38) + 12, EMU_ReadROM((plus ? 0x11BC90 : 0xF6B38) + 12) + 4);
        assert(!GE_ResolveAddressProfile(&addresses));
        /* The aim trampoline map must agree with an independent startup call. */
        loadrom(argv[image]); p = GE_GetHackProfile();
        EMU_WriteROM(p->aimaddress[0], EMU_ReadROM(p->aimaddress[0]) + 1);
        EMU_WriteROM(p->aimaddress[1], EMU_ReadROM(p->aimaddress[1]) + 1);
        {
            GE_HACK_PROFILE test = {0}; GE_ResolveOptionalHacks(&test);
            assert(!test.aimvalid && test.fov[0]);
        }
        /* Missing aim code does not suppress a validated FOV override. */
        loadrom(argv[image]); p = GE_GetHackProfile();
        EMU_WriteROM(p->aimaddress[2], 0x12345678); GE_Quit();
        p = GE_GetHackProfile(); assert(!p->aimvalid && p->fov[0]);
        GE_PrepareROM(); assert(EMU_ReadROM(p->fov[0]) == 0x3C0142B4);
#endif
        printf("%s: production resolver, bounded patch writes, boot-only ROM preparation, lifecycle and feature guards passed (%s)\n", plus ? "Plus" : "retail",
#ifdef SPEEDRUN_BUILD
        "speedrun"
#else
        "normal"
#endif
        );
    }
    free(test_rom); return 0;
}

'''
with tempfile.TemporaryDirectory(prefix='ge-resolver-test-') as work:
    cpath = Path(work) / 'test.c'
    cpath.write_text(source + '\n' + harness)
    binary = Path(work) / 'test'
    for mode in ([], ['-DSPEEDRUN_BUILD']):
        subprocess.run(['cc','-std=c11','-O1','-fgnu89-inline','-Wall','-Wextra','-Wno-parentheses',*mode,str(cpath),'-lm','-o',str(binary)],check=True)
        subprocess.run([str(binary),*sys.argv[1:]],check=True)
