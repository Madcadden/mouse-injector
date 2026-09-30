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
    unsigned int menupage, maxpage, erase_selection, matchended;
    GE_MAPMAKER_PROFILE mapmaker;
} GE_ADDRESS_PROFILE;

static TEST_PROFILE PROFILE[ALLPLAYERS];
static TEST_DEVICE DEVICE[ALLPLAYERS];
static TEST_CONTROLLER CONTROLLER[ALLPLAYERS];
static unsigned int playerbase[ALLPLAYERS];
static GE_ADDRESS_PROFILE test_profile;
static int current_page;
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

    /* Vertical mouse gesture -> native Up. */
    GE_MenuNativeReset();
    current_page = 31;
    clear_input();
    DEVICE[0].YPOS = -100.0f;
    GE_MenuNativeInputs();
    assert(CONTROLLER[0].U_DPAD == 1);
    assert(CONTROLLER[0].D_DPAD == 0);

    /* Horizontal mouse gesture -> native Right (detail-page toggle path). */
    GE_MenuNativeReset();
    current_page = 33;
    clear_input();
    DEVICE[0].XPOS = 100.0f;
    GE_MenuNativeInputs();
    assert(CONTROLLER[0].R_DPAD == 1);
    assert(CONTROLLER[0].L_DPAD == 0);

    /* Fresh left click becomes A after held-entry suppression has cleared. */
    GE_MenuNativeReset();
    current_page = 31;
    clear_input();
    GE_MenuNativeInputs();
    clear_input();
    DEVICE[0].BUTTONPRIM[FIRE] = 1;
    GE_MenuNativeInputs();
    assert(CONTROLLER[0].A_BUTTON == 1);
    assert(CONTROLLER[0].Z_TRIG == 0);

    /* Right-click/Aim is Back and dedicated shoulder mapping remains separate. */
    clear_input();
    DEVICE[0].BUTTONPRIM[AIM] = 1;
    GE_MenuNativeInputs();
    assert(CONTROLLER[0].B_BUTTON == 1);

    puts("PASS: appended Plus frontend menu context, movement, accept and back routing");
    return 0;
}
