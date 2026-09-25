/* Plus keeps the native B action but has several input compiler layouts.
 * Match their control flow, retaining relocated globals and calls. The unused
 * controller bit 0x40 is carried separately from B through the native action
 * flag, just as it is in the retail reload patch. */
typedef struct GE_PLUS_RELOAD_PROFILE
{
	unsigned int address[23], original[23], replacement[23];
} GE_PLUS_RELOAD_PROFILE;

typedef struct GE_PLUS_RELOAD_LAYOUT
{
	unsigned int input[10], edge[7], player[10], flag[8];
} GE_PLUS_RELOAD_LAYOUT;

static const GE_PLUS_RELOAD_LAYOUT geplusreloadlayouts[] = {
	{
		{0x8E0B0000,0x8FA2005C,0x8D630124,0x30494000,0x0009602B,
		 0x2C650001,0xAFA501E4,0xAFA50170,0xAFAC01D8,0xAFAC0040},
		{0x97AD01F6,0x8FB80054,0xA7A30124,0x01A05827,0x030B4824,
		 0x0C000000,0xAFA9005C},
		{0x8FAD01D8,0x11A0009D,0x3C180000,0x8F180000,0x24010001,
		 0x3C090000,0x1701002F,0x3C020000,0x8D290000,0x24040020},
		{0xAC200000,0x10000005,0x8FB8016C,0x8E0D0000,0x240E0001,
		 0xADAE00D0,0x8FB8016C,0x1700000B}
	},
	{
		{0x8E0A0000,0x8FA20060,0x8D430124,0x304B4000,0x000B482B,
		 0x2C650001,0xAFA501EC,0xAFA50178,0xAFA901E0,0xAFA90044},
		{0x97AC01FE,0x8FAD0058,0xA7A3012C,0x01805027,0x01AA5824,
		 0x0C000000,0xAFAB0060},
		{0x8FAD01E0,0x11A0009D,0x3C0B0000,0x8D6B0000,0x24010001,
		 0x3C0E0000,0x1561002F,0x3C020000,0x8DCE0000,0x24040020},
		{0xAC200000,0x10000005,0x8FAB0174,0x8E0D0000,0x240C0001,
		 0xADAC00D0,0x8FAB0174,0x1560000B}
	},
	{
		/* SRAM-era Plus V3: the input stack and temporary registers changed.
		 * Calls and globals still come from the cartridge's own operands. */
		{0x8E0B0000,0x8FA2005C,0x8D630124,0x304C4000,0x000C502B,
		 0x2C650001,0xAFA501EC,0xAFA50178,0xAFAA01E0,0xAFAA0040},
		{0x97AD01FE,0x8FAE0054,0xA7A3012C,0x01A05827,0x01CB6024,
		 0x0C000000,0xAFAC005C},
		{0x8FAE01E0,0x11C0009D,0x3C0C0000,0x8D8C0000,0x24010001,
		 0x3C0F0000,0x1581002F,0x3C020000,0x8DEF0000,0x24040020},
		{0xAC200000,0x10000005,0x8FAC0174,0x8E0E0000,0x240D0001,
		 0xADCD00D0,0x8FAC0174,0x1580000B}
	}
};

