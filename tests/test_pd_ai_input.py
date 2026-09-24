#!/usr/bin/env python3
"""Production driver lifecycle/input regression for the supplied PD-AI v2.42 ROM.

Usage: python tests/test_pd_ai_input.py GoldenEye-PD-AI-v2.42.z64
Uses simulated bounded RDRAM, not live gameplay. No ROM is changed on disk.
"""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
scope = {'__file__': str(root / 'tests/test_goldeneye_resolver.py')}
exec((root / 'tests/test_goldeneye_resolver.py').read_text().split('with tempfile.TemporaryDirectory')[0], scope)
harness = scope['harness'].split('int main(int argc, char **argv)')[0] + r'''
static void pdai_globals(GE_ADDRESS_PROFILE *a) {
    assert(GE_ResolveAddressProfile(a));
    assert(a->bonddata == 0x800803F0U && a->camera == 0x80036754U);
    assert(a->exit == 0x80036770U && a->pause == 0x800486B0U);
    assert(a->menupage == 0x8002A950U && a->maxpage == 27);
    assert(a->matchended == 0x80092C10U && !a->mapmaker.page && !a->native_reload);
}
static void pdai_play(const GE_ADDRESS_PROFILE *a, int multiplayer) {
    const unsigned int player = 0x80100000U;
    memset(test_ram, 0, sizeof(test_ram));
    memset(PROFILE, 0, sizeof(PROFILE)); memset(DEVICE, 0, sizeof(DEVICE));
    memset(CONTROLLER, 0, sizeof(CONTROLLER));
    PROFILE[0].SETTINGS[CONFIG] = WASD;
    PROFILE[0].SETTINGS[SENSITIVITY] = 40;
    EMU_WriteInt(a->camera, multiplayer ? 0 : 4);
    EMU_WriteInt(a->exit, 1); EMU_WriteInt(a->menupage, 11);
    EMU_WriteFloat(a->menux, 200.f); EMU_WriteFloat(a->menuy, 160.f);
    /* Real PD-AI pointer table has nine player slots. Enable only the human. */
    for(int slot = 0; slot < (multiplayer ? 9 : 1); slot++) {
        const unsigned int body = player + slot * 0x4000U;
        EMU_WriteInt(a->bonddata + slot * 4, body);
        EMU_WriteFloat(body + GE_camx, 45.f);
        EMU_WriteFloat(body + GE_camy, 0.f);
        EMU_WriteFloat(body + GE_fov, 60.f);
    }
    DEVICE[0].XPOS = 10; DEVICE[0].YPOS = 5;
    DEVICE[0].BUTTONPRIM[FORWARDS] = DEVICE[0].BUTTONPRIM[STRAFELEFT] = 1;
    DEVICE[0].BUTTONPRIM[FIRE] = DEVICE[0].BUTTONPRIM[AIM] = 1;
    DEVICE[0].BUTTONPRIM[RELOAD] = DEVICE[0].BUTTONPRIM[START] = 1;
    assert(GAME_Status()); GAME_Inject();
    assert(EMU_ReadFloat(player + GE_camx) > 45.f);
    assert(EMU_ReadFloat(player + GE_camy) < 0.f);
    assert(CONTROLLER[0].U_CBUTTON && CONTROLLER[0].L_CBUTTON);
    assert(CONTROLLER[0].Z_TRIG && CONTROLLER[0].R_TRIG && CONTROLLER[0].START_BUTTON);
#ifndef SPEEDRUN_BUILD
    assert(CONTROLLER[0].RELOAD_HACK);
#endif
    for(int slot = 1; slot < (multiplayer ? 9 : 1); slot++) {
        const unsigned int body = player + slot * 0x4000U;
        assert(EMU_ReadFloat(body + GE_camx) == 45.f);
        assert(EMU_ReadFloat(body + GE_camy) == 0.f);
    }
    memset(DEVICE[0].BUTTONPRIM, 0, sizeof(DEVICE[0].BUTTONPRIM));
    GAME_Inject();
    assert(!CONTROLLER[0].U_CBUTTON && !CONTROLLER[0].L_CBUTTON);
    assert(!CONTROLLER[0].Z_TRIG && !CONTROLLER[0].R_TRIG && !CONTROLLER[0].START_BUTTON);
    assert(!CONTROLLER[0].RELOAD_HACK);
    if(multiplayer) {
        /* Adjacent stopPlay must not be mistaken for the game-over flag. */
        float x = EMU_ReadFloat(player + GE_camx);
        EMU_WriteInt(a->matchended + 4, 1);
        GAME_Inject(); assert(EMU_ReadFloat(player + GE_camx) > x);
        EMU_WriteInt(a->matchended, 1);
        x = EMU_ReadFloat(player + GE_camx);
        const float y = EMU_ReadFloat(player + GE_camy);
        DEVICE[0].BUTTONPRIM[ACCEPT] = 1;
        test_write_mode = 2;
        assert(GAME_Status()); GAME_Inject();
        test_write_mode = 0;
        assert(EMU_ReadFloat(player + GE_camx) == x && EMU_ReadFloat(player + GE_camy) == y);
        assert(CONTROLLER[0].A_BUTTON);
        DEVICE[0].BUTTONPRIM[ACCEPT] = 0; GAME_Inject();
        assert(!CONTROLLER[0].A_BUTTON);
        EMU_WriteInt(a->matchended, 0);
        GAME_Inject(); assert(EMU_ReadFloat(player + GE_camx) > x);
    }
}
static void pdai_guards(const char *rom) {
    const unsigned int anchor = 0x1135F8U;
    GE_ADDRESS_PROFILE a;
    for(int mutation = 0; mutation < 9; mutation++) {
        loadrom(rom);
        switch(mutation) {
        case 0: EMU_WriteROM(anchor, 0); break;
        case 1: duplicate_words(anchor, 0x1E0000, 7); break;
        case 2: /* A distinct, retail-order candidate must also be ambiguous. */
            duplicate_words(anchor, 0x1E0000, 7);
            EMU_WriteROM(0x1E0004, 0xAC202D10); EMU_WriteROM(0x1E000C, 0xAC202D14);
            EMU_WriteROM(0x1E0018, 0xAC202D28); break;
        case 3: EMU_WriteROM(anchor + 4, 0xAC202C18); break;
        case 4: EMU_WriteROM(anchor + 24, 0xAC202C2C); break;
        case 5: EMU_WriteROM(anchor + 20, 0); break;
        case 6: /* Adjacent pair crosses below RDRAM. */
            EMU_WriteROM(anchor, 0x3C018000); EMU_WriteROM(anchor + 4, 0xAC200000);
            EMU_WriteROM(anchor + 8, 0x3C018000); EMU_WriteROM(anchor + 12, 0xAC20FFFC);
            EMU_WriteROM(anchor + 16, 0x3C018000); EMU_WriteROM(anchor + 24, 0xAC200014); break;
        case 7: /* Flags valid but paused flag crosses beyond RDRAM. */
            EMU_WriteROM(anchor, 0x3C018080); EMU_WriteROM(anchor + 4, 0xAC20FFF4);
            EMU_WriteROM(anchor + 8, 0x3C018080); EMU_WriteROM(anchor + 12, 0xAC20FFF0);
            EMU_WriteROM(anchor + 16, 0x3C018080); EMU_WriteROM(anchor + 24, 0xAC200008); break;
        case 8: EMU_WriteROM(anchor + 4, 0xAC202C15);
            EMU_WriteROM(anchor + 12, 0xAC202C11); EMU_WriteROM(anchor + 24, 0xAC202C29); break;
        }
        assert(!GE_FindMatchEnded()); assert(!GE_ResolveAddressProfile(&a));
        test_write_mode = 2; assert(!GAME_Status()); GAME_Inject(); GE_PrepareROM(); test_write_mode = 0;
        assert(ge_ownedcount == 0);
    }
    /* A relocated signed-low immediate is still derived, never hardcoded. */
    loadrom(rom);
    EMU_WriteROM(anchor, 0x3C01800A); EMU_WriteROM(anchor + 4, 0xAC209C14);
    EMU_WriteROM(anchor + 8, 0x3C01800A); EMU_WriteROM(anchor + 12, 0xAC209C10);
    EMU_WriteROM(anchor + 16, 0x3C01800A); EMU_WriteROM(anchor + 24, 0xAC209C28);
    assert(GE_FindMatchEnded() == 0x80099C10U);
    assert(GE_ResolveAddressProfile(&a) && a.matchended == 0x80099C10U);
}
int main(int argc, char **argv) {
    assert(argc == 2);
    GE_ADDRESS_PROFILE a;
    loadrom(argv[1]); pdai_globals(&a);
    uint32_t *original = malloc(test_rom_bytes); assert(original);
    memcpy(original, test_rom, test_rom_bytes);
    /* Boot preparation uses the release's production patch logic. */
    for(int cycle = 0; cycle < 2; cycle++) {
        GE_HACK_PROFILE *h = GE_GetHackProfile();
        assert(h); unsigned int allowed[96], count = 0;
        if(h->aimvalid) for(unsigned int i = 0; i < 27; i++) allowed[count++] = h->aimaddress[i];
#ifndef SPEEDRUN_BUILD
        const GE_RELOAD_HACK_PROFILE *r = GE_GetReloadHackProfile();
        if(r) for(unsigned int i = 0; i < 23; i++) allowed[count++] = r->patchaddress[i];
        for(int i = 0; i < 3; i++) if(h->fov[i]) allowed[count++] = h->fov[i];
        if(h->controlstyle) allowed[count++] = h->controlstyle;
        if(h->reversepitch) allowed[count++] = h->reversepitch;
        if(h->ratiocrosshair) allowed[count++] = h->ratiocrosshair;
#endif
        if(h->showcrosshair) allowed[count++] = h->showcrosshair;
        geshowcrosshair = 1; overridefov = 90; overrideratiowidth = 21;
        test_write_mode = 2; GE_PrepareROM(); test_write_mode = 0;
        pdai_globals(&a);
        unsigned int changed = 0;
        for(size_t i = 0; i < test_rom_bytes / 4; i++) if(test_rom[i] != original[i]) {
            int found = 0; for(unsigned int j = 0; j < count; j++) if(allowed[j] == i * 4) found = 1;
            assert(found); changed++;
        }
        assert(changed > 0);
        pdai_play(&a, 0); pdai_play(&a, 1);
        /* Existing test exercises every frontend page including native
         * arena/scenario/team pages, transitions, click and release routing. */
        assert_menu_input(&a, 0);
        assert_native_menu_input(&a); assert_frontend_native_input(&a);
        GAME_Quit(); assert(!GAME_Name());
        assert(!memcmp(test_rom, original, test_rom_bytes));
        /* Same allocation/checksums must re-resolve on the next cycle. */
    }
    pdai_guards(argv[1]);
    free(original); free(test_rom);
    puts("PD-AI v2.42: globals, two boot/reopen cycles, ROM patch bounds/restoration, all menus, single-player, one-human/eight-Sim input isolation, match-end flag and nine rejection mutations passed");
    return 0;
}
'''
rom = Path(sys.argv[1]).resolve()
assert hashlib.sha256(rom.read_bytes()).hexdigest() == '8486cb7ba6bd8a315aaa582bb2ab11970fbc8a9c23473b62484af45174696f32', 'Expected supplied v2.42 ROM'
results = []
with tempfile.TemporaryDirectory(prefix='pdai-input-test-') as work:
    cpath = Path(work) / 'test.c'
    cpath.write_text(scope['source'] + '\n' + scope['game_source'] + '\n' + scope['pd_controller_source'] + '\n' + harness)
    binary = Path(work) / 'test'
    for name, mode in [('normal', []), ('speedrun', ['-DSPEEDRUN_BUILD'])]:
        subprocess.run(['cc', '-std=c11', '-O1', '-fgnu89-inline', *mode, str(cpath), '-lm', '-o', str(binary)], check=True)
        run = subprocess.run([str(binary), str(rom)], check=True, capture_output=True, text=True)
        results.append({'mode': name, 'result': run.stdout.strip()})
print(json.dumps({'rom_sha256': hashlib.sha256(rom.read_bytes()).hexdigest(), 'source_sha256': hashlib.sha256((root / 'games/goldeneye.c').read_bytes()).hexdigest(), 'tests': results, 'limits': 'Production C with bounded simulated RAM/input. Does not execute game frames or prove live Windows input.'}, indent=2))
