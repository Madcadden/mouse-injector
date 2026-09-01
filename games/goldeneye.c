//==========================================================================
// Mouse Injector Plugin
//==========================================================================
// Copyright (C) 2016-2021 Carnivorous
// All rights reserved.
//
// Mouse Injector is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
// for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, visit http://www.gnu.org/licenses/gpl-2.0.html
//==========================================================================
#include <math.h>
#include "../global.h"
#include "../maindll.h"
#include "game.h"
#include "memory.h"

#define GUNAIMLIMIT 6.879164696 // 0x40DC221E
#define CROSSHAIRLIMIT 5.159373283 // 0x40A51996
#define TANKXROTATIONLIMIT 6.283185005 // 0x40C90FDA
#define PI 3.1415927 // 0x40490FDB

typedef struct GE_ADDRESS_PROFILE
{
	unsigned int bonddata;
	unsigned int camera;
	unsigned int exit;
	unsigned int pause;
	unsigned int menupage;
	unsigned int menux;
	unsigned int menuy;
	unsigned int tankxrot;
	unsigned int tankflag;
	unsigned int matchended;
	unsigned int introcounter;
	unsigned int seenintroflag;
} GE_ADDRESS_PROFILE;

static const GE_ADDRESS_PROFILE GE_RETAIL_ADDRESSES =
{
	0x80079EE0, 0x80036494, 0x800364B0, 0x80048370,
	0x8002A8C0, 0x8002A908, 0x8002A90C, 0x80036484,
	0x80036448, 0x8008C700, 0x8002A8CC, 0x8002A930
};

/*
 * Resolve the globals used by mouse injection from stable startup-code
 * patterns. Every anchor must occur exactly once; otherwise the original
 * retail profile is retained and GE_Status() still has to validate it.
 */
#define GE_ROM_SCAN_LIMIT 0x00200000

static const unsigned int gemenupattern[5] = {0x3C013F80, 0x44810000, 0x2402FFFF, 0x3C010000, 0xAC220000};
static const unsigned int gemenumask[5] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000};
static const unsigned int gebonddatapattern[10] = {0x0C000000, 0x0000A025, 0x1840001B, 0x00147080, 0x3C0F0000, 0x25EF0000, 0x01CF9021, 0x24130750, 0x00008825, 0x8E580000};
static const unsigned int gebonddatamask[10] = {0xFC000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
static const unsigned int gecamerapattern[13] = {0x3C010000, 0xE4260000, 0x3C010000, 0xE4280000, 0x3C010000, 0xAC200000, 0x3C010000, 0x240D0001, 0xAC2D0000, 0x3C010000, 0xAC200000, 0x3C010000, 0xAC200000};
static const unsigned int gecameramask[13] = {0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000};
static const unsigned int gepausepattern[12] = {0x3C013F80, 0x44816000, 0x24020001, 0x3C010000, 0x27BDFFC8, 0xAC220000, 0xAFB10024, 0x3C010000, 0x3C110000, 0xAC200000, 0x26310000, 0xAE220000};
static const unsigned int gepausemask[12] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFFFFFF, 0xFFFF0000, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFFFFFF};

#ifndef SPEEDRUN_BUILD
typedef struct GE_RELOAD_HACK_PROFILE
{
	unsigned int reloadflag;
	unsigned int inputmask;
	unsigned int playercheck;
	unsigned int weaponstate;
	unsigned int reloadlogic;
	unsigned int playerhigh;
	unsigned int playerlow;
	unsigned int controllow;
	unsigned int reloadcall;
	unsigned int weaponstatecall;
} GE_RELOAD_HACK_PROFILE;

static const unsigned int gereloadinputpattern[5] = {0x8FA20060, 0x8D830124, 0x304F4000, 0x000F702B, 0x2C650001};
static const unsigned int gereloadinputmask[5] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
static const unsigned int gereloadplayerpattern[10] = {0x8FAB01B0, 0x11600095, 0x3C0D0000, 0x8DAD0000, 0x24010001, 0x3C0C0000, 0x15A1002E, 0x3C020000, 0x8D8C0000, 0x24040020};
static const unsigned int gereloadplayermask[10] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000, 0xFFFFFFFF, 0xFFFF0000, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000, 0xFFFFFFFF};
static const unsigned int gereloadflagpattern[8] = {0xAC200000, 0x10000005, 0x8FAC0144, 0x8E0D0000, 0x240B0001, 0xADAB00D0, 0x8FAC0144, 0x1580000B};
static const unsigned int gereloadflagmask[8] = {0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};
static const unsigned int gereloadweaponpattern[8] = {0x3C0E0000, 0x8DCE0000, 0x03E00008, 0x8DC200D0, 0x3C0E0000, 0x8DCE0000, 0x03E00008, 0xA1C412B6};
static const unsigned int gereloadweaponmask[8] = {0xFFFF0000, 0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF0000, 0xFFFF0000, 0xFFFFFFFF, 0xFFFFFFFF};
static const unsigned int gereloadlogicpattern[11] = {0x00000000, 0x10400009, 0x00000000, 0x0C000000, 0x00000000, 0x10400005, 0x00000000, 0x0C000000, 0x00002025, 0x0C000000, 0x24040001};
static const unsigned int gereloadlogicmask[11] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFC000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFC000000, 0xFFFFFFFF, 0xFC000000, 0xFFFFFFFF};
#endif

static int GE_ROMPatternMatches(const unsigned int offset, const unsigned int *pattern, const unsigned int *mask, const unsigned int wordcount)
{
	for(unsigned int index = 0; index < wordcount; index++)
	{
		if((EMU_ReadROM(offset + index * 4) & mask[index]) != (pattern[index] & mask[index]))
			return 0;
	}
	return 1;
}

static unsigned int GE_FindUniqueROMPattern(const unsigned int *pattern, const unsigned int *mask, const unsigned int wordcount)
{
	const unsigned int bytecount = wordcount * 4;
	unsigned int match = 0;

	if(romptr == 0 || bytecount > GE_ROM_SCAN_LIMIT)
		return 0;

	for(unsigned int offset = 0x1000; offset <= GE_ROM_SCAN_LIMIT - bytecount; offset += 4)
	{
		if(GE_ROMPatternMatches(offset, pattern, mask, wordcount))
		{
			if(match != 0)
				return 0;
			match = offset;
		}
	}

	return match;
}

