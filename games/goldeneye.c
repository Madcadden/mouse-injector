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
	unsigned int maxpage;
	unsigned int menux;
	unsigned int menuy;
	unsigned int tankxrot;
	unsigned int tankflag;
	unsigned int matchended;
	unsigned int introcounter;
	unsigned int seenintroflag;
} GE_ADDRESS_PROFILE;

static const GE_ADDRESS_PROFILE GE_UNRESOLVED_ADDRESSES = {0};

/*
 * Resolve the globals used by mouse injection from stable startup-code
 * patterns. Every anchor must occur exactly once; otherwise mouse injection
 * stays disabled. A plausible retail address is not evidence of a mod layout.
 */
#define GE_ROM_SCAN_LIMIT 0x00200000

/* ROM reloads may reuse both the allocation and header checksums. */
static unsigned int ge_rom_generation = 0;

typedef struct GE_OWNED_ROM_WORD
{
	unsigned int address;
	unsigned int original;
	unsigned int applied;
} GE_OWNED_ROM_WORD;

/* There are at most 58 distinct code/texture words in the current patches.
 * Keep originals until stop, because 1964 can restart the same ROM buffer. */
static GE_OWNED_ROM_WORD ge_ownedwords[128];
static unsigned int ge_ownedcount;
static const unsigned char **ge_ownedrom;
static unsigned int ge_ownedcrc1, ge_ownedcrc2;

static void GE_WriteOwnedROM(const unsigned int address, const unsigned int value)
{
	unsigned int index;
	if(!romptr) return;
	if(ge_ownedrom != romptr || ge_ownedcrc1 != EMU_ReadROM(0x10) || ge_ownedcrc2 != EMU_ReadROM(0x14))
	{
		ge_ownedcount = 0;
		ge_ownedrom = romptr;
		ge_ownedcrc1 = EMU_ReadROM(0x10);
		ge_ownedcrc2 = EMU_ReadROM(0x14);
	}
	for(index = 0; index < ge_ownedcount && ge_ownedwords[index].address != address; index++);
	if(index == ge_ownedcount)
	{
		if(ge_ownedcount == sizeof(ge_ownedwords) / sizeof(ge_ownedwords[0])) return;
		ge_ownedwords[index].address = address;
		ge_ownedwords[index].original = EMU_ReadROM(address);
		ge_ownedcount++;
	}
	ge_ownedwords[index].applied = value;
	EMU_WriteROM(address, value);
}

static void GE_RestoreOwnedROM(void)
{
	if(romptr && ge_ownedrom == romptr && ge_ownedcrc1 == EMU_ReadROM(0x10) && ge_ownedcrc2 == EMU_ReadROM(0x14))
	{
		for(unsigned int index = 0; index < ge_ownedcount; index++)
		{
			const GE_OWNED_ROM_WORD *word = &ge_ownedwords[index];
			if(EMU_ReadROM(word->address) == word->applied)
				EMU_WriteROM(word->address, word->original);
		}
	}
	ge_ownedcount = 0;
	ge_ownedrom = 0;
}

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
	unsigned int patchaddress[23], patchoriginal[23];
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
	profile->patchaddress[0] = profile->reloadflag;
	profile->patchaddress[1] = profile->inputmask;
	for(unsigned int i = 0; i < 7; i++) profile->patchaddress[2 + i] = profile->playercheck + i * 4;
	profile->patchaddress[9] = profile->weaponstate;
	profile->patchaddress[10] = profile->weaponstate + 4;
	profile->patchaddress[11] = profile->weaponstate + 12;
	for(unsigned int i = 0; i < 11; i++) profile->patchaddress[12 + i] = profile->reloadlogic + i * 4;
	for(unsigned int i = 0; i < 23; i++) profile->patchoriginal[i] = EMU_ReadROM(profile->patchaddress[i]);
	return 1;
}

