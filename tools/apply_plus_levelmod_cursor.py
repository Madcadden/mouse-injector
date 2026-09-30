#!/usr/bin/env python3
"""Add direct cursor ownership for GoldenEye Plus Level Modifiers.

Run after tools/prepare_freefly_f3.py. The resolver keys off the compiled
input semantics (Up/Down masks, modulo-3 category selection, and adjacent
level/list globals), not ROM title/CRC or fixed RAM addresses.
"""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
P = ROOT / "games/goldeneye.c"
s = P.read_text()

if "levelmod_category" not in s:
    old = """\tunsigned int seenintroflag;\n\tGE_MAPMAKER_PROFILE mapmaker;\n} GE_ADDRESS_PROFILE;"""
    new = """\tunsigned int seenintroflag;\n\tGE_MAPMAKER_PROFILE mapmaker;\n\tunsigned int levelmod_category, levelmod_level, levelmod_top;\n} GE_ADDRESS_PROFILE;"""
    if old not in s:
        raise SystemExit("GE_ADDRESS_PROFILE anchor not found")
    s = s.replace(old, new, 1)

resolver_anchor = "static int GE_ResolveAddressProfile(GE_ADDRESS_PROFILE *profile)\n{"
if "GE_ResolveLevelModifierProfile" not in s:
    resolver = r'''
/* Resolve Plus's Level Modifiers row state from the compiled input routine.
 * The routine is distinctive: Up is 0x0808, Down is 0x0404, category wraps
 * modulo 3, and entering the level list clears the next two globals. */
static void GE_ResolveLevelModifierProfile(GE_ADDRESS_PROFILE *profile)
{
	unsigned int offset, found = 0, category = 0, level = 0, top = 0;
	for(offset = 0x1000U; offset <= GE_ROM_SCAN_LIMIT - 0xACU; offset += 4U)
	{
		const unsigned int w0 = EMU_ReadROM(offset);
		const unsigned int w2 = EMU_ReadROM(offset + 8);
		const unsigned int w3 = EMU_ReadROM(offset + 12);
		const unsigned int w4 = EMU_ReadROM(offset + 16);
		const unsigned int w5 = EMU_ReadROM(offset + 20);
		const unsigned int w6 = EMU_ReadROM(offset + 24);
		const unsigned int w7 = EMU_ReadROM(offset + 28);
		const unsigned int w8 = EMU_ReadROM(offset + 32);
		const unsigned int w9 = EMU_ReadROM(offset + 36);
		const unsigned int w10 = EMU_ReadROM(offset + 40);
		const unsigned int w12 = EMU_ReadROM(offset + 48);
		const unsigned int w14 = EMU_ReadROM(offset + 56);
		const unsigned int w15 = EMU_ReadROM(offset + 60);
		const unsigned int w16 = EMU_ReadROM(offset + 64);
		const unsigned int w17 = EMU_ReadROM(offset + 68);
		const unsigned int w18 = EMU_ReadROM(offset + 72);
		const unsigned int w19 = EMU_ReadROM(offset + 76);
		const unsigned int w20 = EMU_ReadROM(offset + 80);
		unsigned int base, base2, loadreg, addreg, divreg, resultreg;
		unsigned int loadreg2, addreg2, divreg2, resultreg2;
		unsigned int candidate, candidatelevel, candidatetop;

		if((w0 & 0xFC00FFFFU) != 0x30000808U ||
			(w2 & 0xFC00FFFFU) != 0x30000404U)
			continue;
		if((w3 >> 26) != 0x0FU || (w4 >> 26) != 0x09U || (w5 >> 26) != 0x23U ||
			(w6 >> 26) != 0x09U || (w7 >> 26) != 0x09U ||
			(w12 >> 26) != 0x0FU || (w14 >> 26) != 0x09U || (w15 >> 26) != 0x23U ||
			(w16 >> 26) != 0x09U || (w17 >> 26) != 0x09U)
			continue;
		base = (w3 >> 16) & 31U;
		base2 = (w12 >> 16) & 31U;
		if(!base || base2 != base || ((w4 >> 21) & 31U) != base ||
			((w4 >> 16) & 31U) != base || ((w5 >> 21) & 31U) != base ||
			(w5 & 0xFFFFU) != 0 || ((w14 >> 21) & 31U) != base ||
			((w14 >> 16) & 31U) != base || ((w15 >> 21) & 31U) != base ||
			(w15 & 0xFFFFU) != 0 || (w4 & 0xFFFFU) != (w14 & 0xFFFFU))
			continue;
		loadreg = (w5 >> 16) & 31U;
		divreg = (w6 >> 16) & 31U;
		addreg = (w7 >> 16) & 31U;
		resultreg = (w9 >> 11) & 31U;
		loadreg2 = (w15 >> 16) & 31U;
		divreg2 = (w16 >> 16) & 31U;
		addreg2 = (w17 >> 16) & 31U;
		resultreg2 = (w19 >> 11) & 31U;
		if(((w6 >> 21) & 31U) != 0 || (w6 & 0xFFFFU) != 3 ||
			((w7 >> 21) & 31U) != loadreg || (w7 & 0xFFFFU) != 2 ||
			(w8 >> 26) != 0 || (w8 & 63U) != 0x1AU ||
			((w8 >> 21) & 31U) != addreg || ((w8 >> 16) & 31U) != divreg ||
			(w9 >> 26) != 0 || (w9 & 63U) != 0x10U ||
			(w10 >> 26) != 0x2BU || ((w10 >> 21) & 31U) != base ||
			((w10 >> 16) & 31U) != resultreg || (w10 & 0xFFFFU) != 0)
			continue;
		if(((w16 >> 21) & 31U) != 0 || (w16 & 0xFFFFU) != 3 ||
			((w17 >> 21) & 31U) != loadreg2 || (w17 & 0xFFFFU) != 1 ||
			(w18 >> 26) != 0 || (w18 & 63U) != 0x1AU ||
			((w18 >> 21) & 31U) != addreg2 || ((w18 >> 16) & 31U) != divreg2 ||
			(w19 >> 26) != 0 || (w19 & 63U) != 0x10U ||
			(w20 >> 26) != 0x2BU || ((w20 >> 21) & 31U) != base ||
			((w20 >> 16) & 31U) != resultreg2 || (w20 & 0xFFFFU) != 0)
			continue;

		candidate = GE_MakeAddress(w3, w4);
		candidatelevel = GE_MakeAddress(EMU_ReadROM(offset + 0x9CU), EMU_ReadROM(offset + 0xA0U));
		candidatetop = GE_MakeAddress(EMU_ReadROM(offset + 0xA4U), EMU_ReadROM(offset + 0xA8U));
		if(!GE_MapMakerDataAddress(candidate) || candidatelevel != candidate + 4U ||
			candidatetop != candidate + 8U ||
			(EMU_ReadROM(offset + 0xA0U) & 0xFFFF0000U) != 0xAC200000U ||
			(EMU_ReadROM(offset + 0xA8U) & 0xFFFF0000U) != 0xAC200000U)
			continue;
		if(found) return; /* ambiguous: fail closed */
		found = offset;
		category = candidate;
		level = candidatelevel;
		top = candidatetop;
	}
	if(found && profile->mapmaker.page && profile->maxpage >= profile->mapmaker.page + 3U)
	{
		profile->levelmod_category = category;
		profile->levelmod_level = level;
		profile->levelmod_top = top;
	}
}

/* The Level Modifiers renderer already draws the normal frontend crosshair,
 * but the game itself only changes rows from digital input. Own the exact
 * rectangles it draws and write the resolved selectors directly. This avoids
 * repeated synthetic D-pad pulses and makes visual hover equal selection. */
static int GE_LevelModifierMenuMouse(const GE_ADDRESS_PROFILE *profile)
{
	const int page = EMU_ReadInt(profile->menupage);
	const int relative = profile->mapmaker.page ? page - (int)profile->mapmaker.page : 0;
	const float x = EMU_ReadFloat(profile->menux);
	const float y = EMU_ReadFloat(profile->menuy);
	int target = -1;

	if(!profile->levelmod_category || relative < 1 || relative > 3 ||
		!(x >= 20.0f && x <= 420.0f && y >= 20.0f && y <= 310.0f))
		return 0;

	if(relative == 1)
	{
		/* Highlight rectangles are [70,370] x [84+30*n,104+30*n]. */
		if(x >= 70.0f && x <= 370.0f)
		{
			for(int row = 0; row < 3; row++)
			{
				const float top = 84.0f + row * 30.0f;
				if(y >= top && y <= top + 20.0f) { target = row; break; }
			}
		}
		if(target >= 0 && EMU_ReadInt(profile->levelmod_category) != target)
			EMU_WriteInt(profile->levelmod_category, target);
		return 1;
	}
	if(relative == 2)
	{
		const int category = EMU_ReadInt(profile->levelmod_category);
		const int listtop = EMU_ReadInt(profile->levelmod_top);
		const int count = category == 0 ? 20 : (category == 1 ? 6 : (category == 2 ? 2 : 0));
		if(count > 0 && listtop >= 0 && listtop < count && x >= 68.0f && x <= 360.0f &&
			y >= 77.0f && y < 237.0f)
		{
			const int visible = (int)((y - 77.0f) / 16.0f);
			target = listtop + visible;
			if(target >= count) target = count - 1;
			if(target >= 0 && EMU_ReadInt(profile->levelmod_level) != target)
				EMU_WriteInt(profile->levelmod_level, target);
		}
		return 1;
	}
	/* Detail has one fixed highlighted row; normal Z/A and menu Back mappings
	 * already perform its toggle/back actions without any synthetic movement. */
	return 1;
}

'''
    if resolver_anchor not in s:
        raise SystemExit("resolver insertion anchor not found")
    s = s.replace(resolver_anchor, resolver + resolver_anchor, 1)

call_anchor = "\tGE_ResolveMapMakerProfile(profile);\n"
if "GE_ResolveLevelModifierProfile(profile);" not in s:
    if call_anchor not in s:
        raise SystemExit("profile call anchor not found")
    s = s.replace(call_anchor, call_anchor + "\tGE_ResolveLevelModifierProfile(profile);\n", 1)

inject_anchor = "\tGE_Controller(); // set controller data\n"
if "GE_LevelModifierMenuMouse(GE_GetAddressProfile());" not in s:
    if inject_anchor not in s:
        raise SystemExit("inject anchor not found")
    s = s.replace(inject_anchor,
        "\tGE_LevelModifierMenuMouse(GE_GetAddressProfile());\n" + inject_anchor, 1)

P.write_text(s)
print("Applied direct Plus Level Modifiers cursor resolver")