#ifndef SPEEDRUN_BUILD
static int GE_ResolveReloadHack(GE_RELOAD_HACK_PROFILE *profile)
{
	const unsigned int inputmatch = GE_FindUniqueROMPattern(gereloadinputpattern, gereloadinputmask, 5);
	const unsigned int playermatch = GE_FindUniqueROMPattern(gereloadplayerpattern, gereloadplayermask, 10);
	const unsigned int flagmatch = GE_FindUniqueROMPattern(gereloadflagpattern, gereloadflagmask, 8);
	const unsigned int weaponmatch = GE_FindUniqueROMPattern(gereloadweaponpattern, gereloadweaponmask, 8);
	const unsigned int logicmatch = GE_FindUniqueROMPattern(gereloadlogicpattern, gereloadlogicmask, 11);

	if(inputmatch == 0 || playermatch == 0 || flagmatch == 0 || weaponmatch == 0 || logicmatch == 0)
		return 0;

	/* Both calls reload the same weapon-state function. */
	if(EMU_ReadROM(logicmatch + 0x1C) != EMU_ReadROM(logicmatch + 0x24))
		return 0;

	profile->reloadflag = flagmatch + 0x10;
	profile->inputmask = inputmatch + 0x08;
	profile->playercheck = playermatch + 0x08;
	profile->weaponstate = weaponmatch;
	profile->reloadlogic = logicmatch;
	profile->playerhigh = EMU_ReadROM(profile->playercheck) & 0xFFFF;
	profile->playerlow = EMU_ReadROM(profile->playercheck + 0x04) & 0xFFFF;
	profile->controllow = EMU_ReadROM(profile->playercheck + 0x18) & 0xFFFF;
	profile->reloadcall = EMU_ReadROM(profile->reloadlogic + 0x0C);
	profile->weaponstatecall = EMU_ReadROM(profile->reloadlogic + 0x1C);
	return 1;
}

static const GE_RELOAD_HACK_PROFILE *GE_GetReloadHackProfile(void)
{
	static GE_RELOAD_HACK_PROFILE resolved;
	static unsigned int cachedcrc1 = 0;
	static unsigned int cachedcrc2 = 0;
	static int initialized = 0;
	static int valid = 0;
	unsigned int crc1;
	unsigned int crc2;

	if(romptr == 0)
		return 0;

	crc1 = EMU_ReadROM(0x10);
	crc2 = EMU_ReadROM(0x14);
	if(!initialized || crc1 != cachedcrc1 || crc2 != cachedcrc2)
	{
		initialized = 1;
		cachedcrc1 = crc1;
		cachedcrc2 = crc2;
		valid = GE_ResolveReloadHack(&resolved);
	}

	return valid ? &resolved : 0;
}
#endif

static unsigned int GE_MakeAddress(const unsigned int lui, const unsigned int lowinstruction)
{
	return ((lui & 0xFFFF) << 16) + (int)(short)(lowinstruction & 0xFFFF);
}

static int GE_ResolveAddressProfile(GE_ADDRESS_PROFILE *profile)
{
	unsigned int menumatch;
	unsigned int bondmatch;
	unsigned int cameramatch;
	unsigned int pausematch;

	if(romptr == 0 || EMU_ReadROM(0x20) != 0x474F4C44)
		return 0;

	menumatch = GE_FindUniqueROMPattern(gemenupattern, gemenumask, 5);
	bondmatch = GE_FindUniqueROMPattern(gebonddatapattern, gebonddatamask, 10);
	cameramatch = GE_FindUniqueROMPattern(gecamerapattern, gecameramask, 13);
	pausematch = GE_FindUniqueROMPattern(gepausepattern, gepausemask, 12);

	if(menumatch == 0 || bondmatch == 0 || cameramatch == 0 || pausematch == 0)
		return 0;

	profile->menupage = GE_MakeAddress(EMU_ReadROM(menumatch + 0x0C), EMU_ReadROM(menumatch + 0x10));
	profile->bonddata = GE_MakeAddress(EMU_ReadROM(bondmatch + 0x10), EMU_ReadROM(bondmatch + 0x14));
	profile->camera = GE_MakeAddress(EMU_ReadROM(cameramatch + 0x2C), EMU_ReadROM(cameramatch + 0x30));
	profile->pause = GE_MakeAddress(EMU_ReadROM(pausematch + 0x1C), EMU_ReadROM(pausematch + 0x24));

	profile->exit = profile->camera + 0x1C;
	profile->menux = profile->menupage + 0x48;
	profile->menuy = profile->menupage + 0x4C;
	profile->tankxrot = profile->camera - 0x10;
	profile->tankflag = profile->camera - 0x4C;
	profile->matchended = profile->bonddata + 0x12820;
	profile->introcounter = profile->menupage + 0x0C;
	profile->seenintroflag = profile->menupage + 0x70;

	return (profile->bonddata & 0xFF800000U) == 0x80000000U
		&& (profile->camera & 0xFF800000U) == 0x80000000U
		&& (profile->pause & 0xFF800000U) == 0x80000000U
		&& (profile->menupage & 0xFF800000U) == 0x80000000U
		&& (profile->matchended & 0xFF800000U) == 0x80000000U;
}

static int GE_IsRandomEye(void)
{
	return romptr != 0
		&& EMU_ReadROM(0x10) == 0xB72EDF71
		&& EMU_ReadROM(0x14) == 0xC22234D1;
}

static const GE_ADDRESS_PROFILE *GE_GetAddressProfile(void)
{
	static GE_ADDRESS_PROFILE resolved;
	static unsigned int cachedcrc1 = 0;
	static unsigned int cachedcrc2 = 0;
	static int initialized = 0;
	static int valid = 0;
	unsigned int crc1;
	unsigned int crc2;

	if(romptr == 0)
		return &GE_RETAIL_ADDRESSES;

	crc1 = EMU_ReadROM(0x10);
	crc2 = EMU_ReadROM(0x14);
	if(!initialized || crc1 != cachedcrc1 || crc2 != cachedcrc2)
	{
		initialized = 1;
		cachedcrc1 = crc1;
		cachedcrc2 = crc2;
		valid = GE_ResolveAddressProfile(&resolved);
	}

	return valid ? &resolved : &GE_RETAIL_ADDRESSES;
}

