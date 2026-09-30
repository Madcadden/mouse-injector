/* Host-only regression for dynamically appended Plus frontend menus.
 * No ROM or emulator process is executed. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define ALLPLAYERS 4
#define PLAYER1 0
#define DISABLED 0
#define TICKRATE 5
#define PD_DECOMP 0

enum { CONFIG, MOUSE, SENSITIVITY, SETTINGS_COUNT };
enum { FIRE, ACCEPT, AIM, R_SHOULDER, BUTTON_COUNT };

typedef struct { int SETTINGS[SETTINGS_COUNT]; int BUTTONPRIM[BUTTON_COUNT]; int BUTTONSEC[BUTTON_COUNT]; } TEST_PROFILE;
typedef struct { float XPOS, YPOS; int BUTTONPRIM[BUTTON_COUNT]; int BUTTONSEC[BUTTON_COUNT]; } TEST_DEVICE;
typedef struct {
    int A_BUTTON, B_BUTTON, Z_TRIG, R_TRIG;
    int U_DPAD, D_DPAD, L_DPAD, R_DPAD;
} TEST_CONTROLLER;

typedef struct GE_MAPMAKER_PROFILE { unsigned int page; } GE_MAPMAKER_PROFILE;
typedef struct GE_ADDRESS_PROFILE {
    unsigned int menupage, maxpage, erase_selection, matchended, menux, menuy;
    GE_MAPMAKER_PROFILE mapmaker;
} GE_ADDRESS_PROFILE;

static TEST_PROFILE PROFILE[ALLPLAYERS];
static TEST_DEVICE DEVICE[ALLPLAYERS];
static TEST_CONTROLLER CONTROLLER[ALLPLAYERS];
static unsigned int playerbase[ALLPLAYERS];
static GE_ADDRESS_PROFILE test_profile;
static int current_page;
static float cursor_x = 220.0f, cursor_y = 86.0f;
static int mousetoggle = 1;

#define GE_deathflag 0
#define GE_multipausemenu 0
#define GE_matchended (test_profile.matchended)
#define ONLY1PLAYERACTIVE 1

static int ClampInt(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static int EMU_ReadInt(unsigned int address)
{
    if(address == test_profile.menupage) return current_page;
    if(address == test_profile.erase_selection) return -1;
    if(address == test_profile.matchended) return 0;
    return 0;
}
static float EMU_ReadFloat(unsigned int address)
{
    if(address == test_profile.menux) return cursor_x;
    if(address == test_profile.menuy) return cursor_y;
    return 0.0f;
}
static const GE_ADDRESS_PROFILE *GE_GetAddressProfile(void) { return &test_profile; }

#include "../games/goldeneye.menunav.h"

static void clear_input(void)
{
    memset(DEVICE, 0, sizeof(DEVICE));
    memset(CONTROLLER, 0, sizeof(CONTROLLER));
}

int main(void)
{
    memset(PROFILE, 0, sizeof(PROFILE));
    memset(&test_profile, 0, sizeof(test_profile));
    PROFILE[0].SETTINGS[CONFIG] = 1;
    PROFILE[0].SETTINGS[MOUSE] = 0;
    PROFILE[0].SETTINGS[SENSITIVITY] = 40;
    test_profile.menupage = 0x80000100U;
    test_profile.matchended = 0x80000104U;
    test_profile.menux = 0x80000108U;
    test_profile.menuy = 0x8000010CU;
    test_profile.mapmaker.page = 30;
    test_profile.maxpage = 33;

    current_page = 30;
    assert(GE_MenuNativeContext(0) == 0);
    current_page = 31;
    assert(GE_MenuNativeContext(0) == 231);
    current_page = 32;
    assert(GE_MenuNativeContext(0) == 232);
    current_page = 33;
    assert(GE_MenuNativeContext(0) == 233);

    /* Future Plus menus appended after Map Maker inherit navigation without
     * another page-number edit when the resolved table upper bound grows. */
    test_profile.maxpage = 37;
    current_page = 37;
    assert(GE_MenuNativeContext(0) == 237);
    current_page = 38;
    assert(GE_MenuNativeContext(0) == 0);
    test_profile.maxpage = 33;

    /* Category cursor follows authored rows: hover Multiplayer (row 1). */
    GE_MenuNativeReset();
    current_page = 31;
    cursor_y = 116.0f;
    clear_input();
    GE_MenuNativeInputs();
    assert(CONTROLLER[0].D_DPAD == 1);
    /* Let the pulse and neutral gap finish; the same hover must then settle. */
    for(int i=0;i<20;i++) { clear_input(); GE_MenuNativeInputs(); }
    clear_input(); GE_MenuNativeInputs();
    assert(CONTROLLER[0].U_DPAD == 0 && CONTROLLER[0].D_DPAD == 0);
    assert(ge_menu_native[0].cursorrow == 1);
    assert(ge_plus_levelmod_category == 1);

    /* Moving to Miscellaneous produces another Down edge. */
    cursor_y = 146.0f;
    for(int i=0;i<20;i++) { clear_input(); GE_MenuNativeInputs(); }
    clear_input(); GE_MenuNativeInputs();
    assert(CONTROLLER[0].D_DPAD == 1);

    /* Entering the level list resets the level row but preserves category. */
    GE_MenuNativeReset();
    current_page = 32;
    cursor_y = 110.0f; /* visible row 2 */
    clear_input(); GE_MenuNativeInputs();
    assert(CONTROLLER[0].D_DPAD == 1);
    assert(ge_menu_native[0].pluscategory == 1);

    /* Fresh left click on an already aligned category becomes A, not Z. */
    GE_MenuNativeReset();
    current_page = 31;
    cursor_y = 86.0f;
    clear_input();
    GE_MenuNativeInputs();
    clear_input();
    DEVICE[0].BUTTONPRIM[FIRE] = 1;
    GE_MenuNativeInputs();
    assert(CONTROLLER[0].A_BUTTON == 1);
    assert(CONTROLLER[0].Z_TRIG == 0);

    /* A click on a different row waits for cursor/highlight alignment. */
    GE_MenuNativeReset();
    current_page = 31;
    cursor_y = 146.0f;
    clear_input();
    DEVICE[0].BUTTONPRIM[FIRE] = 1;
    GE_MenuNativeInputs();
    assert(CONTROLLER[0].A_BUTTON == 0);
    assert(CONTROLLER[0].D_DPAD == 1);

    /* Detail page: one row, horizontal mouse movement is not required. */
    GE_MenuNativeReset();
    current_page = 33;
    cursor_y = 94.0f;
    clear_input(); GE_MenuNativeInputs();
    clear_input();
    DEVICE[0].BUTTONPRIM[FIRE] = 1;
    GE_MenuNativeInputs();
    assert(CONTROLLER[0].A_BUTTON == 1);

    /* Right-click/Aim is Back. */
    clear_input();
    DEVICE[0].BUTTONPRIM[AIM] = 1;
    GE_MenuNativeInputs();
    assert(CONTROLLER[0].B_BUTTON == 1);

    puts("PASS: appended Plus frontend menu context, movement, accept and back routing");
    return 0;
}
