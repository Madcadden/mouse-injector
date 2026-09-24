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
source = source.replace('#include "goldeneye.menunav.h"', '#include "' + str(root / 'games/goldeneye.menunav.h') + '"')
source = source.replace('#include "goldeneye.mapmenu.h"', '#include "' + str(root / 'games/goldeneye.mapmenu.h') + '"')
source = source.replace('#include "memory.h"', r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
static uint32_t *test_rom;
static size_t test_rom_bytes;
static uint32_t test_ram[0x800000 / 4];
static unsigned int test_write_mode, test_menu_x, test_menu_y;
static unsigned int test_editor_yaw, test_editor_pitch, test_menu_selector;
static unsigned int EMU_ReadROM(unsigned int at) { assert(!(at & 3) && at + 4 <= test_rom_bytes); return test_rom[at / 4]; }
static void EMU_WriteROM(unsigned int at, unsigned int v) { assert(!(at & 3) && at + 4 <= test_rom_bytes); test_rom[at / 4] = v; }
static int EMU_ReadInt(unsigned int at) { return ((at & 0xFF800003U) == 0x80000000U) ? (int)test_ram[(at & 0x7FFFFF) / 4] : (int)0xABADC0DE; }
static void EMU_WriteInt(unsigned int at, int v) { assert((at & 0xFF800003U) == 0x80000000U); if(test_write_mode == 2) fprintf(stderr, "Unexpected RAM write: %08X = %08X\n", at, (unsigned int)v); assert(test_write_mode != 2); assert(test_write_mode != 1 || at == test_menu_x || at == test_menu_y); assert(test_write_mode != 3 || at == test_editor_yaw || at == test_editor_pitch); assert(test_write_mode != 4 || at == test_menu_x || at == test_menu_y || at == test_menu_selector); test_ram[(at & 0x7FFFFF) / 4] = (unsigned int)v; }
static float EMU_ReadFloat(unsigned int at) { union { uint32_t u; float f; } v; v.u = (uint32_t)EMU_ReadInt(at); return v.f; }
static void EMU_WriteFloat(unsigned int at, float f) { union { uint32_t u; float f; } v; v.f = f; EMU_WriteInt(at, v.u); }
''')
# Exercise the real driver-selection path as well as the GE implementation.
# goldeneye.c already includes this unguarded header in the same translation unit.
game_source = (root / 'games/game.c').read_text().replace('#include "game.h"', '')
# Exercise Perfect Dark's production controller separately from game discovery.
# Copy its exact controller state and reset macros, without its ROM driver.
pd_source = (root / 'games/perfectdark.c').read_text()
pd_controller_start = pd_source.index('static void PD_Controller(void)\n{')
pd_controller_end = pd_source.index('\n}\n', pd_controller_start) + 3
pd_controller_source = '\n'.join(
    line for line in pd_source.splitlines()
    if line.startswith(('static int xstick[', 'static int radialmenudirection[',
                        '#define PD_ResetCamspySlayerStick(', '#define PD_ResetRadialMenuBtns('))
) + '\n' + pd_source[pd_controller_start:pd_controller_end]

harness = r'''
BUTTONS CONTROLLER[4];
struct PROFILE_STRUCT PROFILE[4];
struct DEVICE_STRUCT DEVICE[4];
const unsigned char **rdramptr, **romptr;
const GAMEDRIVER *GAME_PERFECTDARK = NULL;
int mousetoggle = 1;
int emuoverclock, overridefov = 90, overrideratiowidth = 16, overrideratioheight = 9, geshowcrosshair, bypassviewmodelfovtweak;
static void loadrom(const char *name)
{
    GAME_Quit(); romptr = 0;
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
static void duplicate_words(unsigned int from, unsigned int to, unsigned int count)
{
    assert(from + count * 4 <= test_rom_bytes && to + count * 4 <= test_rom_bytes);
    memcpy(test_rom + to / 4, test_rom + from / 4, count * 4);
}
static void assert_globals(const GE_ADDRESS_PROFILE *p, int plus)
{
    assert(p->maxpage == (plus ? 30U : 27U));
    assert(p->menupage == (plus ? 0x8002A8F0U : 0x8002A8C0U));
    assert(p->bonddata == (plus ? 0x8007F410U : 0x80079EE0U));
    assert(p->camera == (plus ? 0x80036834U : 0x80036494U));
    assert(p->pause == (plus ? 0x80048810U : 0x80048370U));
    assert(p->matchended == (plus ? 0x80091D00U : 0x8008C700U));
    assert(p->exit == p->camera + 0x1C && p->tankflag == p->camera - 0x4C);
}
static void assert_menu_input(const GE_ADDRESS_PROFILE *p, int plus)
{
    const unsigned int player = 0x80100000;
    /* Keep gameplay fields valid so a mistaken gameplay injection is caught. */
    memset(PROFILE, 0, sizeof(PROFILE)); memset(DEVICE, 0, sizeof(DEVICE));
    memset(CONTROLLER, 0, sizeof(CONTROLLER));
    PROFILE[0].SETTINGS[CONFIG] = WASD;
    PROFILE[0].SETTINGS[SENSITIVITY] = 40;
    EMU_WriteInt(p->bonddata, player);
    EMU_WriteInt(p->camera, 4); EMU_WriteInt(p->exit, 1);
    EMU_WriteFloat(player + GE_camx, 45.f);
    EMU_WriteFloat(player + GE_camy, 0.f);
    EMU_WriteFloat(player + GE_fov, 60.f);
    EMU_WriteFloat(p->menux, 200.f); EMU_WriteFloat(p->menuy, 160.f);
    test_menu_x = p->menux; test_menu_y = p->menuy;
    if(p->erase_selection) EMU_WriteInt(p->erase_selection, -1);
    if(p->mapmaker.chooser_selection) EMU_WriteInt(p->mapmaker.chooser_selection, -1);
    /* Every discovered frontend page must retain mouse/button routing. */
    for(int page = 1; page <= (int)p->maxpage; page++)
    {
        if(page == 11) continue;
        const int editor = plus && page == 30;
        const int stationary = editor || page == 23;
        const int nativebuttons = (page >= 15 && page <= 17) || page == 20;
        if(!plus && page > 27) continue;
        EMU_WriteInt(p->menupage, page);
        EMU_WriteFloat(p->menux, 200.f); EMU_WriteFloat(p->menuy, 160.f);
        assert(GE_Status()); assert(GAME_Status());
        assert(GAME_Name() && strcmp(GAME_Name(), "GoldenEye 007") == 0);
        DEVICE[0].XPOS = 5; DEVICE[0].YPOS = -3;
        DEVICE[0].BUTTONPRIM[CANCEL] = 1;
        const float x = EMU_ReadFloat(p->menux), y = EMU_ReadFloat(p->menuy);
        test_write_mode = stationary ? 2 : 1;
        GAME_Inject();
        assert(CONTROLLER[0].B_BUTTON);
        if(stationary)
            assert(EMU_ReadFloat(p->menux) == x && EMU_ReadFloat(p->menuy) == y);
        else
            assert(EMU_ReadFloat(p->menux) > x && EMU_ReadFloat(p->menuy) < y);
        DEVICE[0].BUTTONPRIM[CANCEL] = 0;
        GAME_Inject(); assert(!CONTROLLER[0].B_BUTTON);
        DEVICE[0].XPOS = DEVICE[0].YPOS = 0;
        /* Native keyboard/controller button presses and releases remain available. */
        DEVICE[0].BUTTONPRIM[FORWARDS] = DEVICE[0].BUTTONPRIM[BACKWARDS] = 1;
        DEVICE[0].BUTTONPRIM[ACCEPT] = DEVICE[0].BUTTONPRIM[FIRE] = DEVICE[0].BUTTONPRIM[START] = 1;
        GAME_Inject();
        assert(CONTROLLER[0].U_CBUTTON && CONTROLLER[0].D_CBUTTON);
        assert(CONTROLLER[0].A_BUTTON && CONTROLLER[0].Z_TRIG == !nativebuttons && CONTROLLER[0].START_BUTTON);
        memset(DEVICE[0].BUTTONPRIM, 0, sizeof(DEVICE[0].BUTTONPRIM));
        GAME_Inject();
        assert(!CONTROLLER[0].U_CBUTTON && !CONTROLLER[0].D_CBUTTON);
        assert(!CONTROLLER[0].A_BUTTON && !CONTROLLER[0].Z_TRIG && !CONTROLLER[0].START_BUTTON);
        DEVICE[0].BUTTONPRIM[AIM] = 1;
        GAME_Inject(); assert(CONTROLLER[0].R_TRIG == !nativebuttons && CONTROLLER[0].B_BUTTON == !editor);
        DEVICE[0].BUTTONPRIM[AIM] = 0;
        GAME_Inject(); assert(!CONTROLLER[0].B_BUTTON);
        test_write_mode = 0;
    }
    /* An out-of-range page must still drop the driver and stop RAM writes. */
    const int invalid[] = {-2, plus ? 31 : 28, 0x7FFFFFFF};
    for(unsigned int i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++)
    {
        EMU_WriteInt(p->menupage, invalid[i]);
        assert(!GE_Status()); assert(!GAME_Status()); assert(!GAME_Name());
        test_write_mode = 2; GAME_Inject(); test_write_mode = 0;
    }
    /* Returning to gameplay must reacquire the driver and resume mouselook. */
    EMU_WriteInt(p->menupage, 11);
    DEVICE[0].XPOS = 10; DEVICE[0].YPOS = 5;
    assert(GAME_Status()); GAME_Inject();
    assert(EMU_ReadFloat(player + GE_camx) > 45.f);
    assert(EMU_ReadFloat(player + GE_camy) < 0.f);
    GAME_Quit();
    memset(test_ram, 0, sizeof(test_ram));
    memset(PROFILE, 0, sizeof(PROFILE)); memset(DEVICE, 0, sizeof(DEVICE));
    memset(CONTROLLER, 0, sizeof(CONTROLLER));
}
static void assert_menu_bound_guards(const char *rom, int plus)
{
    const unsigned int dispatch = plus ? 0x55D28U : 0x4FA2CU;
    for(int mutation = 0; mutation < 5; mutation++)
    {
        GE_ADDRESS_PROFILE p;
        loadrom(rom);
        switch(mutation)
        {
        case 0: /* Missing dispatch. */
            EMU_WriteROM(dispatch, 0); break;
        case 1: /* Ambiguous dispatch. */
            duplicate_words(dispatch, 0x1E0000, 14); break;
        case 2: /* Plausible signature referencing the wrong menu global. */
            EMU_WriteROM(dispatch + 4, EMU_ReadROM(dispatch + 4) + 4); break;
        case 3: /* Invalid table bound. */
            EMU_WriteROM(dispatch + 16, EMU_ReadROM(dispatch + 16) & 0xFFFF0000U); break;
        case 4: /* Default branch no longer lands on the validated epilogue. */
            EMU_WriteROM(dispatch + 24, EMU_ReadROM(dispatch + 24) + 1); break;
        }
        assert(GE_ResolveAddressProfile(&p)); assert(p.maxpage == 27);
        EMU_WriteInt(p.camera, 4); EMU_WriteInt(p.exit, 1);
        EMU_WriteFloat(p.menux, 200.f); EMU_WriteFloat(p.menuy, 160.f);
        EMU_WriteInt(p.menupage, 28);
        assert(!GE_Status()); assert(!GAME_Status());
        test_write_mode = 2; GAME_Inject(); test_write_mode = 0;
        /* Failure to extend the range must not break ordinary menus. */
        EMU_WriteInt(p.menupage, 27);
        assert(GE_Status()); assert(GAME_Status());
    }
}
static void seed_mapmaker(const GE_ADDRESS_PROFILE *p)
{
    const GE_MAPMAKER_PROFILE *m = &p->mapmaker;
    const unsigned int player = 0x80100000;
    test_write_mode = 0;
    memset(test_ram, 0, sizeof(test_ram));
    memset(PROFILE, 0, sizeof(PROFILE)); memset(DEVICE, 0, sizeof(DEVICE));
    memset(CONTROLLER, 0, sizeof(CONTROLLER));
    PROFILE[0].SETTINGS[CONFIG] = WASD;
    PROFILE[0].SETTINGS[SENSITIVITY] = 40;
    EMU_WriteInt(p->bonddata, player);
    EMU_WriteInt(p->camera, 4); EMU_WriteInt(p->exit, 1);
    EMU_WriteFloat(player + GE_camx, 45.f);
    EMU_WriteFloat(player + GE_camy, 0.f);
    EMU_WriteFloat(player + GE_fov, 60.f);
    EMU_WriteFloat(p->menux, 200.f); EMU_WriteFloat(p->menuy, 160.f);
    EMU_WriteInt(p->menupage, m->page);
    EMU_WriteInt(m->nextpage, -1); EMU_WriteInt(m->nextpagealt, -1);
    EMU_WriteInt(m->freemode, 1);
    /* Camera-only tests deliberately leave optional menu layouts invalid. */
    if(m->menu_selection) EMU_WriteInt(m->menu_selection, -1);
    if(m->chooser_selection) EMU_WriteInt(m->chooser_selection, -1);
    EMU_WriteFloat(m->yaw, 1.f); EMU_WriteFloat(m->pitch, 0.f);
    EMU_WriteInt(m->pitchmin, 0xBFC90FDA);
    EMU_WriteInt(m->pitchmax, 0x3FC90FDA);
    test_editor_yaw = m->yaw; test_editor_pitch = m->pitch;
    test_menu_x = p->menux; test_menu_y = p->menuy;
    assert(GAME_Status());
}
static void editor_frame(unsigned int write_mode)
{
    test_write_mode = write_mode;
    assert(GAME_Status()); GAME_Inject();
    test_write_mode = 0;
}
static void assert_mapmaker_input(const GE_ADDRESS_PROFILE *p, int plus)
{
    const GE_MAPMAKER_PROFILE *m = &p->mapmaker;
    const float degree = 0.017453292519943295f;
    if(!plus)
    {
        const GE_MAPMAKER_PROFILE unresolved = {0};
        assert(!memcmp(m, &unresolved, sizeof(*m)));
        return;
    }
    assert(m->page == 30 && m->yaw == 0x8006C90CU && m->pitch == 0x8006C910U);
    assert(m->menu == 0x8006C918U && m->freemode == 0x8006C934U && m->preview == 0x8006C92CU);
    assert(m->nextpage == 0x8002A8F4U && m->nextpagealt == 0x8002A8F8U);
    assert(m->pitchmin == 0x80054408U && m->pitchmax == 0x80054414U);

    /* Exercise the production driver path; any unrelated RAM write fails. */
    seed_mapmaker(p);
    DEVICE[0].XPOS = DEVICE[0].YPOS = 10;
    editor_frame(3);
    assert(fabsf(EMU_ReadFloat(m->yaw) - (1.f - degree)) < 0.000001f);
    assert(fabsf(EMU_ReadFloat(m->pitch) + degree) < 0.000001f);
    assert(EMU_ReadFloat(p->menux) == 200.f && EMU_ReadFloat(p->menuy) == 160.f);
    assert(EMU_ReadFloat(0x80100000 + GE_camx) == 45.f);
    assert(EMU_ReadFloat(0x80100000 + GE_camy) == 0.f);

    seed_mapmaker(p);
    PROFILE[0].SETTINGS[SENSITIVITY] = 80;
    PROFILE[0].SETTINGS[INVERTPITCH] = 1;
    DEVICE[0].XPOS = DEVICE[0].YPOS = 10;
    editor_frame(3);
    assert(fabsf(EMU_ReadFloat(m->yaw) - (1.f - 2.f * degree)) < 0.000001f);
    assert(fabsf(EMU_ReadFloat(m->pitch) - 2.f * degree) < 0.000001f);

    /* Raw mouse distance must not change with polling cadence or overclock. */
    seed_mapmaker(p);
    DEVICE[0].XPOS = 40; DEVICE[0].YPOS = -40;
    editor_frame(3);
    const float totalyaw = EMU_ReadFloat(m->yaw), totalpitch = EMU_ReadFloat(m->pitch);
    seed_mapmaker(p);
    DEVICE[0].XPOS = 10; DEVICE[0].YPOS = -10; emuoverclock = 1;
    for(int i = 0; i < 4; i++) editor_frame(3);
    emuoverclock = 0;
    assert(fabsf(EMU_ReadFloat(m->yaw) - totalyaw) < 0.000001f);
    assert(fabsf(EMU_ReadFloat(m->pitch) - totalpitch) < 0.000001f);

    /* Zero input cannot write angles, including after a previous movement. */
    DEVICE[0].XPOS = DEVICE[0].YPOS = 0;
    for(int i = 0; i < 8; i++) editor_frame(2);
    assert(EMU_ReadFloat(m->yaw) == totalyaw || fabsf(EMU_ReadFloat(m->yaw) - totalyaw) < 0.000001f);

    /* Native keyboard stick/C-button navigation and editor R remain intact. */
    DEVICE[0].BUTTONPRIM[UP] = DEVICE[0].BUTTONPRIM[RIGHT] = 1;
    DEVICE[0].BUTTONPRIM[FORWARDS] = DEVICE[0].BUTTONPRIM[STRAFELEFT] = 1;
    DEVICE[0].BUTTONPRIM[AIM] = 1;
    editor_frame(2);
    assert(CONTROLLER[0].X_AXIS == 127 && CONTROLLER[0].Y_AXIS == 127);
    assert(CONTROLLER[0].U_CBUTTON && CONTROLLER[0].L_CBUTTON);
    assert(CONTROLLER[0].R_TRIG && !CONTROLLER[0].B_BUTTON);
    DEVICE[0].BUTTONPRIM[CANCEL] = 1;
    editor_frame(2); assert(CONTROLLER[0].B_BUTTON);
    memset(DEVICE[0].BUTTONPRIM, 0, sizeof(DEVICE[0].BUTTONPRIM));
    editor_frame(2);
    assert(!CONTROLLER[0].X_AXIS && !CONTROLLER[0].Y_AXIS && !CONTROLLER[0].R_TRIG && !CONTROLLER[0].B_BUTTON);

    /* Large movements wrap yaw and respect the editor's own pitch limits. */
    for(int sign = -1; sign <= 1; sign += 2)
    {
        seed_mapmaker(p);
        DEVICE[0].XPOS = sign * 1000000; DEVICE[0].YPOS = sign * 1000000;
        editor_frame(3);
        assert(isfinite(EMU_ReadFloat(m->yaw)) && EMU_ReadFloat(m->yaw) >= 0.f && EMU_ReadFloat(m->yaw) < 6.283186f);
        assert(EMU_ReadFloat(m->pitch) == EMU_ReadFloat(sign > 0 ? m->pitchmin : m->pitchmax));
    }

    /* Overlays, orbit/preview, transitions, and invalid state all suppress
     * editor writes while still accepting the controller button release. */
    for(int gate = 0; gate < 18; gate++)
    {
        seed_mapmaker(p);
        DEVICE[0].XPOS = DEVICE[0].YPOS = 10;
        switch(gate)
        {
        case 0: EMU_WriteInt(m->menu, 1); break;
        case 1: EMU_WriteInt(m->preview, 1); break;
        case 2: EMU_WriteInt(m->freemode, 0); break;
        case 3: EMU_WriteInt(m->nextpage, 22); break;
        case 4: EMU_WriteInt(m->nextpagealt, 22); break;
        case 5: DEVICE[0].BUTTONPRIM[START] = 1; break;
        case 6: DEVICE[0].BUTTONSEC[START] = 1; break;
        case 7: PROFILE[0].SETTINGS[CONFIG] = DISABLED; break;
        case 8: EMU_WriteFloat(m->yaw, NAN); break;
        case 9: EMU_WriteFloat(m->pitch, NAN); break;
        case 10: EMU_WriteFloat(m->yaw, INFINITY); break;
        case 11: EMU_WriteFloat(m->pitch, -INFINITY); break;
        case 12: EMU_WriteFloat(m->pitch, 2.f); break;
        case 13: EMU_WriteFloat(m->pitchmin, NAN); break;
        case 14: EMU_WriteFloat(m->pitchmax, INFINITY); break;
        case 15: PROFILE[0].SETTINGS[SENSITIVITY] = 0; break;
        case 16: PROFILE[0].SETTINGS[SENSITIVITY] = -1; break;
        case 17: EMU_WriteInt(m->freemode, 2); break;
        }
        editor_frame(2);
        assert(EMU_ReadFloat(p->menux) == 200.f && EMU_ReadFloat(p->menuy) == 160.f);
        assert(EMU_ReadFloat(0x80100000 + GE_camx) == 45.f);
    }

    /* Selecting Basic/Advanced and returning to ordinary menus keep the
     * frontend cursor, R-as-Back, and gameplay mouselook as before. */
    seed_mapmaker(p);
    DEVICE[0].XPOS = 10; DEVICE[0].YPOS = 5;
    DEVICE[0].BUTTONPRIM[AIM] = 1;
    EMU_WriteInt(p->menupage, 29);
    editor_frame(1);
    assert(CONTROLLER[0].R_TRIG && CONTROLLER[0].B_BUTTON);
    assert(EMU_ReadFloat(p->menux) > 200.f && EMU_ReadFloat(p->menuy) > 160.f);
    assert(EMU_ReadFloat(m->yaw) == 1.f && EMU_ReadFloat(m->pitch) == 0.f);
    DEVICE[0].BUTTONPRIM[AIM] = 0;
    EMU_WriteInt(p->menupage, 11);
    editor_frame(0);
    assert(EMU_ReadFloat(0x80100000 + GE_camx) > 45.f);
    assert(EMU_ReadFloat(0x80100000 + GE_camy) < 0.f);
    assert(EMU_ReadFloat(m->yaw) == 1.f && EMU_ReadFloat(m->pitch) == 0.f);
    GAME_Quit();
    memset(test_ram, 0, sizeof(test_ram));
    memset(PROFILE, 0, sizeof(PROFILE)); memset(DEVICE, 0, sizeof(DEVICE));
    memset(CONTROLLER, 0, sizeof(CONTROLLER));
}
static unsigned int dpad_bits(unsigned int player)
{
    return CONTROLLER[player].U_DPAD | CONTROLLER[player].D_DPAD << 1
        | CONTROLLER[player].L_DPAD << 2 | CONTROLLER[player].R_DPAD << 3;
}
static void assert_mapmaker_dpad(const GE_ADDRESS_PROFILE *p)
{
    const int directions[] = {D_UP, D_DOWN, D_LEFT, D_RIGHT};
    for(int page = 29; page <= 30; page++)
    {
        seed_mapmaker(p); EMU_WriteInt(p->menupage, page);
        const unsigned int write_mode = page == 30 ? 2 : 1;
        /* Each binding sends its actual D-pad bit on press and releases it
         * again, for both configurable primary and secondary bindings. */
        for(unsigned int direction = 0; direction < 4; direction++)
            for(int secondary = 0; secondary < 2; secondary++)
            {
                int *buttons = secondary ? DEVICE[0].BUTTONSEC : DEVICE[0].BUTTONPRIM;
                buttons[directions[direction]] = 1;
                editor_frame(write_mode);
                assert(dpad_bits(0) == (1U << direction));
                assert(!CONTROLLER[0].X_AXIS && !CONTROLLER[0].Y_AXIS);
                assert(!CONTROLLER[0].U_CBUTTON && !CONTROLLER[0].D_CBUTTON);
                buttons[directions[direction]] = 0;
                editor_frame(write_mode); assert(!dpad_bits(0));
            }
        /* Opposing directions remain independent buttons, not a synthetic
         * stick axis. Existing arrow and WASD outputs stay separate. */
        for(unsigned int direction = 0; direction < 4; direction++)
            DEVICE[0].BUTTONPRIM[directions[direction]] = 1;
        DEVICE[0].BUTTONPRIM[UP] = DEVICE[0].BUTTONPRIM[RIGHT] = 1;
        DEVICE[0].BUTTONPRIM[FORWARDS] = DEVICE[0].BUTTONPRIM[STRAFELEFT] = 1;
        editor_frame(write_mode);
        assert(dpad_bits(0) == 15);
        assert(CONTROLLER[0].X_AXIS == 127 && CONTROLLER[0].Y_AXIS == 127);
        assert(CONTROLLER[0].U_CBUTTON && CONTROLLER[0].L_CBUTTON);
        for(unsigned int direction = 0; direction < 4; direction++)
            DEVICE[0].BUTTONPRIM[directions[direction]] = 0;
        editor_frame(write_mode);
        assert(!dpad_bits(0));
        assert(CONTROLLER[0].X_AXIS == 127 && CONTROLLER[0].Y_AXIS == 127);
        assert(CONTROLLER[0].U_CBUTTON && CONTROLLER[0].L_CBUTTON);
        memset(DEVICE[0].BUTTONPRIM, 0, sizeof(DEVICE[0].BUTTONPRIM));

        /* One player's bindings never appear on another controller, and
         * disabling that player clears held D-pad buttons on the next poll. */
        PROFILE[1].SETTINGS[CONFIG] = WASD;
        DEVICE[1].BUTTONPRIM[D_LEFT] = DEVICE[1].BUTTONSEC[D_UP] = 1;
        DEVICE[2].BUTTONPRIM[D_RIGHT] = 1; /* Player 3 stays disabled. */
        editor_frame(write_mode);
        assert(!dpad_bits(0) && dpad_bits(1) == 5 && !dpad_bits(2) && !dpad_bits(3));
        PROFILE[1].SETTINGS[CONFIG] = DISABLED;
        editor_frame(write_mode); assert(!dpad_bits(1));
        PROFILE[0].SETTINGS[CONFIG] = DISABLED;
        DEVICE[0].BUTTONPRIM[D_DOWN] = 1;
        editor_frame(2); assert(!dpad_bits(0));
        assert(EMU_ReadFloat(p->mapmaker.yaw) == 1.f && EMU_ReadFloat(p->mapmaker.pitch) == 0.f);
        assert(EMU_ReadFloat(p->menux) == 200.f && EMU_ReadFloat(p->menuy) == 160.f);
    }
    GAME_Quit();
    memset(test_ram, 0, sizeof(test_ram));
    memset(PROFILE, 0, sizeof(PROFILE)); memset(DEVICE, 0, sizeof(DEVICE));
    memset(CONTROLLER, 0, sizeof(CONTROLLER));
}
static void assert_shoulder_controller(void (*frame)(void))
{
    const int bindings[] = {L_SHOULDER, R_SHOULDER};
    const unsigned int bits[] = {0x2000, 0x1000};
    memset(PROFILE, 0, sizeof(PROFILE)); memset(DEVICE, 0, sizeof(DEVICE));
    memset(CONTROLLER, 0, sizeof(CONTROLLER));
    PROFILE[0].SETTINGS[CONFIG] = WASD;
    /* Check the actual output word: shoulders must not also rotate the stick,
     * select a module, fire, delete, or trigger the synthetic reload bit. */
    for(unsigned int shoulder = 0; shoulder < 2; shoulder++)
        for(int secondary = 0; secondary < 2; secondary++)
        {
            int *buttons = secondary ? DEVICE[0].BUTTONSEC : DEVICE[0].BUTTONPRIM;
            buttons[bindings[shoulder]] = 1; frame();
            assert(CONTROLLER[0].Value == bits[shoulder]);
            for(int player = 1; player < ALLPLAYERS; player++) assert(!CONTROLLER[player].Value);
            buttons[bindings[shoulder]] = 0; frame();
            assert(!CONTROLLER[0].Value);
        }
    /* Releasing one binding must not release a shoulder still held through
     * the other binding. Aim remains an independent source of native R. */
    for(unsigned int shoulder = 0; shoulder < 2; shoulder++)
    {
        DEVICE[0].BUTTONPRIM[bindings[shoulder]] = DEVICE[0].BUTTONSEC[bindings[shoulder]] = 1;
        frame(); assert(CONTROLLER[0].Value == bits[shoulder]);
        DEVICE[0].BUTTONPRIM[bindings[shoulder]] = 0;
        frame(); assert(CONTROLLER[0].Value == bits[shoulder]);
        DEVICE[0].BUTTONSEC[bindings[shoulder]] = 0;
        frame(); assert(!CONTROLLER[0].Value);
    }
    for(int secondary = 0; secondary < 2; secondary++)
    {
        int *buttons = secondary ? DEVICE[0].BUTTONSEC : DEVICE[0].BUTTONPRIM;
        buttons[AIM] = 1; frame(); assert(CONTROLLER[0].Value == 0x1000);
        DEVICE[0].BUTTONPRIM[R_SHOULDER] = 1;
        buttons[AIM] = 0; frame(); assert(CONTROLLER[0].Value == 0x1000);
        buttons[AIM] = 1; DEVICE[0].BUTTONPRIM[R_SHOULDER] = 0;
        frame(); assert(CONTROLLER[0].Value == 0x1000);
        buttons[AIM] = 0; frame(); assert(!CONTROLLER[0].Value);
    }
    DEVICE[0].BUTTONPRIM[D_RIGHT] = 1;
    frame(); assert(CONTROLLER[0].Value == 0x0001);
    DEVICE[0].BUTTONPRIM[L_SHOULDER] = DEVICE[0].BUTTONSEC[R_SHOULDER] = 1;
    frame(); assert(CONTROLLER[0].Value == 0x3001);
    DEVICE[0].BUTTONPRIM[D_RIGHT] = 0;
    frame(); assert(CONTROLLER[0].Value == 0x3000);
    DEVICE[0].BUTTONPRIM[L_SHOULDER] = DEVICE[0].BUTTONSEC[R_SHOULDER] = 0;
    DEVICE[0].BUTTONPRIM[RELOAD] = 1; frame();
    assert(!CONTROLLER[0].L_TRIG && !CONTROLLER[0].R_TRIG && !dpad_bits(0));
    DEVICE[0].BUTTONPRIM[RELOAD] = 0; frame(); assert(!CONTROLLER[0].Value);

    /* Deliberately leave held inputs on disabled profiles. Only the enabled
     * player's controller may receive either shoulder, and disabling it must
     * clear both bits on the next poll rather than leave a stale hold. */
    for(int enabled_player = PLAYER1; enabled_player < ALLPLAYERS; enabled_player++)
    {
        for(int player = PLAYER1; player < ALLPLAYERS; player++)
        {
            PROFILE[player].SETTINGS[CONFIG] = player == enabled_player ? WASD : DISABLED;
            DEVICE[player].BUTTONPRIM[L_SHOULDER] = DEVICE[player].BUTTONSEC[R_SHOULDER] = 1;
        }
        frame();
        for(int player = PLAYER1; player < ALLPLAYERS; player++)
            assert(CONTROLLER[player].Value == (player == enabled_player ? 0x3000U : 0U));
        PROFILE[enabled_player].SETTINGS[CONFIG] = DISABLED;
        frame();
        for(int player = PLAYER1; player < ALLPLAYERS; player++) assert(!CONTROLLER[player].Value);
    }
    memset(PROFILE, 0, sizeof(PROFILE)); memset(DEVICE, 0, sizeof(DEVICE));
    memset(CONTROLLER, 0, sizeof(CONTROLLER));
}
static void shoulder_editor_frame(void)
{
    editor_frame(2); /* Shoulders with no mouse motion must never write RAM. */
}
static void assert_mapmaker_shoulders(const GE_ADDRESS_PROFILE *p)
{
    seed_mapmaker(p);
    assert_shoulder_controller(shoulder_editor_frame);
    assert(EMU_ReadFloat(p->mapmaker.yaw) == 1.f && EMU_ReadFloat(p->mapmaker.pitch) == 0.f);
    assert(EMU_ReadFloat(p->menux) == 200.f && EMU_ReadFloat(p->menuy) == 160.f);
    GAME_Quit(); memset(test_ram, 0, sizeof(test_ram));
}
static void seed_mapmaker_menu(const GE_ADDRESS_PROFILE *p, int chooser)
{
    seed_mapmaker(p); GE_MenuMouseReset(); GE_MenuNativeReset(); mousetoggle = 1;
    EMU_WriteInt(p->mapmaker.menu, chooser ? 0 : 1);
    EMU_WriteInt(p->mapmaker.menu_tool, 0);
    EMU_WriteInt(p->mapmaker.menu_selection, 0);
    EMU_WriteInt(p->mapmaker.chooser_selection, 0);
    EMU_WriteInt(p->menupage, chooser ? p->mapmaker.chooser_page : p->mapmaker.page);
    test_menu_selector = chooser ? p->mapmaker.chooser_selection : p->mapmaker.menu_selection;
}
static void assert_mapmaker_menu_input(const GE_ADDRESS_PROFILE *p)
{
    const GE_MAPMAKER_PROFILE *m = &p->mapmaker;
    /* All fourteen rows work in both panel widths and every tool mode.
     * Writes are restricted to the cursor plus the independently resolved selector. */
    for(int tool = 0; tool < 5; tool++)
        for(int row = 0; row < 14; row++)
        {
            seed_mapmaker_menu(p, 0);
            EMU_WriteInt(m->menu_tool, tool);
            EMU_WriteInt(m->menu_selection, (row + 1) % 14);
            EMU_WriteFloat(p->menux, 100.f); EMU_WriteFloat(p->menuy, 60.f + row * 19.f);
            DEVICE[0].XPOS = 1; editor_frame(4);
            assert(EMU_ReadInt(m->menu_selection) == row);
            assert(EMU_ReadFloat(m->yaw) == 1.f && EMU_ReadFloat(m->pitch) == 0.f);
            assert(EMU_ReadFloat(0x80100000 + GE_camx) == 45.f);
            DEVICE[0].XPOS = 0; DEVICE[0].BUTTONPRIM[FIRE] = 1; editor_frame(2);
            assert(CONTROLLER[0].Z_TRIG == (row != 10));
            assert(dpad_bits(0) == (row == 10 ? 8U : 0U));
            assert(!CONTROLLER[0].A_BUTTON);
            DEVICE[0].BUTTONPRIM[FIRE] = 0; editor_frame(2);
            assert(!CONTROLLER[0].Z_TRIG && !dpad_bits(0));
        }
    /* A fresh click selects the row before the native action. Both mouse
     * bindings and both halves of the Material/Music/Grid values work. */
    const int adjustable[] = {5,8,10};
    for(unsigned int index = 0; index < 3; index++)
        for(int right = 0; right < 2; right++)
            for(int secondary = 0; secondary < 2; secondary++)
            {
                const int row = adjustable[index];
                seed_mapmaker_menu(p, 0);
                EMU_WriteFloat(p->menux, right ? 280.f : 240.f);
                EMU_WriteFloat(p->menuy, 60.f + row * 19.f);
                int *buttons = secondary ? DEVICE[0].BUTTONSEC : DEVICE[0].BUTTONPRIM;
                buttons[FIRE] = 1; editor_frame(4);
                assert(EMU_ReadInt(m->menu_selection) == row);
                assert(dpad_bits(0) == (right ? 8U : 4U));
                assert(!CONTROLLER[0].Z_TRIG && !CONTROLLER[0].A_BUTTON);
                /* Dragging while held cannot change the selected row/action. */
                DEVICE[0].YPOS = -30; editor_frame(4);
                assert(EMU_ReadInt(m->menu_selection) == row);
                assert(dpad_bits(0) == (right ? 8U : 4U));
                DEVICE[0].YPOS = 0; buttons[FIRE] = 0; editor_frame(2);
                assert(!dpad_bits(0));
            }
    /* A stationary cursor cannot undo native keyboard navigation. */
    seed_mapmaker_menu(p, 0); EMU_WriteInt(m->menu_selection, 7);
    EMU_WriteFloat(p->menux, 100.f); EMU_WriteFloat(p->menuy, 60.f);
    DEVICE[0].BUTTONPRIM[D_DOWN] = 1; editor_frame(2);
    assert(EMU_ReadInt(m->menu_selection) == 7 && CONTROLLER[0].D_DPAD);
    DEVICE[0].BUTTONPRIM[D_DOWN] = 0;
    DEVICE[0].BUTTONPRIM[AIM] = 1; editor_frame(2);
    assert(CONTROLLER[0].B_BUTTON && !CONTROLLER[0].R_TRIG);
    DEVICE[0].BUTTONPRIM[R_SHOULDER] = 1; editor_frame(2); assert(CONTROLLER[0].R_TRIG);
    DEVICE[0].BUTTONPRIM[AIM] = DEVICE[0].BUTTONPRIM[R_SHOULDER] = 0;
    DEVICE[0].BUTTONPRIM[PREVIOUSWEAPON] = DEVICE[0].BUTTONSEC[NEXTWEAPON] = 1;
    editor_frame(2); assert(!CONTROLLER[0].A_BUTTON && !CONTROLLER[0].Z_TRIG);
    DEVICE[0].BUTTONPRIM[ACCEPT] = DEVICE[0].BUTTONPRIM[CANCEL] = 1;
    editor_frame(2); assert(CONTROLLER[0].A_BUTTON && CONTROLLER[0].B_BUTTON);

    /* The two chooser rows have separate geometry and selector storage. */
    for(int row = 0; row < 2; row++)
        for(int secondary = 0; secondary < 2; secondary++)
        {
            seed_mapmaker_menu(p, 1); EMU_WriteInt(m->chooser_selection, 1 - row);
            EMU_WriteFloat(p->menux, 100.f); EMU_WriteFloat(p->menuy, row ? 130.f : 100.f);
            DEVICE[0].XPOS = 1; editor_frame(4);
            assert(EMU_ReadInt(m->chooser_selection) == row);
            DEVICE[0].XPOS = 0;
            (secondary ? DEVICE[0].BUTTONSEC : DEVICE[0].BUTTONPRIM)[FIRE] = 1;
            editor_frame(2); assert(CONTROLLER[0].Z_TRIG && !CONTROLLER[0].A_BUTTON);
            assert(!dpad_bits(0) && EMU_ReadFloat(m->yaw) == 1.f);
        }
    const float outside[][2] = {{85,60},{355,60},{100,50},{20,20}};
    for(unsigned int point = 0; point < sizeof(outside) / sizeof(outside[0]); point++)
    {
        seed_mapmaker_menu(p, 0); EMU_WriteInt(m->menu_selection, 7);
        EMU_WriteFloat(p->menux, outside[point][0]); EMU_WriteFloat(p->menuy, outside[point][1]);
        DEVICE[0].BUTTONPRIM[FIRE] = 1; editor_frame(2);
        assert(EMU_ReadInt(m->menu_selection) == 7 && !CONTROLLER[0].Z_TRIG && !dpad_bits(0));
    }
    seed_mapmaker_menu(p, 0); EMU_WriteInt(m->menu_tool, 1);
    EMU_WriteFloat(p->menux, 330.f); EMU_WriteFloat(p->menuy, 60.f);
    DEVICE[0].BUTTONPRIM[FIRE] = 1; editor_frame(2); assert(!CONTROLLER[0].Z_TRIG);
    const float chooseroutside[][2] = {{69,100},{401,100},{100,89},{100,110},{100,119},{100,140}};
    for(unsigned int point = 0; point < sizeof(chooseroutside) / sizeof(chooseroutside[0]); point++)
    {
        seed_mapmaker_menu(p, 1);
        EMU_WriteFloat(p->menux, chooseroutside[point][0]); EMU_WriteFloat(p->menuy, chooseroutside[point][1]);
        DEVICE[0].BUTTONPRIM[FIRE] = 1; editor_frame(2);
        assert(!CONTROLLER[0].Z_TRIG && !dpad_bits(0));
    }
    for(int sign = -1; sign <= 1; sign += 2)
    {
        seed_mapmaker_menu(p, 0);
        DEVICE[0].XPOS = DEVICE[0].YPOS = sign * 1000000; editor_frame(4);
        assert(EMU_ReadFloat(p->menux) == (sign < 0 ? 20.f : 420.f));
        assert(EMU_ReadFloat(p->menuy) == (sign < 0 ? 20.f : 310.f));
        assert(EMU_ReadFloat(m->yaw) == 1.f && EMU_ReadFloat(m->pitch) == 0.f);
    }
    /* Invalid menu state must neither select a row nor rotate a camera. */
    for(int gate = 0; gate < 11; gate++)
    {
        seed_mapmaker_menu(p, 0); DEVICE[0].XPOS = DEVICE[0].YPOS = 10;
        switch(gate)
        {
        case 0: EMU_WriteInt(m->menu, 2); break;
        case 1: EMU_WriteInt(m->preview, 1); break;
        case 2: EMU_WriteInt(m->nextpage, 22); break;
        case 3: EMU_WriteInt(m->nextpagealt, 22); break;
        case 4: EMU_WriteInt(m->menu_selection, -1); break;
        case 5: EMU_WriteInt(m->menu_selection, 14); break;
        case 6: EMU_WriteInt(m->menu_tool, -1); break;
        case 7: EMU_WriteInt(m->menu_tool, 5); break;
        case 8: PROFILE[0].SETTINGS[SENSITIVITY] = 0; break;
        case 9: PROFILE[0].SETTINGS[CONFIG] = DISABLED; break;
        case 10: mousetoggle = 0; break;
        }
        editor_frame(gate == 8 ? 4 : 2); mousetoggle = 1;
        assert(EMU_ReadFloat(m->yaw) == 1.f && EMU_ReadFloat(m->pitch) == 0.f);
    }
    /* Zero movement sensitivity still permits clicks, and recapturing a
     * held mouse button cannot manufacture a fresh menu activation. */
    seed_mapmaker_menu(p, 0); PROFILE[0].SETTINGS[SENSITIVITY] = 0;
    EMU_WriteFloat(p->menux, 100.f); EMU_WriteFloat(p->menuy, 79.f);
    DEVICE[0].BUTTONPRIM[FIRE] = 1; editor_frame(4);
    assert(EMU_ReadInt(m->menu_selection) == 1 && CONTROLLER[0].Z_TRIG);
    seed_mapmaker_menu(p, 0);
    EMU_WriteFloat(p->menux, 100.f); EMU_WriteFloat(p->menuy, 60.f);
    editor_frame(2); mousetoggle = 0;
    DEVICE[0].BUTTONPRIM[FIRE] = 1; editor_frame(2);
    mousetoggle = 1; editor_frame(2); assert(!CONTROLLER[0].Z_TRIG);
    DEVICE[0].BUTTONPRIM[FIRE] = 0; editor_frame(2);
    DEVICE[0].BUTTONPRIM[FIRE] = 1; editor_frame(2); assert(CONTROLLER[0].Z_TRIG);

    /* Opening a menu while holding Fire cannot activate under the cursor;
     * a click that closes a menu cannot become a placement or gunshot. */
    seed_mapmaker_menu(p, 0); EMU_WriteInt(m->menu, 0);
    DEVICE[0].BUTTONPRIM[FIRE] = 1; editor_frame(2);
    EMU_WriteInt(m->menu, 1); EMU_WriteFloat(p->menux, 100.f); EMU_WriteFloat(p->menuy, 60.f);
    editor_frame(2); assert(!CONTROLLER[0].Z_TRIG);
    DEVICE[0].BUTTONPRIM[FIRE] = 0; editor_frame(2);
    DEVICE[0].BUTTONPRIM[FIRE] = 1; editor_frame(2); assert(CONTROLLER[0].Z_TRIG);
    EMU_WriteInt(m->menu, 0); editor_frame(2); assert(!CONTROLLER[0].Z_TRIG);
    EMU_WriteInt(p->menupage, 11); EMU_WriteInt(p->pause, 1); editor_frame(2);
    assert(!CONTROLLER[0].Z_TRIG);
    DEVICE[0].BUTTONPRIM[FIRE] = 0; editor_frame(2);
    DEVICE[0].BUTTONPRIM[FIRE] = 1; editor_frame(2); assert(CONTROLLER[0].Z_TRIG);
    GAME_Quit();
    assert(!ge_menu_mouse_context && !ge_menu_mouse_clickcontext && !ge_menu_mouse_blockfire && !ge_menu_mouse_previousfire);
    memset(test_ram, 0, sizeof(test_ram));
    memset(PROFILE, 0, sizeof(PROFILE)); memset(DEVICE, 0, sizeof(DEVICE));
}

static void seed_native_menu(const GE_ADDRESS_PROFILE *p)
{
    GE_MenuNativeReset(); test_write_mode = 0; mousetoggle = 1;
    memset(test_ram, 0, sizeof(test_ram));
    memset(PROFILE, 0, sizeof(PROFILE)); memset(DEVICE, 0, sizeof(DEVICE));
    memset(CONTROLLER, 0, sizeof(CONTROLLER));
    EMU_WriteInt(p->menupage, 11); EMU_WriteInt(p->camera, 4);
    EMU_WriteInt(p->exit, 1); EMU_WriteInt(p->pause, 1);
    EMU_WriteFloat(p->menux, 200.f); EMU_WriteFloat(p->menuy, 160.f);
    test_menu_x = p->menux; test_menu_y = p->menuy;
    for(int player = 0; player < 4; player++)
    {
        const unsigned int base = 0x80100000 + player * 0x10000;
        PROFILE[player].SETTINGS[CONFIG] = WASD;
        PROFILE[player].SETTINGS[SENSITIVITY] = 40;
        EMU_WriteInt(p->bonddata + player * 4, base);
        EMU_WriteFloat(base + GE_camx, 45.f);
        EMU_WriteFloat(base + GE_camy, 0.f);
        EMU_WriteFloat(base + GE_fov, 60.f);
    }
    editor_frame(2);
}
static void assert_native_menu_input(const GE_ADDRESS_PROFILE *p)
{
    const unsigned int base = 0x80100000;
    /* Opening/closing animations must not consume mouse gestures. */
    for(int watch = 0; watch <= 13; watch++)
    {
        seed_native_menu(p); EMU_WriteInt(base + GE_watch, watch);
        DEVICE[0].XPOS = 100; editor_frame(2);
        assert(GE_MenuNativeContext(0) == (watch == 5 ? 1 : 0));
        assert(dpad_bits(0) == (watch == 5 ? 8U : 0U));
    }
    /* Multiplayer input is per player, accepts live pause and completed
     * results, but not the end-of-round countdown or normal death. */
    for(int ended = -1; ended <= 3; ended++)
        for(int dead = 0; dead <= 2; dead++)
        {
            seed_native_menu(p); EMU_WriteInt(p->matchended, ended);
            EMU_WriteInt(base + GE_deathflag, dead);
            EMU_WriteInt(base + GE_multipausemenu, 1);
            DEVICE[0].YPOS = -100; editor_frame(2);
            const int active = (ended == 0 || ended == 1) && (dead == 0 || (dead == 1 && ended == 1));
            assert(GE_MenuNativeContext(0) == (active ? 2 : 0));
            assert(dpad_bits(0) == (active ? 1U : 0U));
        }
    const int dx[] = {0,0,-100,100};
    const int dy[] = {-100,100,0,0};
    for(int context = 1; context <= 2; context++)
        for(int direction = 0; direction < 4; direction++)
        {
            seed_native_menu(p);
            EMU_WriteInt(base + (context == 1 ? GE_watch : GE_multipausemenu), context == 1 ? 5 : 1);
            DEVICE[0].XPOS = dx[direction]; DEVICE[0].YPOS = dy[direction];
            editor_frame(2); assert(dpad_bits(0) == (1U << direction));
            DEVICE[0].XPOS = DEVICE[0].YPOS = 0;
            for(int frame = 0; frame < 100; frame++) editor_frame(2);
            assert(!dpad_bits(0));
            assert(EMU_ReadFloat(base + GE_camx) == 45.f && EMU_ReadFloat(base + GE_camy) == 0.f);
        }
    /* Both mouse bindings activate/cancel, while the physical controls
     * retain their existing outputs and right-click cannot turn a page. */
    for(int secondary = 0; secondary < 2; secondary++)
    {
        seed_native_menu(p); EMU_WriteInt(base + GE_watch, 5);
        editor_frame(2);
        int *buttons = secondary ? DEVICE[0].BUTTONSEC : DEVICE[0].BUTTONPRIM;
        buttons[FIRE] = buttons[AIM] = 1;
        buttons[D_LEFT] = buttons[UP] = buttons[FORWARDS] = 1;
        editor_frame(2);
        assert(CONTROLLER[0].A_BUTTON && CONTROLLER[0].B_BUTTON);
        assert(!CONTROLLER[0].Z_TRIG && !CONTROLLER[0].R_TRIG);
        assert(CONTROLLER[0].L_DPAD && CONTROLLER[0].X_AXIS == 127 && CONTROLLER[0].U_CBUTTON);
        buttons[R_SHOULDER] = 1; editor_frame(2); assert(CONTROLLER[0].R_TRIG);
        buttons[R_SHOULDER] = buttons[AIM] = 0;
        EMU_WriteInt(base + GE_watch, 0); editor_frame(2);
        assert(!CONTROLLER[0].A_BUTTON && !CONTROLLER[0].Z_TRIG && !CONTROLLER[0].B_BUTTON);
        buttons[FIRE] = 0; editor_frame(2);
        buttons[FIRE] = 1; editor_frame(2); assert(CONTROLLER[0].Z_TRIG && !CONTROLLER[0].A_BUTTON);
    }
    /* Held Fire while a watch opens waits for release; zero sensitivity
     * and no selected mouse device must still allow a deliberate click. */
    seed_native_menu(p); DEVICE[0].BUTTONPRIM[FIRE] = 1;
    editor_frame(2); assert(CONTROLLER[0].Z_TRIG);
    EMU_WriteInt(base + GE_watch, 5); editor_frame(2);
    assert(!CONTROLLER[0].A_BUTTON && !CONTROLLER[0].Z_TRIG);
    DEVICE[0].BUTTONPRIM[FIRE] = 0; editor_frame(2);
    PROFILE[0].SETTINGS[SENSITIVITY] = 0;
    DEVICE[0].BUTTONPRIM[FIRE] = 1; editor_frame(2);
    assert(CONTROLLER[0].A_BUTTON && !CONTROLLER[0].Z_TRIG);
    DEVICE[0].BUTTONPRIM[FIRE] = 0; editor_frame(2);
    PROFILE[0].SETTINGS[MOUSE] = -1;
    DEVICE[0].BUTTONSEC[FIRE] = 1; editor_frame(2);
    assert(CONTROLLER[0].A_BUTTON && !CONTROLLER[0].Z_TRIG);
    DEVICE[0].BUTTONSEC[FIRE] = 0;
    DEVICE[0].BUTTONPRIM[PREVIOUSWEAPON] = DEVICE[0].BUTTONSEC[NEXTWEAPON] = 1;
    editor_frame(2); assert(!CONTROLLER[0].A_BUTTON && !CONTROLLER[0].Z_TRIG);
    DEVICE[0].BUTTONPRIM[ACCEPT] = 1; editor_frame(2); assert(CONTROLLER[0].A_BUTTON);

    /* A player's watch is single-player only; each MP menu is independent. */
    seed_native_menu(p);
    EMU_WriteInt(base + 0x10000 + GE_watch, 5);
    DEVICE[1].XPOS = 100; editor_frame(2); assert(!dpad_bits(1));
    EMU_WriteInt(base + 0x10000 + GE_watch, 0);
    EMU_WriteInt(base + 0x10000 + GE_multipausemenu, 1);
    editor_frame(2); assert(dpad_bits(1) == 8 && !dpad_bits(0) && !dpad_bits(2) && !dpad_bits(3));
    PROFILE[1].SETTINGS[CONFIG] = DISABLED; editor_frame(2); assert(!dpad_bits(1));
    /* Capture, sensitivity and invalid pointers must not cause speculative
     * RAM or camera writes. */
    for(int gate = 0; gate < 7; gate++)
    {
        seed_native_menu(p); EMU_WriteInt(base + GE_watch, 5);
        DEVICE[0].XPOS = 100;
        switch(gate)
        {
        case 0: mousetoggle = 0; break;
        case 1: PROFILE[0].SETTINGS[CONFIG] = DISABLED; break;
        case 2: PROFILE[0].SETTINGS[SENSITIVITY] = 0; break;
        case 3: PROFILE[0].SETTINGS[SENSITIVITY] = -1; break;
        case 4: PROFILE[0].SETTINGS[MOUSE] = -1; break;
        case 5: EMU_WriteInt(p->bonddata, 0x80000001); break;
        case 6: EMU_WriteInt(p->bonddata, 0x807FFFFC); break;
        }
        editor_frame(2); assert(!dpad_bits(0)); mousetoggle = 1;
    }
    seed_native_menu(p); EMU_WriteInt(base + GE_watch, 5);
    DEVICE[0].XPOS = 100; editor_frame(2); assert(dpad_bits(0));
    GAME_Quit();
    const GE_MENU_NATIVE_STATE empty = {0};
    for(int player = 0; player < 4; player++) assert(!memcmp(&ge_menu_native[player], &empty, sizeof(empty)));
    DEVICE[0].XPOS = 0; editor_frame(2); assert(!dpad_bits(0));
    GAME_Quit(); memset(test_ram, 0, sizeof(test_ram));
    memset(PROFILE, 0, sizeof(PROFILE)); memset(DEVICE, 0, sizeof(DEVICE));
}
static void assert_frontend_native_input(const GE_ADDRESS_PROFILE *p)
{
    const int pages[] = {5,15,16,17,20};
    assert(p->erase_selection == p->menupage + 0x5C);
    for(unsigned int i = 0; i < sizeof(pages) / sizeof(pages[0]); i++)
        for(int direction = 0; direction < 4; direction++)
        {
            const int page = pages[i];
            seed_native_menu(p); EMU_WriteInt(p->menupage, page);
            EMU_WriteInt(p->erase_selection, 0);
            DEVICE[0].XPOS = direction == 2 ? -100 : direction == 3 ? 100 : 0;
            DEVICE[0].YPOS = direction == 0 ? -100 : direction == 1 ? 100 : 0;
            editor_frame(1);
            assert(GE_MenuNativeContext(0) == page + 100);
            assert(dpad_bits(0) == (direction >= 2 || page == 20 ? 1U << direction : 0U));
            DEVICE[0].BUTTONPRIM[FIRE] = DEVICE[0].BUTTONSEC[AIM] = 1;
            editor_frame(1);
            assert(CONTROLLER[0].A_BUTTON && CONTROLLER[0].B_BUTTON);
            assert(!CONTROLLER[0].Z_TRIG && !CONTROLLER[0].R_TRIG);
            DEVICE[0].BUTTONPRIM[FIRE] = DEVICE[0].BUTTONSEC[AIM] = 0;
            DEVICE[0].XPOS = DEVICE[0].YPOS = 0;
            for(int frame = 0; frame < 100; frame++) editor_frame(1);
            assert(!CONTROLLER[0].A_BUTTON && !CONTROLLER[0].B_BUTTON && !dpad_bits(0));
        }
    for(int page = 15; page <= 17; page++)
    {
        seed_native_menu(p); EMU_WriteInt(p->menupage, page);
        DEVICE[2].XPOS = -100; editor_frame(1);
        DEVICE[2].XPOS = 0; DEVICE[2].BUTTONSEC[FIRE] = 1;
        editor_frame(1);
        assert(GE_MenuNativeContext(2) == page + 100);
        assert(dpad_bits(2) == 4 && !dpad_bits(0) && !dpad_bits(1) && !dpad_bits(3));
        assert(CONTROLLER[2].A_BUTTON && !CONTROLLER[2].Z_TRIG && !CONTROLLER[0].A_BUTTON);
    }
    for(int flag = -1; flag <= 4; flag++)
    {
        seed_native_menu(p); EMU_WriteInt(p->menupage, 5);
        EMU_WriteInt(p->erase_selection, flag);
        DEVICE[0].XPOS = DEVICE[1].XPOS = 100;
        editor_frame(1);
        const int active = flag >= 0 && flag <= 3;
        assert(GE_MenuNativeContext(0) == (active ? 105 : 0));
        assert(dpad_bits(0) == (active ? 8U : 0U));
        assert(!GE_MenuNativeContext(1) && !dpad_bits(1));
    }
    GAME_Quit(); memset(test_ram, 0, sizeof(test_ram));
    memset(PROFILE, 0, sizeof(PROFILE)); memset(DEVICE, 0, sizeof(DEVICE));
}
static void assert_erase_menu_resolver_guards(const char *rom, int plus)
{
    const unsigned int anchor = plus ? 0x40F00 : 0x40960;
    GE_ADDRESS_PROFILE p;
    for(int mutation = 0; mutation < 4; mutation++)
    {
        loadrom(rom);
        switch(mutation)
        {
        case 0: EMU_WriteROM(anchor, 0); break;
        case 1: duplicate_words(anchor, 0x1E0000, 9); break;
        case 2: EMU_WriteROM(anchor + 0x70, 0); break;
        case 3: EMU_WriteROM(anchor + 0x64, EMU_ReadROM(anchor + 0x64) + 1); break;
        }
        assert(GE_ResolveAddressProfile(&p)); assert_globals(&p, plus);
        assert(!p.erase_selection);
        if(plus) assert(p.mapmaker.menu_selection && p.mapmaker.chooser_selection && p.mapmaker.page == 30);
    }
    loadrom(rom);
}

static void assert_native_menu_pulse(void)
{
    /* A 50 ms hold and 50 ms neutral interval are observable to the emulated
     * game, and one large movement cannot queue an unbounded scroll. */
    GE_MENU_NATIVE_STATE state = {0};
    assert(GE_MenuNativePulse(&state, 10000, 0, 10) == 4);
    for(int frame = 0; frame < 4; frame++) assert(GE_MenuNativePulse(&state, 0, 0, 10) == 4);
    for(int frame = 0; frame < 20; frame++) assert(!GE_MenuNativePulse(&state, 0, 0, 10));
    assert(!state.x && !state.y);
    state = (GE_MENU_NATIVE_STATE){0};
    assert(!GE_MenuNativePulse(&state, 3, 0, 10));
    assert(!GE_MenuNativePulse(&state, 3, 0, 10));
    assert(GE_MenuNativePulse(&state, 2, 0, 10) == 4);
    state = (GE_MENU_NATIVE_STATE){0};
    assert(GE_MenuNativePulse(&state, 100, -100, 10) == 1);
    state = (GE_MENU_NATIVE_STATE){0};
    assert(GE_MenuNativePulse(&state, 200, 100, 10) == 4);
    state = (GE_MENU_NATIVE_STATE){0};
    assert(!GE_MenuNativePulse(&state, 7, 0, 10));
    for(int frame = 0; frame < 10; frame++) assert(!GE_MenuNativePulse(&state, 0, 0, 10));
    assert(!GE_MenuNativePulse(&state, 1, 0, 10));
    for(int invalid = 0; invalid < 5; invalid++)
    {
        state = (GE_MENU_NATIVE_STATE){0};
        assert(GE_MenuNativePulse(&state, 0, 8, 10) == 2);
        assert(!GE_MenuNativePulse(&state, invalid == 0 ? NAN : invalid == 1 ? INFINITY : 0,
            invalid == 2 ? -INFINITY : 0, invalid == 3 ? 0 : invalid == 4 ? 51 : 10));
        assert(!state.heldms && !state.releasems && !state.x && !state.y);
    }
}

static void set_address_operands(unsigned int high, unsigned int low, unsigned int address)
{
    EMU_WriteROM(high, (EMU_ReadROM(high) & 0xFFFF0000U) | ((address + 0x8000U) >> 16));
    EMU_WriteROM(low, (EMU_ReadROM(low) & 0xFFFF0000U) | (address & 0xFFFFU));
}
static void assert_mapmaker_resolver_guards(const char *rom)
{
    const unsigned int anchors[] = {0x62730, 0x6338C, 0x5F2BC, 0x51B84, 0x5521C};
    const unsigned int lengths[] = {30, 24, 56, 19, 15};
    GE_ADDRESS_PROFILE baseline, p;
    loadrom(rom); assert(GE_ResolveAddressProfile(&baseline));
    for(unsigned int anchor = 0; anchor < 5; anchor++)
        for(int duplicate = 0; duplicate < 2; duplicate++)
        {
            loadrom(rom);
            if(duplicate) duplicate_words(anchors[anchor], 0x1E0000, lengths[anchor]);
            else EMU_WriteROM(anchors[anchor], 0xFFFFFFFFU);
            assert(GE_ResolveAddressProfile(&p));
            assert(p.maxpage == 30 && p.mapmaker.page == 0);
            seed_mapmaker(&baseline);
            DEVICE[0].XPOS = DEVICE[0].YPOS = 10;
            DEVICE[0].BUTTONPRIM[AIM] = 1;
            /* Unsupported editors retain ordinary controls; no editor
             * address or gameplay field may be written speculatively. */
            editor_frame(1);
            assert(CONTROLLER[0].R_TRIG && CONTROLLER[0].B_BUTTON);
            assert(EMU_ReadFloat(baseline.mapmaker.yaw) == 1.f);
        }
    /* Move the referenced data by 2 MiB, including signed low operands.
     * Successful injection must touch the new camera and leave the old one. */
    loadrom(rom);
    const unsigned int delta = 0x200000;
    set_address_operands(0x62730 + 0x3C, 0x62730 + 0x40, baseline.mapmaker.preview + delta);
    set_address_operands(0x62730 + 0x50, 0x62730 + 0x64, baseline.mapmaker.menu + delta);
    set_address_operands(0x6338C, 0x6338C + 4, baseline.mapmaker.freemode + delta);
    set_address_operands(0x5F2BC + 0x50, 0x5F2BC + 0x58, baseline.mapmaker.yaw + delta);
    set_address_operands(0x5F2BC + 0x68, 0x5F2BC + 0x70, baseline.mapmaker.pitch + delta);
    set_address_operands(0x5F2BC + 0x30, 0x5F2BC + 0x38, baseline.mapmaker.pitchmin + delta);
    set_address_operands(0x5F2BC + 0x88, 0x5F2BC + 0xBC, baseline.mapmaker.pitchmax + delta);
    assert(GE_ResolveAddressProfile(&p));
    assert(p.mapmaker.page == 30 && p.mapmaker.yaw == baseline.mapmaker.yaw + delta);
    assert(p.mapmaker.pitch == baseline.mapmaker.pitch + delta);
    seed_mapmaker(&p);
    DEVICE[0].XPOS = DEVICE[0].YPOS = 10;
    editor_frame(3);
    assert(EMU_ReadFloat(p.mapmaker.yaw) < 1.f && EMU_ReadFloat(p.mapmaker.pitch) < 0.f);
    assert(EMU_ReadInt(baseline.mapmaker.yaw) == 0 && EMU_ReadInt(baseline.mapmaker.pitch) == 0);
    /* A plausible but inconsistent pitch address must disable the feature. */
    EMU_WriteROM(0x5F2BC + 0x70, EMU_ReadROM(0x5F2BC + 0x70) + 4);
    assert(GE_ResolveAddressProfile(&p)); assert(!p.mapmaker.page);
    loadrom(rom);
}
static void assert_mapmaker_menu_resolver_guards(const char *rom)
{
    GE_ADDRESS_PROFILE baseline, p;
    loadrom(rom); assert(GE_ResolveAddressProfile(&baseline));
    assert(baseline.mapmaker.menu_selection == 0x8006C91CU);
    assert(baseline.mapmaker.menu_tool == 0x8006C930U);
    assert(baseline.mapmaker.chooser_page == 29);
    assert(baseline.mapmaker.chooser_selection == 0x8006C8ECU);

    /* Optional pause-menu proof must fail independently of both the
     * camera and the separately verified Basic/Advanced selector. */
    for(int fault = 0; fault < 10; fault++)
    {
        GE_MAPMAKER_PROFILE expected = baseline.mapmaker;
        expected.menu_selection = expected.menu_tool = 0;
        loadrom(rom);
        switch(fault)
        {
        case 0: EMU_WriteROM(0x52BC4, 0xFFFFFFFFU); break;
        case 1: duplicate_words(0x52BC4, 0x1E0000, 55); break;
        case 2: EMU_WriteROM(0x627DC, 0xFFFFFFFFU); break;
        case 3: EMU_WriteROM(0x53040, 0xFFFFFFFFU); break;
        case 4: EMU_WriteROM(0x64D48, 0xFFFFFFFFU); break;
        case 5: EMU_WriteROM(0x64DC8, 0xFFFFFFFFU); break;
        case 6: EMU_WriteROM(0x52BC4 + 0x6C, 0x0C000000U); break;
        case 7: EMU_WriteROM(0x52BC4, 0x0C000000U); break;
        case 8:
            set_address_operands(0x62730 + 0xD8, 0x62730 + 0xE0,
                                 baseline.mapmaker.menu_selection + 4);
            break;
        case 9:
            EMU_WriteROM(0x53044, EMU_ReadROM(0x53044) + 4);
            break;
        }
        assert(GE_ResolveAddressProfile(&p));
        if(memcmp(&p.mapmaker, &expected, sizeof(expected)))
            fprintf(stderr, "optional pause-menu resolver fault %d\n", fault);
        assert(!memcmp(&p.mapmaker, &expected, sizeof(expected)));
    }

    /* Chooser input, drawing, return page and linked calls must agree.
     * Damage in this proof leaves the pause-menu and camera available. */
    for(int fault = 0; fault < 10; fault++)
    {
        GE_MAPMAKER_PROFILE expected = baseline.mapmaker;
        expected.chooser_page = expected.chooser_selection = 0;
        loadrom(rom);
        switch(fault)
        {
        case 0: EMU_WriteROM(0x51B10, 0xFFFFFFFFU); break;
        case 1: EMU_WriteROM(0x51C94, 0xFFFFFFFFU); break;
        case 2: EMU_WriteROM(0x51D54, 0xFFFFFFFFU); break;
        case 3: EMU_WriteROM(0x51F1C + 4, 0xFFFFFFFFU); break;
        case 4: EMU_WriteROM(0x51F1C, 0x0C000000U); break;
        case 5: EMU_WriteROM(0x51B84 + 0x418, 0x0C000000U); break;
        case 6:
            EMU_WriteROM(0x51B84 + 0x414, 0x2404001EU);
            break; /* Editor itself cannot also be the chooser page. */
        case 7:
            EMU_WriteROM(0x51B84 + 0x414, 0x2404001FU);
            break; /* Above the discovered frontend dispatch limit. */
        case 8:
            set_address_operands(0x51B14, 0x51B18, 0x80800000U);
            break;
        case 9:
            set_address_operands(0x51D54, 0x51D58,
                                 baseline.mapmaker.chooser_selection + 4);
            break;
        }
        assert(GE_ResolveAddressProfile(&p));
        if(memcmp(&p.mapmaker, &expected, sizeof(expected)))
            fprintf(stderr, "optional chooser resolver fault %d\n", fault);
        assert(!memcmp(&p.mapmaker, &expected, sizeof(expected)));
    }
    loadrom(rom);
}

static void assert_native_reload(const GE_ADDRESS_PROFILE *p, int plus)
{
#ifdef SPEEDRUN_BUILD
    (void)plus; assert(!p->native_reload);
#else
    assert(p->native_reload == plus);
    if(!plus) return;
    for(int secondary = 0; secondary < 2; secondary++)
    {
        seed_mapmaker(p); EMU_WriteInt(p->menupage, 11);
        if(secondary) DEVICE[0].BUTTONSEC[RELOAD] = 1;
        else DEVICE[0].BUTTONPRIM[RELOAD] = 1;
        DEVICE[0].BUTTONPRIM[FIRE] = 1;
        editor_frame(0);
        assert(CONTROLLER[0].B_BUTTON && !CONTROLLER[0].Z_TRIG && !CONTROLLER[0].RELOAD_HACK);
        DEVICE[0].BUTTONPRIM[RELOAD] = DEVICE[0].BUTTONSEC[RELOAD] = 0;
        editor_frame(0);
        assert(!CONTROLLER[0].B_BUTTON && CONTROLLER[0].Z_TRIG && !CONTROLLER[0].RELOAD_HACK);
        /* The existing E/native-B action, including its Fire combination,
         * remains unchanged after releasing the dedicated reload key. */
        DEVICE[0].BUTTONPRIM[CANCEL] = 1;
        editor_frame(0); assert(CONTROLLER[0].B_BUTTON && CONTROLLER[0].Z_TRIG);
    }
    for(int gate = 0; gate < 11; gate++)
    {
        seed_mapmaker(p); EMU_WriteInt(p->menupage, 11);
        DEVICE[0].BUTTONPRIM[RELOAD] = 1;
        switch(gate)
        {
        case 0: EMU_WriteInt(p->menupage, 30); break;
        case 1: EMU_WriteInt(p->menupage, 29); break;
        case 2: EMU_WriteInt(p->pause, 1); break;
        case 3: EMU_WriteInt(0x80100000 + GE_deathflag, 1); break;
        case 4: EMU_WriteInt(0x80100000 + GE_watch, 1); break;
        case 5: EMU_WriteInt(0x80100000 + GE_multipausemenu, 1); break;
        case 6: EMU_WriteInt(p->exit, 0); break;
        case 7: EMU_WriteInt(p->camera, 2); break;
        case 8: EMU_WriteInt(p->matchended, 1); break;
        case 9: PROFILE[0].SETTINGS[CONFIG] = DISABLED; break;
        case 10: DEVICE[0].BUTTONPRIM[START] = 1; break;
        }
        editor_frame(0);
        assert(!CONTROLLER[0].B_BUTTON && !CONTROLLER[0].RELOAD_HACK);
    }
    GAME_Quit();
    memset(test_ram, 0, sizeof(test_ram));
    memset(PROFILE, 0, sizeof(PROFILE)); memset(DEVICE, 0, sizeof(DEVICE));
    memset(CONTROLLER, 0, sizeof(CONTROLLER));
#endif
}
static void assert_native_reload_guards(const char *rom)
{
#ifndef SPEEDRUN_BUILD
    const unsigned int anchors[] = {0xD49A0, 0xDDE0C, 0x1172B0};
    const unsigned int lengths[] = {10, 8, 11};
    GE_ADDRESS_PROFILE p;
    for(unsigned int anchor = 0; anchor < 3; anchor++)
        for(int duplicate = 0; duplicate < 2; duplicate++)
        {
            loadrom(rom);
            if(duplicate) duplicate_words(anchors[anchor], 0x1E0000, lengths[anchor]);
            else EMU_WriteROM(anchors[anchor], 0xFFFFFFFFU);
            assert(GE_ResolveAddressProfile(&p));
            if(p.mapmaker.page != 30 || p.native_reload) fprintf(stderr, "reload guard anchor=%X duplicate=%d page=%u reload=%d\n", anchors[anchor], duplicate, p.mapmaker.page, p.native_reload);
            assert(p.mapmaker.page == 30 && !p.native_reload);
            seed_mapmaker(&p); EMU_WriteInt(p.menupage, 11);
            DEVICE[0].BUTTONPRIM[RELOAD] = DEVICE[0].BUTTONPRIM[FIRE] = 1;
            editor_frame(0);
            assert(!CONTROLLER[0].B_BUTTON && CONTROLLER[0].Z_TRIG && CONTROLLER[0].RELOAD_HACK);
        }
    loadrom(rom);
    EMU_WriteROM(0x1172B0 + 0x24, EMU_ReadROM(0x1172B0 + 0x24) + 1);
    assert(GE_ResolveAddressProfile(&p)); assert(p.mapmaker.page == 30 && !p.native_reload);
    loadrom(rom);
#else
    (void)rom;
#endif
}
int main(int argc, char **argv)
{
    assert(argc == 3);
    assert_native_menu_pulse();
    assert_shoulder_controller(PD_Controller);
    puts("Perfect Dark: production controller shoulder bindings, aim alias, release and player isolation passed");
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
        assert_mapmaker_input(&addresses, plus);
        if(plus) assert_mapmaker_dpad(&addresses);
        if(plus) assert_mapmaker_shoulders(&addresses);
        if(plus) assert_mapmaker_menu_input(&addresses);
        assert_menu_input(&addresses, plus);
        assert_native_menu_input(&addresses);
        assert_frontend_native_input(&addresses);
        assert_native_reload(&addresses, plus);
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
        assert_menu_bound_guards(argv[image], plus);
        assert_erase_menu_resolver_guards(argv[image], plus);
        if(plus)
        {
            assert_mapmaker_resolver_guards(argv[image]);
            assert_mapmaker_menu_resolver_guards(argv[image]);
            assert_native_reload_guards(argv[image]);
        }
        printf("%s: production resolver, frontend/watch/multiplayer/Map Maker input, D-pad and shoulders, native reload, bounded writes, boot preparation, lifecycle and signature guards passed (%s)\n", plus ? "Plus" : "retail",
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
    cpath.write_text(source + '\n' + game_source + '\n' + pd_controller_source + '\n' + harness)
    binary = Path(work) / 'test'
    for mode in ([], ['-DSPEEDRUN_BUILD']):
        subprocess.run(['cc','-std=c11','-O1','-fgnu89-inline','-Wall','-Wextra','-Wno-parentheses',*mode,str(cpath),'-lm','-o',str(binary)],check=True)
        subprocess.run([str(binary),*sys.argv[1:]],check=True)