// GOLDENEYE ADDRESSES - OFFSET ADDRESSES BELOW (REQUIRES PLAYERBASE TO USE)
#define GE_stanceflag 0x800D2FFC - 0x800D2F60
#define GE_deathflag 0x800D3038 - 0x800D2F60
#define GE_camx 0x800D30A8 - 0x800D2F60
#define GE_camy 0x800D30B8 - 0x800D2F60
#define GE_fov 0x800D4124 - 0x800D2F60
#define GE_crosshairx 0x800D3F50 - 0x800D2F60
#define GE_crosshairy 0x800D3F54 - 0x800D2F60
#define GE_watch 0x800D3128 - 0x800D2F60
#define GE_gunx 0x800D3F64 - 0x800D2F60
#define GE_guny 0x800D3F68 - 0x800D2F60
#define GE_aimingflag 0x800D3084 - 0x800D2F60
#define GE_currentweapon 0x800D37D0 - 0x800D2F60
#define GE_multipausemenu 0x800A9D24 - 0x800A7360
// STATIC ADDRESSES BELOW
#define BONDDATA(X) (unsigned int)EMU_ReadInt(GE_GetAddressProfile()->bonddata + (X * 0x4)) // player pointer address (0x4 offset for each players)
#define GE_camera (GE_GetAddressProfile()->camera) // camera flag (0 = multiplayer, 1 = map overview, 2 = start flyby, 3 = in flyby, 4 = player in control, 5 = trigger restart map)
#define GE_exit (GE_GetAddressProfile()->exit) // exit flag (0 = disable controls, 1 = enable controls)
#define GE_pause (GE_GetAddressProfile()->pause) // pause flag (1 = GE is paused)
#define GE_menupage (GE_GetAddressProfile()->menupage) // menu page id
#define GE_menux (GE_GetAddressProfile()->menux) // crosshair menu cursor x axis
#define GE_menuy (GE_GetAddressProfile()->menuy) // crosshair menu cursor y axis
#define GE_tankxrot (GE_GetAddressProfile()->tankxrot) // tank x rotation
#define GE_tankflag (GE_GetAddressProfile()->tankflag) // tank flag (0 = walking, 1 = in-tank)
#define GE_matchended (GE_GetAddressProfile()->matchended) // multiplayer match flag
#define GE_defaultratio 0x80055264 // 16:9 ratio default
#define GE_defaultratiocrosshair 0x0009F198 // 16:9 crosshair ratio default (rom)
#define GE_defaultfov 0x000B78BC // field of view default (rom)
#define GE_defaultfovinit 0x000CF838 // field of view init value (rom)
#define GE_defaultfovzoom 0x000B78DC // field of view default for zoom (rom)
#define GE_defaultzoomspeed 0x8004F1A8 // default zoom speed
#define GE_showcrosshair 0x0009F128 // show crosshair code (rom)
#define GE_crosshairimage 0x0029DE8C // crosshair image (rom)
#define GE_introcounter (GE_GetAddressProfile()->introcounter) // counter for intro
#define GE_seenintroflag (GE_GetAddressProfile()->seenintroflag) // seen intro flag
#define GE_controlstyle 0x000D98FC // instruction reads the current controller style (rom)
#define GE_reversepitch 0x000D9970 // instruction reads the current reverse pitch option (rom)
#define GE_pickupyaxisthreshold 0x800532E0 // y axis threshold on picking up weapons
#define GE_weaponypos 0x8003249C // y axis position for view models
#define GE_weaponzpos (GE_weaponypos + 4) // z axis position for view models

static unsigned int playerbase[4] = {0}; // current player's bonddata address
static int safetocrouch[4] = {1, 1, 1, 1}, safetostand[4] = {0}, crouchstance[4] = {0}; // used for crouch toggle (limits tick-tocking)
static float crosshairposx[4], crosshairposy[4], aimx[4], aimy[4];

int GE_Status(void);
void GE_Inject(void);
static void GE_Crouch(const int player);
#define GE_ResetCrouchToggle(X) safetocrouch[X] = 1, safetostand[X] = 0, crouchstance[X] = 0 // reset crouch toggle bind
static void GE_AimMode(const int player, const int aimingflag, const float fov, const float basefov);
static void GE_Controller(void);
static void GE_InjectHacks(void);
void GE_Quit(void);

static const GAMEDRIVER GAMEDRIVER_INTERFACE =
{
	"GoldenEye 007",
	GE_Status,
	GE_Inject,
	GE_Quit
};

const GAMEDRIVER *GAME_GOLDENEYE007 = &GAMEDRIVER_INTERFACE;