static int GE_PlusReloadTitle(void)
{
	static const char title[] = "goldeneye 007 plus";
	for(unsigned int start = 0; start <= 20 - (sizeof(title) - 1); start++)
	{
		unsigned int index;
		for(index = 0; index < sizeof(title) - 1; index++)
		{
			unsigned int offset = 0x20 + start + index;
			unsigned int ch = (EMU_ReadROM(offset & ~3U) >> ((3 - (offset & 3)) * 8)) & 255;
			if(ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
			if(ch != (unsigned char)title[index]) break;
		}
		if(index == sizeof(title) - 1) return 1;
	}
	return 0;
}

static int GE_ResolvePlusReload(GE_PLUS_RELOAD_PROFILE *result)
{
	static const unsigned int exact[10] = {
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF
	};
	static const unsigned int edgemask[7] = {
		0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFC000000,0xFFFFFFFF
	};
	const GE_ADDRESS_PROFILE *addresses = GE_GetAddressProfile();
	unsigned int input = 0, player = 0, flag = 0, weapon, logic, codebase;
	unsigned int rawstack, rawreg, tankreg, propreg, flagreg, tankhigh;
	unsigned int tanklow, proplow, target, call, currentplayer;
	if(!GE_PlusReloadTitle() || !addresses->bonddata || GE_GetReloadHackProfile()) return 0;
	for(unsigned int layout = 0; layout < sizeof(geplusreloadlayouts) / sizeof(geplusreloadlayouts[0]); layout++)
	{
		const GE_PLUS_RELOAD_LAYOUT *candidate = &geplusreloadlayouts[layout];
		unsigned int found = GE_FindUniqueROMPattern(candidate->input, exact, 10);
		if(!found) continue;
		if(input || found < 0x5C || !GE_ROMPatternMatches(found - 0x5C, candidate->edge, edgemask, 7)) return 0;
		input = found;
		player = GE_FindUniqueROMPattern(candidate->player, gereloadplayermask, 10);
		flag = GE_FindUniqueROMPattern(candidate->flag, gereloadflagmask, 8);
	}
	weapon = GE_FindUniqueROMPattern(gereloadweaponpattern, gereloadweaponmask, 8);
	logic = GE_FindUniqueROMPattern(gereloadlogicpattern, gereloadlogicmask, 11);
	if(!input || !player || !flag || !weapon || logic < 0x164) return 0;
	/* Keep all three input sites in the same routine. The no-action branch
	 * must land immediately after the flag store, and R's tank bypass must
	 * land at the ordinary non-tank action path. */
	if(player <= input || player - input > 0x1000 || flag <= player || flag - player > 0x800
		|| player + 8 + (short)EMU_ReadROM(player + 4) * 4 != flag + 24) return 0;
	if(EMU_ReadROM(logic + 0x1C) != EMU_ReadROM(logic + 0x24)) return 0;
	call = EMU_ReadROM(logic - 4);
	if((call & 0xFC000000U) != 0x0C000000U) return 0;
	target = 0x80000000U | ((call & 0x03FFFFFFU) << 2);
	if(target < weapon) return 0;
	codebase = target - weapon;
	currentplayer = GE_MakeAddress(EMU_ReadROM(weapon), EMU_ReadROM(weapon + 4));
	/* lv's saved s0 holds the address of g_CurrentPlayer, not the separate
	 * player-pointer array used by the host mouse injector. Bind its setup
	 * to both native getters before using it in the call delay slot. */
	if((codebase & 0xFF800003U) != 0x80000000U
		|| !GE_ValidDataAddress(currentplayer, 4)
		|| GE_MakeAddress(EMU_ReadROM(weapon + 16), EMU_ReadROM(weapon + 20)) != currentplayer
		|| (EMU_ReadROM(logic - 0x164) & 0xFFFF0000U) != 0x3C100000U
		|| (EMU_ReadROM(logic - 0x160) & 0xFFFF0000U) != 0x26100000U
		|| GE_MakeAddress(EMU_ReadROM(logic - 0x164), EMU_ReadROM(logic - 0x160)) != currentplayer) return 0;
	tankhigh = EMU_ReadROM(player + 8) & 0xFFFF;
	tanklow = EMU_ReadROM(player + 12) & 0xFFFF;
	proplow = EMU_ReadROM(player + 32) & 0xFFFF;
	if((EMU_ReadROM(player + 20) & 0xFFFF) != tankhigh
		|| (EMU_ReadROM(player + 28) & 0xFFFF) != tankhigh
		|| GE_MakeAddress(EMU_ReadROM(player + 8), EMU_ReadROM(player + 12)) != addresses->tankflag
		|| !GE_ValidDataAddress(GE_MakeAddress(EMU_ReadROM(player + 20), EMU_ReadROM(player + 32)), 4)) return 0;
	rawstack = EMU_ReadROM(input + 4) & 0xFFFF;
	rawreg = (EMU_ReadROM(player + 8) >> 16) & 31;
	tankreg = rawreg;
	propreg = (EMU_ReadROM(player + 32) >> 16) & 31;
	flagreg = (EMU_ReadROM(flag + 16) >> 16) & 31;
	result->address[0] = flag + 16;
	result->replacement[0] = 0x8FA00000 | (flagreg << 16) | rawstack;
	result->address[1] = input + 12;
	result->replacement[1] = EMU_ReadROM(input + 12) | 0x40;
	for(unsigned int i = 0; i < 7; i++) result->address[2 + i] = player + 8 + i * 4;
	result->replacement[2] = 0x8FA00000 | (rawreg << 16) | rawstack;
	result->replacement[3] = 0x30004000 | (rawreg << 21) | (rawreg << 16);
	result->replacement[4] = 0x10000000 | (rawreg << 21) | ((flag + 12 - (player + 20)) / 4);
	result->replacement[5] = 0x3C020000 | tankhigh;
	result->replacement[6] = 0x8C400000 | (tankreg << 16) | tanklow;
	/* The native tank flag is boolean; preserve its non-tank branch target. */
	result->replacement[7] = 0x10000000 | (tankreg << 21) | (short)(EMU_ReadROM(player + 24) - 1);
	result->replacement[8] = 0x8C400000 | (propreg << 16) | proplow;
	result->address[9] = weapon;
	result->address[10] = weapon + 4;
	result->address[11] = weapon + 12;
	result->replacement[9] = 0x8C4200D0;
	result->replacement[10] = 0x304B0040;
	result->replacement[11] = 0x304A4000;
	for(unsigned int i = 0; i < 11; i++) result->address[12 + i] = logic + i * 4;
	result->replacement[12] = 0x8E020000;
	result->replacement[13] = 0x51600005;
	result->replacement[14] = 0;
	result->replacement[15] = EMU_ReadROM(logic + 0x1C);
	result->replacement[16] = 0x00002025;
	result->replacement[17] = EMU_ReadROM(logic + 0x24);
	result->replacement[18] = 0x24040001;
	result->replacement[19] = 0x11400003;
	result->replacement[20] = 0;
	result->replacement[21] = EMU_ReadROM(logic + 0x0C);
	result->replacement[22] = 0;
	for(unsigned int i = 0; i < 23; i++) result->original[i] = EMU_ReadROM(result->address[i]);
	return 1;
}

static void GE_InjectPlusReloadHack(void)
{
	static GE_PLUS_RELOAD_PROFILE profile;
	static const unsigned char **cachedrom;
	static unsigned int generation, crc1, crc2;
	static int initialized, valid;
	if(!romptr) return;
	if(!initialized || generation != ge_rom_generation || cachedrom != romptr
		|| crc1 != EMU_ReadROM(0x10) || crc2 != EMU_ReadROM(0x14))
	{
		initialized = 1;
		generation = ge_rom_generation;
		cachedrom = romptr;
		crc1 = EMU_ReadROM(0x10);
		crc2 = EMU_ReadROM(0x14);
		valid = GE_ResolvePlusReload(&profile);
	}
	if(!valid) return;
	for(unsigned int i = 0; i < 23; i++)
		if(!GE_PatchWordIsSafe(profile.address[i], profile.original[i], profile.replacement[i])) return;
	for(unsigned int i = 0; i < 23; i++) GE_WriteOwnedROM(profile.address[i], profile.replacement[i]);
}