static const GE_RELOAD_HACK_PROFILE *GE_GetReloadHackProfile(void)
{
	static GE_RELOAD_HACK_PROFILE resolved;
	static unsigned int cachedcrc1 = 0;
	static unsigned int cachedcrc2 = 0;
	static int initialized = 0;
	static unsigned int cachedgeneration = 0;
	static const unsigned char **cachedrom = 0;
	static int valid = 0;
	unsigned int crc1;
	unsigned int crc2;

	if(romptr == 0)
		return 0;

	crc1 = EMU_ReadROM(0x10);
	crc2 = EMU_ReadROM(0x14);
	if(!initialized || cachedgeneration != ge_rom_generation || cachedrom != romptr || crc1 != cachedcrc1 || crc2 != cachedcrc2)
	{
		initialized = 1;
		cachedgeneration = ge_rom_generation;
		cachedrom = romptr;
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

static unsigned int GE_FindPauseAnchor(void)
{
	static const unsigned int alternate[12] = {0x3C013F80, 0x44816000, 0x24020001, 0x3C010000, 0x27BDFFC8, 0xAC220000, 0xAFB00020, 0x3C010000, 0x3C100000, 0xAC200000, 0x26100000, 0xAE020000};
	unsigned int match = 0;
	for(unsigned int offset = 0x1000; offset <= GE_ROM_SCAN_LIMIT - 48; offset += 4)
	{
		if(GE_ROMPatternMatches(offset, gepausepattern, gepausemask, 12) || GE_ROMPatternMatches(offset, alternate, gepausemask, 12))
		{
			if(match) return 0;
			match = offset;
		}
	}
	return match;
}

static unsigned int GE_FindMatchEnded(void)
{
	static const unsigned int pattern[7] = {0x3C010000, 0xAC200000, 0x3C010000, 0xAC200000, 0x3C010000, 0x03E00008, 0xAC200000};
	static const unsigned int mask[7] = {0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFF0000, 0xFFFFFFFF, 0xFFFF0000};
	unsigned int match = 0;
	for(unsigned int offset = 0x1000; offset <= GE_ROM_SCAN_LIMIT - 28; offset += 4)
	{
		if(GE_ROMPatternMatches(offset, pattern, mask, 7))
		{
			const unsigned int first = GE_MakeAddress(EMU_ReadROM(offset), EMU_ReadROM(offset + 4));
			const unsigned int second = GE_MakeAddress(EMU_ReadROM(offset + 8), EMU_ReadROM(offset + 12));
			const unsigned int third = GE_MakeAddress(EMU_ReadROM(offset + 16), EMU_ReadROM(offset + 24));
			if((first & 0xFF800003U) != 0x80000000U || second != first + 4 || third != first + 0x18)
				continue;
			if(match) return 0;
			match = first;
		}
	}
	return match;
}

static unsigned int GE_FindMenuMaxPage(const unsigned int menupage)
{
	/* The frontend constructor dispatch reads current_menu, checks its
	 * unsigned table index, then jumps through the menu table. Derive the
	 * upper bound from this code so added mod menus keep receiving input.
	 * Preserve the existing range unless one verified dispatch extends it. */
	static const unsigned int pattern[14] = {0x3C0E0000,0x8DCE0000,0x27BDFFE0,0xAFB00018,0x2DC10000,0x00808025,0x10200000,0xAFBF001C,0x000E7080,0x3C010000,0x002E0821,0x8C2E0000,0x01C00008,0};
	static const unsigned int mask[14] = {0xFFFF0000,0xFFFF0000,0xFFFFFFFF,0xFFFFFFFF,0xFFFF0000,0xFFFFFFFF,0xFFFF0000,0xFFFFFFFF,0xFFFFFFFF,0xFFFF0000,0xFFFFFFFF,0xFFFF0000,0xFFFFFFFF,0xFFFFFFFF};
	const unsigned int match = GE_FindUniqueROMPattern(pattern, mask, 14);
	unsigned int count, table, end;
	int displacement;
	if(!match || GE_MakeAddress(EMU_ReadROM(match), EMU_ReadROM(match + 4)) != menupage)
		return 27;
	count = EMU_ReadROM(match + 16) & 0xFFFFU;
	table = GE_MakeAddress(EMU_ReadROM(match + 36), EMU_ReadROM(match + 44));
	displacement = (short)(EMU_ReadROM(match + 24) & 0xFFFFU);
	/* Check the table range and the default branch's matching epilogue;
	 * reject invalid or unexpectedly large dispatches rather than treating
	 * an arbitrary small integer as a supported menu page. */
	if(count == 0 || count > 256 || (table & 0xFF800003U) != 0x80000000U
		|| (table & 0x7FFFFFU) > 0x800000U - count * 4 || displacement < 7)
		return 27;
	end = match + 28 + (unsigned int)displacement * 4;
	if(end > GE_ROM_SCAN_LIMIT - 20
		|| EMU_ReadROM(end) != 0x8FBF001C || EMU_ReadROM(end + 4) != 0x02001025
		|| EMU_ReadROM(end + 8) != 0x8FB00018 || EMU_ReadROM(end + 12) != 0x03E00008
		|| EMU_ReadROM(end + 16) != 0x27BD0020)
		return 27;
	return count > 28 ? count - 1 : 27;
}

static int GE_ResolveAddressProfile(GE_ADDRESS_PROFILE *profile)
{
	unsigned int menumatch;
	unsigned int bondmatch;
	unsigned int cameramatch;
	unsigned int pausematch;

	if(romptr == 0)
		return 0;

	menumatch = GE_FindUniqueROMPattern(gemenupattern, gemenumask, 5);
	bondmatch = GE_FindUniqueROMPattern(gebonddatapattern, gebonddatamask, 10);
	cameramatch = GE_FindUniqueROMPattern(gecamerapattern, gecameramask, 13);
	pausematch = GE_FindPauseAnchor();

	if(menumatch == 0 || bondmatch == 0 || cameramatch == 0 || pausematch == 0)
		return 0;

	profile->menupage = GE_MakeAddress(EMU_ReadROM(menumatch + 0x0C), EMU_ReadROM(menumatch + 0x10));
	profile->maxpage = GE_FindMenuMaxPage(profile->menupage);
	profile->bonddata = GE_MakeAddress(EMU_ReadROM(bondmatch + 0x10), EMU_ReadROM(bondmatch + 0x14));
	profile->camera = GE_MakeAddress(EMU_ReadROM(cameramatch + 0x2C), EMU_ReadROM(cameramatch + 0x30));
	profile->pause = GE_MakeAddress(EMU_ReadROM(pausematch + 0x1C), EMU_ReadROM(pausematch + 0x24));

	profile->exit = profile->camera + 0x1C;
	profile->menux = profile->menupage + 0x48;
	profile->menuy = profile->menupage + 0x4C;
	profile->tankxrot = profile->camera - 0x10;
	profile->tankflag = profile->camera - 0x4C;
	profile->matchended = GE_FindMatchEnded();
	profile->introcounter = profile->menupage + 0x0C;
	profile->seenintroflag = profile->menupage + 0x70;

	return (profile->bonddata & 0xFF800000U) == 0x80000000U
		&& (profile->camera & 0xFF800000U) == 0x80000000U
		&& (profile->pause & 0xFF800000U) == 0x80000000U
		&& (profile->menupage & 0xFF800000U) == 0x80000000U
		&& (profile->matchended & 0xFF800000U) == 0x80000000U;
}

static const GE_ADDRESS_PROFILE *GE_GetAddressProfile(void)
{
	static GE_ADDRESS_PROFILE resolved;
	static unsigned int cachedcrc1 = 0;
	static unsigned int cachedcrc2 = 0;
	static int initialized = 0;
	static unsigned int cachedgeneration = 0;
	static const unsigned char **cachedrom = 0;
	static int valid = 0;
	unsigned int crc1;
	unsigned int crc2;

	if(romptr == 0)
		return &GE_UNRESOLVED_ADDRESSES;

	crc1 = EMU_ReadROM(0x10);
	crc2 = EMU_ReadROM(0x14);
	if(!initialized || cachedgeneration != ge_rom_generation || cachedrom != romptr || crc1 != cachedcrc1 || crc2 != cachedcrc2)
	{
		initialized = 1;
		cachedgeneration = ge_rom_generation;
		cachedrom = romptr;
		cachedcrc1 = crc1;
		cachedcrc2 = crc2;
		valid = GE_ResolveAddressProfile(&resolved);
	}

	return valid ? &resolved : &GE_UNRESOLVED_ADDRESSES;
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
#define GE_crosshairimage 0x0029DE8C // crosshair image (rom)
#define GE_introcounter (GE_GetAddressProfile()->introcounter) // counter for intro
#define GE_seenintroflag (GE_GetAddressProfile()->seenintroflag) // seen intro flag

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
	if(GE_GetAddressProfile()->bonddata == 0)
		return 0;
	const int ge_max_page = (int)GE_GetAddressProfile()->maxpage;
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
	for(int player = PLAYER1; player < ALLPLAYERS; player++)
	{
		const int forwards = DEVICE[player].BUTTONPRIM[FORWARDS] || DEVICE[player].BUTTONSEC[FORWARDS];
		const int backwards = DEVICE[player].BUTTONPRIM[BACKWARDS] || DEVICE[player].BUTTONSEC[BACKWARDS];
		CONTROLLER[player].U_CBUTTON = forwards;
		CONTROLLER[player].D_CBUTTON = backwards;
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
typedef struct GE_HACK_PROFILE
{
	unsigned int fov[3];
	unsigned int fovword;
	unsigned int zoomspeed;
	unsigned int itemtable;
	unsigned int defaultstats;
	int viewfov;
	unsigned int controlstyle;
	unsigned int reversepitch;
	unsigned int ratio;
	unsigned int ratiocrosshair;
	unsigned int ratioword;
	unsigned int pickup;
	unsigned int showcrosshair;
	unsigned int aimaddress[27];
	unsigned int aimoriginal[27];
	unsigned int aimcode[27];
	int aimvalid;
} GE_HACK_PROFILE;

static int GE_ValidDataAddress(const unsigned int address, const unsigned int bytes)
{
	return (address & 0xFF800003U) == 0x80000000U && bytes <= 0x00800000U && address - 0x80000000U <= 0x00800000U - bytes;
}

static unsigned int GE_FindExactROMPattern(const unsigned int *pattern, const unsigned int wordcount)
{
	unsigned int mask[20];
	if(wordcount > 20) return 0;
	for(unsigned int i = 0; i < wordcount; i++) mask[i] = 0xFFFFFFFFU;
	return GE_FindUniqueROMPattern(pattern, mask, wordcount);
}

static void GE_ResolveFOV(GE_HACK_PROFILE *profile)
{
	/* Shared non-zoom/zoom path: validate both 60-degree loads together. */
	static const unsigned int pattern[13] = {0x0C000000,0,0x4614003E,0x46000306,0x3C014270,0x45000003,0,0x44816000,0,0x0C000000,0,0x0C000000,0};
	static const unsigned int mask[13] = {0xFC000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFC000000,0xFFFFFFFF,0xFC000000,0xFFFFFFFF};
	/* Player initialisation. Register allocation can change; the sequence of
	 * stores into the player's weapon/aim fields must stay intact. */
	static const unsigned int initpattern[15] = {0x3C014270,0xAC000FD0,0x8C600000,0xAC000FD4,0x8C600000,0xAC000FD8,0x8C600000,0xA0000FDC,0x8C600000,0xA0000FDD,0x8C600000,0xA0000FDE,0x8C600000,0x3C020000,0x24420000};
	static const unsigned int initmask[15] = {0xFFFFFFFF,0xFC1FFFFF,0xFFE0FFFF,0xFC1FFFFF,0xFFE0FFFF,0xFC1FFFFF,0xFFE0FFFF,0xFC00FFFF,0xFFE0FFFF,0xFC00FFFF,0xFFE0FFFF,0xFC00FFFF,0xFFE0FFFF,0xFFFF0000,0xFFFF0000};
	const unsigned int match = GE_FindUniqueROMPattern(pattern, mask, 13);
	const unsigned int init = GE_FindUniqueROMPattern(initpattern, initmask, 15);
	if(match < 0x1010 || !init || EMU_ReadROM(match - 16) != 0x3C014270 || EMU_ReadROM(match - 12) != 0x44816000 || (EMU_ReadROM(match - 8) & 0xFC1FFFFFU) != 0x1000000A || EMU_ReadROM(match - 4) != 0)
		return;
	profile->fov[0] = match - 16;
	profile->fov[1] = match + 16;
	profile->fov[2] = init;
	profile->fovword = 0x3C014270;
}

static int GE_VerifyCodeMapping(const unsigned int mapping)
{
	static const unsigned int pattern[15] = {0x0C000000,0,0x0C000000,0,0x0C000000,0,0x0C000000,0,0x0C000000,0,0x0C000000,0,0x8FBF0034,0x8FB00028,0x8FB1002C};
	static const unsigned int mask[15] = {0xFC000000,0xFFFFFFFF,0xFC000000,0xFFFFFFFF,0xFC000000,0xFFFFFFFF,0xFC000000,0xFFFFFFFF,0xFC000000,0xFFFFFFFF,0xFC000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
	const unsigned int calls = GE_FindUniqueROMPattern(pattern, mask, 15);
	const unsigned int entry = GE_FindUniqueROMPattern(gemenupattern, gemenumask, 5);
	unsigned int address;
	if(!calls || !entry) return 0;
	address = (EMU_ReadROM(calls + 24) & 0x03FFFFFFU) << 2;
	return address - entry == mapping
		&& ((EMU_ReadROM(calls) & 0x03FFFFFFU) << 2) == address + 0x110
		&& ((EMU_ReadROM(calls + 8) & 0x03FFFFFFU) << 2) == address + 0x9D0
		&& ((EMU_ReadROM(calls + 16) & 0x03FFFFFFU) << 2) == address + 0xB60;
}

static void GE_ResolveAimHack(GE_HACK_PROFILE *profile)
{
	static const unsigned int first[10] = {0x460C5100,0xE4440FF0,0x8CE20000,0xC4480FF4,0x46144182,0x460E3280,0xE44A0FF4,0x8C990000,0x0079082A,0x5420FFF3};
	static const unsigned int second[10] = {0x460C4100,0xE4441004,0x8CE20000,0xC4461008,0x46163282,0x460E5200,0xE4481008,0x8C890000,0x0069082A,0x5420FFF3};
	static const unsigned int stand[17] = {0x3C050000,0x24A50000,0x8CA20000,0x8C4E009C,0x01C47821,0xAC4F009C,0x8CA20000,0x8C43009C,0x04610003,0x28610003,0x03E00008,0xAC40009C,0x14200002,0x24180002,0xAC58009C,0x03E00008,0};
	static const unsigned int standmask[17] = {0xFFFF0000,0xFFFF0000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
	static const unsigned int calls[8] = {0x0C000000,0x2404FFFE,0x10000006,0x8E080000,0x50000004,0x8E080000,0x0C000000,0x24040002};
	static const unsigned int callmask[8] = {0xFC000000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFC1FFFFF,0xFFFFFFFF,0xFC000000,0xFFFFFFFF};
	const unsigned int x = GE_FindExactROMPattern(first, 10);
	const unsigned int y = GE_FindExactROMPattern(second, 10);
	const unsigned int body = GE_FindUniqueROMPattern(stand, standmask, 17);
	const unsigned int call = GE_FindUniqueROMPattern(calls, callmask, 8);
	unsigned int target;
	unsigned int mapping;
	if(!body || !call || EMU_ReadROM(call) != EMU_ReadROM(call + 24)) return;
	/* Derive jump targets from the game's own call, allowing either TLB ROM
	 * code or expansion-RAM code. Do not assume 0x7F000000 code addresses. */
	target = (EMU_ReadROM(call) & 0x03FFFFFFU) << 2;
	mapping = target - body;
	if(!x || !y || !GE_VerifyCodeMapping(mapping)) return;
	if(((mapping + x) >> 28) != (target >> 28) || ((mapping + y) >> 28) != (target >> 28)) return;
	profile->aimaddress[0] = call;
	profile->aimaddress[1] = call + 24;
	profile->aimcode[0] = profile->aimcode[1] = 0;
	for(unsigned int i = 0; i < 4; i++)
	{
		const unsigned int at = i < 2 ? x + i * 20 : y + (i - 2) * 20;
		profile->aimaddress[2 + i * 2] = at;
		profile->aimaddress[3 + i * 2] = at + 4;
		profile->aimcode[2 + i * 2] = 0x08000000U | (((target + i * 16) >> 2) & 0x03FFFFFFU);
		profile->aimcode[3 + i * 2] = EMU_ReadROM(at);
		profile->aimcode[10 + i * 4] = 0x8C590124;
		profile->aimcode[11 + i * 4] = 0x53200001;
		profile->aimcode[12 + i * 4] = EMU_ReadROM(at + 4);
		profile->aimcode[13 + i * 4] = 0x08000000U | (((mapping + at + 8) >> 2) & 0x03FFFFFFU);
	}
	profile->aimcode[26] = 0;
	for(unsigned int i = 0; i < 17; i++) profile->aimaddress[10 + i] = body + i * 4;
	for(unsigned int i = 0; i < 27; i++) profile->aimoriginal[i] = EMU_ReadROM(profile->aimaddress[i]);
	profile->aimvalid = 1;
}

static void GE_ResolveOptionalHacks(GE_HACK_PROFILE *profile)
{
	static const unsigned int zoom[7] = {0x3C010000,0xC4300000,0x3C010000,0xE4300000,0x3C010000,0xAC200000,0x3C010000};
	static const unsigned int zoommask[7] = {0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFF0000};
	static const unsigned int items[15] = {0x000470C0,0x01C47023,0x3C0F0000,0x25EF0000,0x000E70C0,0x01CF1821,0x8C780008,0x17000003,0,0x03E00008,0x8C62000C,0x3C020000,0x24420000,0x03E00008,0};
	static const unsigned int itemsmask[15] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFF0000,0xFFFF0000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFF0000,0xFFFF0000,0xFFFFFFFF,0xFFFFFFFF};
	static const unsigned int controls[7] = {0x3C0E0000,0x8DCE0000,0x03E00008,0x8DC22A58,0x3C030000,0x24630000,0x8C6E0000};
	static const unsigned int controlmask[7] = {0xFFFF0000,0xFFFF0000,0xFFFFFFFF,0xFFFFFFFF,0xFFFF0000,0xFFFF0000,0xFFFFFFFF};
	static const unsigned int reverse[6] = {0x3C020000,0x03E00008,0x8C420000,0x3C010000,0x03E00008,0xAC240000};
	static const unsigned int reversemask[6] = {0xFFFF0000,0xFFFFFFFF,0xFFFF0000,0xFFFF0000,0xFFFFFFFF,0xFFFF0000};
	static const unsigned int ratio[9] = {0x3C010000,0xC4280000,0x468021A0,0x460A3403,0x46128102,0,0x46082302,0x0C000000,0};
	static const unsigned int ratiomask[9] = {0xFFFF0000,0xFFFF0000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFC000000,0xFFFFFFFF};
	static const unsigned int crossratio[8] = {0x24010001,0x14410006,0x27A40054,0x3C013F40,0x44815000,0xC7A80044,0x460A4402,0xE7B00044};
	static const unsigned int crosshair[9] = {0xAFA40058,0x8C4E1128,0x55C0003B,0x8FBF003C,0x8C4F29C4,0x3C050000,0x24060004,0x15E00035,0x00003825};
	static const unsigned int crossmask[9] = {0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFF0000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF};
	static const unsigned int pickup[8] = {0x3C010000,0xC4260000,0x3C0C0000,0x4606003C,0,0x45000007,0,0x8D8C0000};
	static const unsigned int pickupmask[8] = {0xFFFF0000,0xFFFF0000,0xFFFF0000,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFF0000};
	unsigned int match;
	GE_ResolveFOV(profile);
	GE_ResolveAimHack(profile);
	match = GE_FindUniqueROMPattern(zoom, zoommask, 7);
	if(match) profile->zoomspeed = GE_MakeAddress(EMU_ReadROM(match), EMU_ReadROM(match + 4));
	match = GE_FindUniqueROMPattern(items, itemsmask, 15);
	if(match)
	{
		profile->itemtable = GE_MakeAddress(EMU_ReadROM(match + 8), EMU_ReadROM(match + 12));
		profile->defaultstats = GE_MakeAddress(EMU_ReadROM(match + 44), EMU_ReadROM(match + 48));
	}
	profile->viewfov = 60;
	match = GE_FindUniqueROMPattern(controls, controlmask, 7);
	if(match) profile->controlstyle = match + 12;
	/* Getter/setter pairs are common. The following independent upright
	 * option getter identifies the original control-options layout. */
	for(unsigned int offset = 0x1000; offset <= GE_ROM_SCAN_LIMIT - 36; offset += 4)
	{
		if(GE_ROMPatternMatches(offset, reverse, reversemask, 6) && EMU_ReadROM(offset + 8) == 0x8C420A84 && EMU_ReadROM(offset + 20) == 0xAC240A84 && EMU_ReadROM(offset + 32) == 0x8C420A90)
		{
			if(profile->reversepitch) { profile->reversepitch = 0; break; }
			profile->reversepitch = offset + 8;
		}
	}
	match = GE_FindUniqueROMPattern(ratio, ratiomask, 9);
	if(match) profile->ratio = GE_MakeAddress(EMU_ReadROM(match), EMU_ReadROM(match + 4));
	match = GE_FindExactROMPattern(crossratio, 8);
	if(match) { profile->ratiocrosshair = match + 12; profile->ratioword = 0x3C013F40; }
	match = GE_FindUniqueROMPattern(crosshair, crossmask, 9);
	if(match) profile->showcrosshair = match + 4;
	match = GE_FindUniqueROMPattern(pickup, pickupmask, 8);
	if(match) profile->pickup = GE_MakeAddress(EMU_ReadROM(match), EMU_ReadROM(match + 4));
}

static GE_HACK_PROFILE *GE_GetHackProfile(void)
{
	static GE_HACK_PROFILE profile;
	static unsigned int crc1, crc2, generation;
	static const unsigned char **cachedrom = 0;
	static int initialized = 0;
	if(!romptr) return 0;
	if(!initialized || generation != ge_rom_generation || cachedrom != romptr || crc1 != EMU_ReadROM(0x10) || crc2 != EMU_ReadROM(0x14))
	{
		GE_HACK_PROFILE empty = {0};
		profile = empty;
		initialized = 1;
		generation = ge_rom_generation;
		cachedrom = romptr;
		crc1 = EMU_ReadROM(0x10);
		crc2 = EMU_ReadROM(0x14);
		GE_ResolveOptionalHacks(&profile);
	}
	return &profile;
}

/* Code writes are prepared synchronously at the emulator's game entry,
 * before any game-code DMA or compilation. Never write live code from the
 * input thread; expansion-RAM mods then copy the already-patched ROM. */
static int GE_PatchWordIsSafe(const unsigned int address, const unsigned int original, const unsigned int value)
{
	const unsigned int word = EMU_ReadROM(address);
	return word == original || word == value;
}

#ifndef SPEEDRUN_BUILD
static void GE_AdjustViewmodels(GE_HACK_PROFILE *profile, const int fov)
{
	unsigned int stats[33];
	unsigned int count = 0;
	if(!GE_ValidDataAddress(profile->itemtable, 33 * 0x38) || !GE_ValidDataAddress(profile->defaultstats, 16) || profile->viewfov == fov) return;
	/* The item lookup code proves the stride and pointer fields; follow
	 * each item's actual stats pointer, including mods with moved/shared
	 * records. Validate the whole set before writing any positions. */
	for(unsigned int item = 0; item < 33; item++)
	{
		const unsigned int entry = profile->itemtable + item * 0x38;
		const int hidden = EMU_ReadInt(entry + 8);
		const unsigned int pointer = hidden ? profile->defaultstats : (unsigned int)EMU_ReadInt(entry + 12);
		unsigned int seen;
		if((hidden != 0 && hidden != 1) || !GE_ValidDataAddress(pointer, 16)) return;
		for(unsigned int axis = 1; axis <= 3; axis++)
		{
			const float value = EMU_ReadFloat(pointer + axis * 4);
			if(!(value >= -10000.f && value <= 10000.f)) return;
		}
		for(seen = 0; seen < count && stats[seen] != pointer; seen++);
		if(seen == count) stats[count++] = pointer;
	}
	for(unsigned int i = 0; i < count; i++)
	{
		const float difference = (float)(fov - profile->viewfov);
		EMU_WriteFloat(stats[i] + 8, EMU_ReadFloat(stats[i] + 8) - difference / 9.f);
		EMU_WriteFloat(stats[i] + 12, EMU_ReadFloat(stats[i] + 12) + difference / 2.75f);
	}
	profile->viewfov = fov;
}
#endif

#ifndef SPEEDRUN_BUILD
static void GE_InjectReloadHack(void)
{
	const GE_RELOAD_HACK_PROFILE *profile = GE_GetReloadHackProfile();
	unsigned int playercode[7];
	unsigned int logiccode[11];
	unsigned int values[23];
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

	values[0] = 0x8FAB01C8;
	values[1] = 0x304F4040;
	for(unsigned int i = 0; i < 7; i++) values[2 + i] = playercode[i];
	values[9] = 0x8C4200D0;
	values[10] = 0x304B0040;
	values[11] = 0x304A4000;
	for(unsigned int i = 0; i < 11; i++) values[12 + i] = logiccode[i];
	for(unsigned int i = 0; i < 23; i++)
		if(!GE_PatchWordIsSafe(profile->patchaddress[i], profile->patchoriginal[i], values[i])) return;
	for(unsigned int i = 0; i < 23; i++) GE_WriteOwnedROM(profile->patchaddress[i], values[i]);
}
#endif

static void GE_ApplyHacks(const int romonly)
{
	GE_HACK_PROFILE *profile = GE_GetHackProfile();
	if(!profile) return;
#ifndef SPEEDRUN_BUILD
	GE_InjectReloadHack();
#endif
	if(profile->aimvalid)
	{
		int safe = 1;
		for(unsigned int i = 0; i < 27; i++)
		{
			if(!GE_PatchWordIsSafe(profile->aimaddress[i], profile->aimoriginal[i], profile->aimcode[i])) safe = 0;
		}
		if(safe)
			for(unsigned int i = 0; i < 27; i++)
				GE_WriteOwnedROM(profile->aimaddress[i], profile->aimcode[i]);
	}
#ifndef SPEEDRUN_BUILD
	if(profile->controlstyle && GE_PatchWordIsSafe(profile->controlstyle, 0x8DC22A58, 0x34020001))
		GE_WriteOwnedROM(profile->controlstyle, 0x34020001);
	if(profile->reversepitch && GE_PatchWordIsSafe(profile->reversepitch, 0x8C420A84, 0x34020001))
		GE_WriteOwnedROM(profile->reversepitch, 0x34020001);
	if(!romonly && GE_ValidDataAddress(profile->pickup, 4) && (unsigned int)EMU_ReadInt(profile->pickup) == 0xBF490FDB && EMU_ReadInt(GE_menupage) == 0)
		EMU_WriteFloat(profile->pickup, -60.f * PI / 180.f);
	if(profile->fov[0] && OVERRIDEFOV >= FOV_MIN && OVERRIDEFOV <= FOV_MAX)
	{
		int safe = 1;
		union { float f; unsigned int u; } newfov;
		newfov.f = (float)OVERRIDEFOV;
		for(unsigned int i = 0; i < 3; i++)
			if(!GE_PatchWordIsSafe(profile->fov[i], 0x3C014270, profile->fovword) && !GE_PatchWordIsSafe(profile->fov[i], profile->fovword, 0x3C010000U | (newfov.u >> 16))) safe = 0;
		if(safe)
		{
			profile->fovword = 0x3C010000U | (newfov.u >> 16);
			for(unsigned int i = 0; i < 3; i++) GE_WriteOwnedROM(profile->fov[i], profile->fovword);
			if(!romonly && !bypassviewmodelfovtweak) GE_AdjustViewmodels(profile, OVERRIDEFOV);
			if(!romonly && OVERRIDEFOV > 60 && GE_ValidDataAddress(profile->zoomspeed, 4) && (unsigned int)EMU_ReadInt(profile->zoomspeed) == 0x3F68BA2E)
				EMU_WriteFloat(profile->zoomspeed, (OVERRIDEFOV - 60) * ((1.7f - 0.909091f) / 60.f) + 0.909091f);
		}
	}
	if(GE_ValidDataAddress(profile->ratio, 4) && profile->ratiocrosshair && EMU_ReadROM(profile->ratiocrosshair) == profile->ratioword && overrideratiowidth > 0 && overrideratioheight > 0 && (overrideratiowidth != 16 || overrideratioheight != 9))
	{
		union { float f; unsigned int u; } newratio;
		newratio.f = (4.f / 3.f) / ((float)overrideratiowidth / (float)overrideratioheight);
		profile->ratioword = 0x3C010000U | (newratio.u >> 16);
		GE_WriteOwnedROM(profile->ratiocrosshair, profile->ratioword);
		if(!romonly && (unsigned int)EMU_ReadInt(profile->ratio) == 0x3FE38E39)
			EMU_WriteFloat(profile->ratio, (float)overrideratiowidth / (float)overrideratioheight);
	}
#endif
	if(geshowcrosshair && profile->showcrosshair && GE_PatchWordIsSafe(profile->showcrosshair, 0x8C4E1128, 0x8C4E01C8))
	{
		GE_WriteOwnedROM(profile->showcrosshair, 0x8C4E01C8);
		/* Only the original retail image archive has a verified beta icon.
		 * Mods may replace or relocate that resource independently of code. */
		if(EMU_ReadROM(0x10) == 0xDCBC50D1 && EMU_ReadROM(0x14) == 0x09FD1AA3 && EMU_ReadROM(GE_crosshairimage) == 0x000008BC)
			GE_WriteOwnedROM(GE_crosshairimage, 0x000008BD);
	}
	if(!romonly && CONTROLLER[PLAYER1].Z_TRIG && CONTROLLER[PLAYER1].R_TRIG)
	{
		EMU_WriteInt(GE_introcounter, 0x00001000);
		EMU_WriteInt(GE_seenintroflag, 0);
	}
}
/* HookROM calls this on the emulation thread at the game entry point. */
void GE_PrepareROM(void)
{
	if(GE_GetAddressProfile()->bonddata != 0)
		GE_ApplyHacks(1);
}

static void GE_InjectHacks(void)
{
	GE_ApplyHacks(0);
}

//==========================================================================
// Purpose: run when emulator closes rom
// Changes Globals: playerbase, safetocrouch, safetostand, crouchstance
//==========================================================================
void GE_Quit(void)
{
	GE_RestoreOwnedROM();
	ge_rom_generation++;
	for(int player = PLAYER1; player < ALLPLAYERS; player++)
	{
		playerbase[player] = 0;
		GE_ResetCrouchToggle(player);
	}
}