//==========================================================================
// Purpose: returns a value, which is then used to check what game is running in game.c
// Q: What is happening here?
// A: We look up some static addresses and if the values are within the expected ranges the program can assume that the game is currently running
//==========================================================================
int GE_Status(void)
{
	const int ge_max_page = 27;
	const int ge_camera = EMU_ReadInt(GE_camera), ge_page = EMU_ReadInt(GE_menupage), ge_pause = EMU_ReadInt(GE_pause), ge_exit = EMU_ReadInt(GE_exit);
	const float ge_crosshairx = EMU_ReadFloat(GE_menux), ge_crosshairy = EMU_ReadFloat(GE_menuy);
	return (ge_camera >= 0 && ge_camera <= 10 && ge_page >= -1 && ge_page <= ge_max_page && ge_pause >= 0 && ge_pause <= 1 && ge_exit >= 0 && ge_exit <= 1 && ge_crosshairx >= 20 && ge_crosshairx <= 420 && ge_crosshairy >= 20 && ge_crosshairy <= 310); // if GoldenEye 007 is current game
}
//==========================================================================
// Purpose: calculate mouse movement and inject into current game
// Changes Globals: safetocrouch, safetostand, crouchstance
// Q: Could you explain !aimingflag ? 10.0f : 40.0f
// A: While the player is aiming weapon sway will be reduced by 75%. While this is not in the original design of GE/PD I feel it gives a legitimate advantage for aiming instead of only making the crosshair visible
// Q: Could you explain basefov = fov > 60.0f ? (float)OVERRIDEFOV : 60.0f
// A: For weapons that zoom, the fov is lower than 60 - when this happens we compute our calculations using 60 as a base instead of the override fov. This is to prevent weapons from becoming too sluggish to move while zoomed with a high override fov
// Q: Could you explain if(aimingflag) gunx /= emuoverclock ? 1.03f : 1.07f, crosshairx /= emuoverclock ? 1.03f : 1.07f;
// A: GE_InjectHacks() disables the engine from overwriting the crosshair pos and gun rot - this is so aiming is jitter free. But it removed the auto centering code while aiming. If the player turns off cursor aiming for GE, the crosshair and gun will no longer move back to the center (it'll float to the corners of the screen). This if statement will emulate the centering code. The emuoverclock condition will make the scale the same - regardless of overclock or stock (inject exec at higher tickrate if overclocked).
//==========================================================================
void GE_Inject(void)
{
	if(EMU_ReadInt(GE_menupage) < 1) // hacks can only be injected at boot sequence before code blocks are cached, so inject until the main menu
		GE_InjectHacks();
	const int camera = EMU_ReadInt(GE_camera);
	const int exit = EMU_ReadInt(GE_exit);
	const int pause = EMU_ReadInt(GE_pause);
	const int menupage = EMU_ReadInt(GE_menupage);
	const int tankflag = EMU_ReadInt(GE_tankflag);
	const int mproundend = EMU_ReadInt(GE_matchended);
	for(int player = PLAYER1; player < ALLPLAYERS; player++)
	{
		if(PROFILE[player].SETTINGS[CONFIG] == DISABLED) // bypass disabled players
			continue;
		playerbase[player] = BONDDATA(player);
		const int dead = EMU_ReadInt(playerbase[player] + GE_deathflag);
		const int watch = EMU_ReadInt(playerbase[player] + GE_watch);
		const int aimingflag = EMU_ReadInt(playerbase[player] + GE_aimingflag);
		const int mppausemenu = EMU_ReadInt(playerbase[player] + GE_multipausemenu);
		const int cursoraimingflag = PROFILE[player].SETTINGS[GEAIMMODE] && aimingflag;
		const float fov = EMU_ReadFloat(playerbase[player] + GE_fov);
		const float basefov = fov > 60.0f ? (float)OVERRIDEFOV : 60.0f;
		const float mouseaccel = PROFILE[player].SETTINGS[ACCELERATION] ? sqrt(DEVICE[player].XPOS * DEVICE[player].XPOS + DEVICE[player].YPOS * DEVICE[player].YPOS) / TICKRATE / 12.0f * PROFILE[player].SETTINGS[ACCELERATION] : 0;
		const float sensitivity = PROFILE[player].SETTINGS[SENSITIVITY] / 40.0f * fmax(mouseaccel, 1);
		const float gunsensitivity = sensitivity * (PROFILE[player].SETTINGS[CROSSHAIR] / 2.5f);
		float camx = EMU_ReadFloat(playerbase[player] + GE_camx), camy = EMU_ReadFloat(playerbase[player] + GE_camy);
		if(camx >= 0 && camx <= 360 && camy >= -90 && camy <= 90 && fov >= 1 && fov <= FOV_MAX && dead == 0 && watch == 0 && pause == 0 && (camera == 4 || camera == 0) && exit == 1 && menupage == 11 && !mproundend && !mppausemenu) // if safe to inject
		{
			GE_AimMode(player, cursoraimingflag, fov, basefov);
			if(!tankflag) // player is on foot
			{
				GE_Crouch(player); // only allow crouching if player is not in tank
				if(!cursoraimingflag) // if not aiming (or geaimmode is off)
					camx += DEVICE[player].XPOS / 10.0f * sensitivity * (fov / basefov); // regular mouselook calculation
				else
					camx += aimx[player] * (fov / basefov); // scroll screen with aimx/aimy
				while(camx < 0)
					camx += 360;
				while(camx >= 360)
					camx -= 360;
				EMU_WriteFloat(playerbase[player] + GE_camx, camx);
			}
			else // player is in tank
			{
				GE_ResetCrouchToggle(player); // reset crouch toggle if in tank
				float tankx = EMU_ReadFloat(GE_tankxrot);
				if(!cursoraimingflag || EMU_ReadInt(playerbase[player] + GE_currentweapon) == 32) // if not aiming (or geaimmode is off) or player is driving tank with tank equipped as weapon, then use regular mouselook calculation
					tankx += DEVICE[player].XPOS / 10.0f * sensitivity / (360 / TANKXROTATIONLIMIT * 2.5) * (fov / basefov);
				else
					tankx += aimx[player] / (360 / TANKXROTATIONLIMIT * 2.5) * (fov / basefov);
				while(tankx < 0)
					tankx += TANKXROTATIONLIMIT;
				while(tankx >= TANKXROTATIONLIMIT)
					tankx -= TANKXROTATIONLIMIT;
				EMU_WriteFloat(GE_tankxrot, tankx);
			}
			if(!cursoraimingflag)
				camy += (!PROFILE[player].SETTINGS[INVERTPITCH] ? -DEVICE[player].YPOS : DEVICE[player].YPOS) / 10.0f * sensitivity * (fov / basefov);
			else
				camy += -aimy[player] * (fov / basefov);
			camy = ClampFloat(camy, tankflag ? -20 : -90, 90); // tank limits player from looking down -20
			EMU_WriteFloat(playerbase[player] + GE_camy, camy);
			if(PROFILE[player].SETTINGS[CROSSHAIR] && !cursoraimingflag) // if crosshair movement is enabled and player isn't aiming (don't calculate weapon movement while the player is in aim mode)
			{
				if(!tankflag)
				{
					float gunx = EMU_ReadFloat(playerbase[player] + GE_gunx), crosshairx = EMU_ReadFloat(playerbase[player] + GE_crosshairx); // after camera x and y have been calculated and injected, calculate the gun/crosshair movement
					gunx += DEVICE[player].XPOS / (!aimingflag ? 10.0f : 40.0f) * gunsensitivity * (fov / basefov) * 0.019f;
					crosshairx += DEVICE[player].XPOS / (!aimingflag ? 10.0f : 40.0f) * gunsensitivity * (fov / 4 / (basefov / 4)) * 0.01912f / RATIOFACTOR;
					if(aimingflag) // emulate cursor moving back to the center
						gunx /= emuoverclock ? 1.03f : 1.07f, crosshairx /= emuoverclock ? 1.03f : 1.07f;
					gunx = ClampFloat(gunx, -GUNAIMLIMIT, GUNAIMLIMIT);
					crosshairx = ClampFloat(crosshairx, -CROSSHAIRLIMIT, CROSSHAIRLIMIT);
					EMU_WriteFloat(playerbase[player] + GE_gunx, gunx);
					EMU_WriteFloat(playerbase[player] + GE_crosshairx, crosshairx);
				}
				if((!tankflag && camy > -90 || tankflag && camy > -20) && camy < 90) // only allow player's gun to pitch within a valid range
				{
					float guny = EMU_ReadFloat(playerbase[player] + GE_guny), crosshairy = EMU_ReadFloat(playerbase[player] + GE_crosshairy);
					guny += (!PROFILE[player].SETTINGS[INVERTPITCH] ? DEVICE[player].YPOS : -DEVICE[player].YPOS) / (!aimingflag ? 40.0f : 20.0f) * gunsensitivity * (fov / basefov) * 0.025f;
					crosshairy += (!PROFILE[player].SETTINGS[INVERTPITCH] ? DEVICE[player].YPOS : -DEVICE[player].YPOS) / (!aimingflag ? 40.0f : 20.0f) * gunsensitivity * (fov / 4 / (basefov / 4)) * 0.0225f;
					if(aimingflag)
						guny /= emuoverclock ? 1.15f : 1.35f, crosshairy /= emuoverclock ? 1.15f : 1.35f;
					guny = ClampFloat(guny, -GUNAIMLIMIT, GUNAIMLIMIT);
					crosshairy = ClampFloat(crosshairy, -CROSSHAIRLIMIT, CROSSHAIRLIMIT);
					EMU_WriteFloat(playerbase[player] + GE_guny, guny);
					EMU_WriteFloat(playerbase[player] + GE_crosshairy, crosshairy);
				}
			}
		}
		else if(player == PLAYER1 && menupage != 11 && menupage != 23) // if user is in menu (only player 1 can control menu)
		{
			float menucrosshairx = EMU_ReadFloat(GE_menux), menucrosshairy = EMU_ReadFloat(GE_menuy);
			menucrosshairx += DEVICE[player].XPOS / 10.0f * sensitivity * 6;
			menucrosshairy += DEVICE[player].YPOS / 10.0f * sensitivity * (400.0f / 290.0f * 6); // y is a little weaker then x in the menu so add more power to make it feel even with x axis
			menucrosshairx = ClampFloat(menucrosshairx, 20, 420);
			menucrosshairy = ClampFloat(menucrosshairy, 20, 310);
			EMU_WriteFloat(GE_menux, menucrosshairx);
			EMU_WriteFloat(GE_menuy, menucrosshairy);
		}
		if(dead || menupage != 11) // if player is dead or in menu, reset crouch toggle
			GE_ResetCrouchToggle(player);
	}
	GE_Controller(); // set controller data
}
//==========================================================================
// Purpose: crouching function for GoldenEye (2 = stand, 1 = kneel (in tank), 0 = crouch)
// Changes Globals: safetocrouch, crouchstance, safetostand
//==========================================================================
static void GE_Crouch(const int player)
{
	int crouchheld = DEVICE[player].BUTTONPRIM[CROUCH] || DEVICE[player].BUTTONSEC[CROUCH] || DEVICE[player].BUTTONPRIM[KNEEL] || DEVICE[player].BUTTONSEC[KNEEL];
	if(PROFILE[player].SETTINGS[CROUCHTOGGLE]) // check and toggle player stance
	{
		if(safetocrouch[player] && crouchheld) // standing to crouching
			safetocrouch[player] = 0, crouchstance[player] = 1;
		else if(!safetocrouch[player] && !crouchheld) // crouch is no longer being held, ready to stand
			safetostand[player] = 1;
		if(safetostand[player] && crouchheld) // stand up
			safetocrouch[player] = 1, crouchstance[player] = 0;
		else if(safetostand[player] && safetocrouch[player] && !crouchheld) // crouch key not active, ready to toggle
			safetostand[player] = 0;
		crouchheld = crouchstance[player];
	}
	EMU_WriteInt(playerbase[player] + GE_stanceflag, !crouchheld ? 2 : 0); // set in-game stance
}
//==========================================================================
// Purpose: replicate the original aiming system, uses aimx/y to move screen when crosshair is on border of screen
// Changes Globals: crosshairposx, crosshairposy, aimx, aimy
//==========================================================================
static void GE_AimMode(const int player, const int aimingflag, const float fov, const float basefov)
{
	const float crosshairx = EMU_ReadFloat(playerbase[player] + GE_crosshairx), crosshairy = EMU_ReadFloat(playerbase[player] + GE_crosshairy), offsetpos[2][33] = {{0, 0, 0, 0, 0.1625, 0.1625, 0.15, 0.5, 0.8, 0.4, 0.5, 0.5, 0.48, 0.9, 0.25, 0.6, 0.6, 0.7, 0.25, 0.15, 0.1625, 0.1625, 0.5, 0.5, 0.9, 0.9, 0, 0, 0, 0, 0, 0.4}, {0, 0, 0, 0, 0.1, 0.1, 0.2, 0.325, 1, 0.3, 0.425, 0.425, 0.45, 0.95, 0.1, 0.55, 0.5, 0.7, 0.25, 0.1, 0.1, 0.1, 0.275, 1, 0.9, 0.8, 0, 0, 0, 0, 0, 0.25}}; // table of X/Y offset for weapons
	const int currentweapon = EMU_ReadInt(playerbase[player] + GE_currentweapon);
	const float fovratio = fov / basefov, fovmodifier = basefov / 60.f; // basefov is 60 unless override is above 60
	const float threshold = 0.72f, speed = 475.f, sensitivity = 292.f * fovmodifier;
	const int aimingintank = EMU_ReadInt(GE_tankflag) == 1 && currentweapon == 32; // flag if player is driving tank with tank equipped as weapon
	if(aimingflag) // if player is aiming
	{
		const float mouseaccel = PROFILE[player].SETTINGS[ACCELERATION] ? sqrt(DEVICE[player].XPOS * DEVICE[player].XPOS + DEVICE[player].YPOS * DEVICE[player].YPOS) / TICKRATE / 12.0f * PROFILE[player].SETTINGS[ACCELERATION] : 0;
		crosshairposx[player] += DEVICE[player].XPOS / 10.0f * (PROFILE[player].SETTINGS[SENSITIVITY] / sensitivity / RATIOFACTOR) * fmax(mouseaccel, 1); // calculate the crosshair position
		crosshairposy[player] += (!PROFILE[player].SETTINGS[INVERTPITCH] ? DEVICE[player].YPOS : -DEVICE[player].YPOS) / 10.0f * (PROFILE[player].SETTINGS[SENSITIVITY] / sensitivity) * fmax(mouseaccel, 1);
		crosshairposx[player] = ClampFloat(crosshairposx[player], -CROSSHAIRLIMIT, CROSSHAIRLIMIT); // apply clamp then inject
		crosshairposy[player] = ClampFloat(crosshairposy[player], -CROSSHAIRLIMIT, CROSSHAIRLIMIT);
		if(aimingintank) // if player is aiming while driving tank with tank equipped as weapon, set x axis crosshair to 0 (like the original game - so you cannot aim across the screen because the tank barrel is locked in the center)
			crosshairposx[player] = 0;
		EMU_WriteFloat(playerbase[player] + GE_crosshairx, crosshairposx[player]);
		EMU_WriteFloat(playerbase[player] + GE_crosshairy, crosshairposy[player]);
		EMU_WriteFloat(playerbase[player] + GE_gunx, crosshairposx[player] * RATIOFACTOR * (1.11f + (currentweapon >= 0 && currentweapon <= 32 ? offsetpos[0][currentweapon] : 0.15f) * 1.5f) + fovratio - 1); // calculate and inject the gun angles (uses pre-made pos table or if unknown weapon use fail-safe value)
		EMU_WriteFloat(playerbase[player] + GE_guny, crosshairposy[player] * (1.11f + (currentweapon >= 0 && currentweapon <= 32 ? offsetpos[1][currentweapon] : 0) * 1.5f) + fovratio - 1);
		if(crosshairx > 0 && crosshairx / CROSSHAIRLIMIT > threshold) // if crosshair is within threshold of the border then calculate a linear scrolling speed and enable mouselook
			aimx[player] = (crosshairx / CROSSHAIRLIMIT - threshold) * speed * TIMESTEP;
		else if(crosshairx < 0 && crosshairx / CROSSHAIRLIMIT < -threshold)
			aimx[player] = (crosshairx / CROSSHAIRLIMIT + threshold) * speed * TIMESTEP;
		else
			aimx[player] = 0;
		if(crosshairy > 0 && crosshairy / CROSSHAIRLIMIT > threshold)
			aimy[player] = (crosshairy / CROSSHAIRLIMIT - threshold) * speed * TIMESTEP;
		else if(crosshairy < 0 && crosshairy / CROSSHAIRLIMIT < -threshold)
			aimy[player] = (crosshairy / CROSSHAIRLIMIT + threshold) * speed * TIMESTEP;
		else
			aimy[player] = 0;
	}
	else // player is not aiming so reset crosshairposxy values
		crosshairposx[player] = crosshairx, crosshairposy[player] = crosshairy;
}
//==========================================================================
// Purpose: calculate and send emulator key combo
//==========================================================================
static void GE_Controller(void)
{
	const int randomeye = GE_IsRandomEye();
	for(int player = PLAYER1; player < ALLPLAYERS; player++)
	{
		const int forwards = DEVICE[player].BUTTONPRIM[FORWARDS] || DEVICE[player].BUTTONSEC[FORWARDS];
		const int backwards = DEVICE[player].BUTTONPRIM[BACKWARDS] || DEVICE[player].BUTTONSEC[BACKWARDS];
		CONTROLLER[player].U_CBUTTON = forwards;
		/* RandomEye enables GoldenEye's C-Up + C-Down debug-menu shortcut.
		 * Give forward movement priority when both directions are held, matching
		 * the retail game's effective movement behaviour without opening it. */
		CONTROLLER[player].D_CBUTTON = backwards && !(randomeye && preventrandomeyedebugshortcut && forwards);
		CONTROLLER[player].L_CBUTTON = DEVICE[player].BUTTONPRIM[STRAFELEFT] || DEVICE[player].BUTTONSEC[STRAFELEFT];
		CONTROLLER[player].R_CBUTTON = DEVICE[player].BUTTONPRIM[STRAFERIGHT] || DEVICE[player].BUTTONSEC[STRAFERIGHT];
		CONTROLLER[player].Z_TRIG = DEVICE[player].BUTTONPRIM[FIRE] || DEVICE[player].BUTTONSEC[FIRE] || DEVICE[player].BUTTONPRIM[PREVIOUSWEAPON] || DEVICE[player].BUTTONSEC[PREVIOUSWEAPON];
		CONTROLLER[player].R_TRIG = DEVICE[player].BUTTONPRIM[AIM] || DEVICE[player].BUTTONSEC[AIM];
#ifndef SPEEDRUN_BUILD // speedrun build does not have reload button support
		CONTROLLER[player].RELOAD_HACK = DEVICE[player].BUTTONPRIM[RELOAD] || DEVICE[player].BUTTONSEC[RELOAD];
#endif
		CONTROLLER[player].A_BUTTON = DEVICE[player].BUTTONPRIM[ACCEPT] || DEVICE[player].BUTTONSEC[ACCEPT] || DEVICE[player].BUTTONPRIM[PREVIOUSWEAPON] || DEVICE[player].BUTTONSEC[PREVIOUSWEAPON] || DEVICE[player].BUTTONPRIM[NEXTWEAPON] || DEVICE[player].BUTTONSEC[NEXTWEAPON];
		CONTROLLER[player].B_BUTTON = DEVICE[player].BUTTONPRIM[CANCEL] || DEVICE[player].BUTTONSEC[CANCEL];
		CONTROLLER[player].START_BUTTON = DEVICE[player].BUTTONPRIM[START] || DEVICE[player].BUTTONSEC[START];
		DEVICE[player].ARROW[0] = (DEVICE[player].BUTTONPRIM[UP] || DEVICE[player].BUTTONSEC[UP]) ? 127 : 0;
		DEVICE[player].ARROW[1] = (DEVICE[player].BUTTONPRIM[DOWN] || DEVICE[player].BUTTONSEC[DOWN]) ? (EMU_ReadInt(GE_menupage) != 11 ? -127 : -128) : 0; // clamp to -127 for menus due to overflow bug
		DEVICE[player].ARROW[2] = (DEVICE[player].BUTTONPRIM[LEFT] || DEVICE[player].BUTTONSEC[LEFT]) ? -128 : 0;
		DEVICE[player].ARROW[3] = (DEVICE[player].BUTTONPRIM[RIGHT] || DEVICE[player].BUTTONSEC[RIGHT]) ? 127 : 0;
		CONTROLLER[player].X_AXIS = DEVICE[player].ARROW[0] + DEVICE[player].ARROW[1];
		CONTROLLER[player].Y_AXIS = DEVICE[player].ARROW[2] + DEVICE[player].ARROW[3];
	}
	if(EMU_ReadInt(GE_menupage) != 11 && !CONTROLLER[PLAYER1].B_BUTTON) // pressing aim will act like the back button (only in menus and for player 1)
		CONTROLLER[PLAYER1].B_BUTTON = DEVICE[PLAYER1].BUTTONPRIM[AIM] || DEVICE[PLAYER1].BUTTONSEC[AIM];
}
//==========================================================================
// Purpose: inject hacks into rom before code has been cached
//==========================================================================
static int GE_RetailHacksAreSafe(void)
{
	static const unsigned int addresses[27] = {0x000B7EA0, 0x000B7EB8, 0x0009C7F8, 0x0009C7FC, 0x0009C80C, 0x0009C810, 0x0009C998, 0x0009C99C, 0x0009C9AC, 0x0009C9B0, 0x000AE4DC, 0x000AE4E0, 0x000AE4E4, 0x000AE4E8, 0x000AE4EC, 0x000AE4F0, 0x000AE4F4, 0x000AE4F8, 0x000AE4FC, 0x000AE500, 0x000AE504, 0x000AE508, 0x000AE50C, 0x000AE510, 0x000AE514, 0x000AE518, 0x000AE51C};
	static const unsigned int expected[27] = {0x0FC1E66B, 0x0FC1E66B, 0x460C5100, 0xE4440FF0, 0x460E3280, 0xE44A0FF4, 0x460C4100, 0xE4441004, 0x460E5200, 0xE4481008, 0x3C058008, 0x24A5A0B0, 0x8CA20000, 0x8C4E009C, 0x01C47821, 0xAC4F009C, 0x8CA20000, 0x8C43009C, 0x04610003, 0x28610003, 0x03E00008, 0xAC40009C, 0x14200002, 0x24180002, 0xAC58009C, 0x03E00008, 0x00000000};

	for(int index = 0; index < 27; index++)
	{
		if(EMU_ReadROM(addresses[index]) != expected[index])
			return 0;
	}

	return 1;
}

#ifndef SPEEDRUN_BUILD
static void GE_InjectReloadHack(void)
{
	const GE_RELOAD_HACK_PROFILE *profile = GE_GetReloadHackProfile();
	unsigned int playercode[7];
	unsigned int logiccode[11];
	if(profile == 0)
		return;

	playercode[0] = 0x330D4000;
	playercode[1] = 0x51A00091;
	playercode[2] = 0x8E0D0000;
	playercode[3] = 0x3C020000 | profile->playerhigh;
	playercode[4] = 0x8C4D0000 | profile->playerlow;
	playercode[5] = 0x11A0002D;
	playercode[6] = 0x8C4C0000 | profile->controllow;

	logiccode[0] = 0x8E020000;
	logiccode[1] = 0x51600005;
	logiccode[2] = 0x00000000;
	logiccode[3] = profile->weaponstatecall;
	logiccode[4] = 0x00002025;
	logiccode[5] = profile->weaponstatecall;
	logiccode[6] = 0x24040001;
	logiccode[7] = 0x11400003;
	logiccode[8] = 0x00000000;
	logiccode[9] = profile->reloadcall;
	logiccode[10] = 0x00000000;

	EMU_WriteROM(profile->reloadflag, 0x8FAB01C8);
	EMU_WriteROM(profile->inputmask, 0x304F4040);
	for(int index = 0; index < 7; index++)
		EMU_WriteROM(profile->playercheck + index * 4, playercode[index]);
	EMU_WriteROM(profile->weaponstate, 0x8C4200D0);
	EMU_WriteROM(profile->weaponstate + 0x04, 0x304B0040);
	EMU_WriteROM(profile->weaponstate + 0x0C, 0x304A4000);
	for(int index = 0; index < 11; index++)
		EMU_WriteROM(profile->reloadlogic + index * 4, logiccode[index]);
}
#endif

static void GE_InjectHacks(void)
{
#ifndef SPEEDRUN_BUILD
	GE_InjectReloadHack();
#endif

	if(!GE_RetailHacksAreSafe())
	{
		if(CONTROLLER[PLAYER1].Z_TRIG && CONTROLLER[PLAYER1].R_TRIG)
		{
			EMU_WriteInt(GE_introcounter, 0x00001000);
			EMU_WriteInt(GE_seenintroflag, 0);
		}
		return;
	}

	const int addressarray[27] = {0x000B7EA0, 0x000B7EB8, 0x0009C7F8, 0x0009C7FC, 0x0009C80C, 0x0009C810, 0x0009C998, 0x0009C99C, 0x0009C9AC, 0x0009C9B0, 0x000AE4DC, 0x000AE4E0, 0x000AE4E4, 0x000AE4E8, 0x000AE4EC, 0x000AE4F0, 0x000AE4F4, 0x000AE4F8, 0x000AE4FC, 0x000AE500, 0x000AE504, 0x000AE508, 0x000AE50C, 0x000AE510, 0x000AE514, 0x000AE518, 0x000AE51C}, codearray[27] = {0x00000000, 0x00000000, 0x0BC1E66B, 0x460C5100, 0x0BC1E66F, 0x460E3280, 0x0BC1E673, 0x460C4100, 0x0BC1E677, 0x460E5200, 0x8C590124, 0x53200001, 0xE4440FF0, 0x0BC19F34, 0x8C590124, 0x53200001, 0xE44A0FF4, 0x0BC19F39, 0x8C590124, 0x53200001, 0xE4441004, 0x0BC19F9C, 0x8C590124, 0x53200001, 0xE4481008, 0x0BC19FA1, 0x00000000}; // disable autostand code, add branch to crosshair code so cursor aiming mode is absolute (without jitter)
	for(int index = 0; index < 27; index++) // inject code array
		EMU_WriteROM(addressarray[index], codearray[index]);
#ifndef SPEEDRUN_BUILD // gives unfair advantage, remove for speedrun build
	if((unsigned int)EMU_ReadROM(GE_controlstyle) == 0x8DC22A58) // if safe to overwrite
		EMU_WriteROM(GE_controlstyle, 0x34020001); // always force game to use 1.2 control style
	if((unsigned int)EMU_ReadROM(GE_reversepitch) == 0x8C420A84) // if safe to overwrite
		EMU_WriteROM(GE_reversepitch, 0x34020001); // always force game to use upright pitch
	if((unsigned int)EMU_ReadInt(GE_pickupyaxisthreshold) == 0xBF490FDB && EMU_ReadInt(GE_menupage) == 0) // if safe to overwrite
		EMU_WriteFloat(GE_pickupyaxisthreshold, -60.f * PI / 180.f); // overwrite default y axis limit for picking up items (from -45 to -60)
	if(OVERRIDEFOV != 60) // override default fov
	{
		float newfov = OVERRIDEFOV;
		unsigned int unsignedinteger = *(unsigned int *)(float *)(&newfov);
		EMU_WriteROM(GE_defaultfov, 0x3C010000 + (short)(unsignedinteger / 0x10000));
		EMU_WriteROM(GE_defaultfovinit, 0x3C010000 + (short)(unsignedinteger / 0x10000));
		EMU_WriteROM(GE_defaultfovzoom, 0x3C010000 + (short)(unsignedinteger / 0x10000));
		if(!bypassviewmodelfovtweak) // allow user to bypass viewmodel position compensation for override fov (so they can see more detail at the expense of introducing draw order glitches)
		{
			if(EMU_ReadInt(GE_weaponypos) == 0 && EMU_ReadInt(GE_weaponzpos) == 0) // if first weapon slot position is default
			{
				for(int index = 0; index <= 32; index++) // cycle through first 32 weapons
				{
					const float fovoffset = OVERRIDEFOV - 60;
					const float weaponypos = EMU_ReadFloat(GE_weaponypos + (index * 0x70)) - (fovoffset / (2.25f * 4.f)); // adjust weapon Y/Z positions for override field of view
					const float weaponzpos = EMU_ReadFloat(GE_weaponzpos + (index * 0x70)) + (fovoffset / 2.75f);
					EMU_WriteFloat(GE_weaponypos + (index * 0x70), weaponypos);
					EMU_WriteFloat(GE_weaponzpos + (index * 0x70), weaponzpos);
				}
			}
		}
		if(OVERRIDEFOV > 60)
			EMU_WriteFloat(GE_defaultzoomspeed, (OVERRIDEFOV - 60) * ((1.7f - 0.909091f) / 60.0f) + 0.909091f); // adjust zoom speed default (0.909091 default, 1.7 max)
	}
	if((unsigned int)EMU_ReadInt(GE_defaultratio) == 0x3FE38E39 && (overrideratiowidth != 16 || overrideratioheight != 9)) // override default 16:9 ratio
	{
		EMU_WriteFloat(GE_defaultratio, (float)overrideratiowidth / (float)overrideratioheight); // apply new ratio
		float newratio = (4.f / 3.f) / ((float)overrideratiowidth / (float)overrideratioheight); // now apply ratio factor to crosshair scale
		unsigned int unsignedinteger = *(unsigned int *)(float *)(&newratio);
		EMU_WriteROM(GE_defaultratiocrosshair, 0x3C010000 + (short)(unsignedinteger / 0x10000));
	}
#endif
	if(geshowcrosshair) // inject show crosshair hack
	{
		EMU_WriteROM(GE_showcrosshair, 0x8C4E01C8); // replace lw $t6, 0x1128 ($v0) (8C4E1128) with lw $t6, 0x01C8 ($v0) (8C4E01C8)
		if(EMU_ReadROM(GE_crosshairimage) == 0x000008BC && EMU_ReadROM(0x28) != 0x45522020) // if crosshair image found and rom isn't Goldfinger 64 (why? because GF64 replaced beta crosshair image with ammo icon)
			EMU_WriteROM(GE_crosshairimage, 0x000008BD); // replace crosshair image with beta crosshair
	}
	if(CONTROLLER[PLAYER1].Z_TRIG && CONTROLLER[PLAYER1].R_TRIG) // skip intros if holding down fire + aim
	{
		EMU_WriteInt(GE_introcounter, 0x00001000);
		EMU_WriteInt(GE_seenintroflag, 0);
	}
}
//==========================================================================
// Purpose: run when emulator closes rom
// Changes Globals: playerbase, safetocrouch, safetostand, crouchstance
//==========================================================================
void GE_Quit(void)
{
	for(int player = PLAYER1; player < ALLPLAYERS; player++)
	{
		playerbase[player] = 0;
		GE_ResetCrouchToggle(player);
	}
}
