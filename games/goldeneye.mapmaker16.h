/* Plus's revised 16-row Map Maker. GPL-2.0-or-later.
 * These independent code/data-flow anchors identify the compiled input,
 * camera and drawing layout, not a ROM title, checksum or absolute address.
 * Instruction changes outside these layouts fail closed. The older 14-row
 * layout remains independently validated in goldeneye.c. */
static const unsigned int gemapmaker16inputpattern[32] = {
	0x27BDFFA8U,0xAFBF001CU,0xAFB00018U,0x00002025U,0x0C000000U,0x3405FFFFU,
	0x00408025U,0x00002025U,0x0C000000U,0x3405FFFFU,0xAFA20050U,0x0C000000U,
	0x00002025U,0xAFA2004CU,0x0C000000U,0x00002025U,0x3C0E0000U,0x8DCE0000U,
	0x3C040000U,0xAFA20048U,0x11C00005U,0x24840000U,0x0C000000U,0x00000000U,
	0x100005F5U,0x00001025U,0x8C8F0000U,0x32185000U,0x320C1000U,0x11E00115U,
	0x00000000U,0x1300000BU
};
static const unsigned int gemapmaker16inputmask[32] = {
	0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFC000000U,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFFFFFFFFU,0xFC000000U,0xFFFFFFFFU,0xFFFFFFFFU,0xFC000000U,
	0xFFFFFFFFU,0xFFFFFFFFU,0xFC000000U,0xFFFFFFFFU,0xFFFF0000U,0xFFFF0000U,
	0xFFFF0000U,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFF0000U,0xFC000000U,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFFFFFFFFU
};
static const unsigned int gemapmaker16modepattern[24] = {
	0x3C0C0000U,0x8D8C0000U,0x11800015U,0x320D0020U,0x11A0000FU,0x3C0E0000U,
	0x8DCE0000U,0x24010001U,0x3C0F0000U,0x15C10005U,0x00000000U,0x0C000000U,
	0x00000000U,0x10000006U,0x00000000U,0x8DEF0000U,0x3C010000U,0x25F80001U,
	0x33190003U,0xAC390000U,0x0C000000U,0x00000000U,0x100000E0U,0x00000000U
};
static const unsigned int gemapmaker16modemask[24] = {
	0xFFFF0000U,0xFFFF0000U,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFF0000U,
	0xFFFF0000U,0xFFFFFFFFU,0xFFFF0000U,0xFFFFFFFFU,0xFFFFFFFFU,0xFC000000U,
	0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFF0000U,0xFFFF0000U,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFFFF0000U,0xFC000000U,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU
};
static const unsigned int gemapmaker16navpattern[27] = {
	0x1320000BU,0x320C0404U,0x3C030000U,0x24630000U,0x8C690000U,0x252A000FU,
	0x05410004U,0x314B000FU,0x11600002U,0x00000000U,0x256BFFF0U,0xAC6B0000U,
	0x3C030000U,0x11800009U,0x24630000U,0x8C6D0000U,0x25AE0001U,0x05C10004U,
	0x31CF000FU,0x11E00002U,0x00000000U,0x25EFFFF0U,0xAC6F0000U,0x8C620000U,
	0x24010003U,0x3218A300U,0x5441000DU
};
static const unsigned int gemapmaker16navmask[27] = {
	0xFFFFFFFFU,0xFFFFFFFFU,0xFFFF0000U,0xFFFF0000U,0xFFFFFFFFU,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
	0xFFFF0000U,0xFFFFFFFFU,0xFFFF0000U,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU
};
static const unsigned int gemapmaker16renderpattern[55] = {
	0x0C000000U,0x00000000U,0x14530004U,0x24050044U,0x240C0148U,0x10000003U,
	0xAFAC00A8U,0x240D0174U,0xAFAD00A8U,0x240E0140U,0x240F00F0U,0xAFAF0014U,
	0xAFAE0010U,0x8FA40158U,0x2406001CU,0x0C000000U,0x8FA700A8U,0x3C18FFE0U,
	0x371870FFU,0x3C070000U,0xAFA20158U,0x24E70000U,0xAFB80010U,0x00402025U,
	0x2405005CU,0x0C000000U,0x24060022U,0x0C000000U,0xAFA20158U,0x3C080000U,
	0x25120000U,0x0002C880U,0x00025080U,0x01525821U,0x03324821U,0x3C15D0D0U,
	0x00409825U,0x36B5D0FFU,0xAFA90070U,0xAFAB006CU,0x24140032U,0x8FAC0070U,
	0x8FA40158U,0x24050056U,0x1592000AU,0x2686FFFDU,0x8FA700A8U,0x3C0E7656U,
	0x35CE1CB0U,0x268D000FU,0xAFAD0010U,0xAFAE0014U,0x0C000000U,0x24E7FFEEU,
	0xAFA20158U
};
static const unsigned int gemapmaker16rendermask[55] = {
	0xFC000000U,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFC000000U,0xFFFFFFFFU,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFFFF0000U,0xFFFFFFFFU,0xFFFF0000U,0xFFFFFFFFU,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFC000000U,0xFFFFFFFFU,0xFC000000U,0xFFFFFFFFU,0xFFFF0000U,
	0xFFFF0000U,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFC000000U,0xFFFFFFFFU,
	0xFFFFFFFFU
};
static const unsigned int gemapmaker16renderendpattern[7] = {
	0x3C180000U,0x27180000U,0x26520004U,0x1658FED2U,0x26940011U,0x0C000000U,
	0x8FA40158U
};
static const unsigned int gemapmaker16renderendmask[7] = {
	0xFFFF0000U,0xFFFF0000U,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFC000000U,
	0xFFFFFFFFU
};

static void GE_ResolveMapMaker16Menu(GE_ADDRESS_PROFILE *profile,
	const unsigned int input, const unsigned int mode, const unsigned int codebase)
{
	GE_MAPMAKER_PROFILE *editor = &profile->mapmaker;
	const unsigned int drawing = GE_FindUniqueROMPattern(gemapmaker16renderpattern, gemapmaker16rendermask, 55);
	unsigned int row, tool, labels;
	/* Validate navigation modulus, draw-loop extent, row spacing and the
	 * getter calls together before allowing the mouse to select any row. */
	if(input > GE_ROM_SCAN_LIMIT - 0x118
		|| !GE_ROMPatternMatches(input + 0xAC, gemapmaker16navpattern, gemapmaker16navmask, 27)
		|| drawing < 0xA64 || drawing > GE_ROM_SCAN_LIMIT - 0x568
		|| EMU_ReadROM(drawing - 0xA64) != 0x24130001U
		|| !GE_ROMPatternMatches(drawing + 0x54C, gemapmaker16renderendpattern, gemapmaker16renderendmask, 7)
		/* Verify the native music-value column rather than guessing it. */
		|| EMU_ReadROM(drawing + 0x2D4) != 0x240500BEU)
		return;
	row = GE_MakeAddress(EMU_ReadROM(input + 0xB4), EMU_ReadROM(input + 0xB8));
	tool = GE_MakeAddress(EMU_ReadROM(mode + 0x14), EMU_ReadROM(mode + 0x18));
	labels = GE_MakeAddress(EMU_ReadROM(drawing + 0x74), EMU_ReadROM(drawing + 0x78));
	if(!GE_MapMakerDataAddress(row) || row != editor->menu + 4
		|| GE_MakeAddress(EMU_ReadROM(input + 0xDC), EMU_ReadROM(input + 0xE4)) != row
		|| GE_MapMakerMenuGetter(EMU_ReadROM(drawing + 0x6C), codebase) != row
		|| !GE_MapMakerDataAddress(tool) || tool + 4 != editor->freemode
		|| GE_MapMakerMenuGetter(EMU_ReadROM(drawing), codebase) != tool
		|| !GE_MapMakerDataAddress(labels)
		|| GE_MakeAddress(EMU_ReadROM(drawing + 0x54C), EMU_ReadROM(drawing + 0x550)) != labels + 16 * 4)
		return;
	editor->menu_selection = row;
	editor->menu_tool = tool;
	editor->menu_rows = 16;
	editor->menu_top = 47;
	editor->menu_stride = 17;
	editor->menu_music_x = 190;
}

/* First Person View is an editor preview, NOT Native Test gameplay.
 * It has independent angles. Recognize the linked input routine and its
 * angle update/clamp, leaving normal gameplay watches and aiming alone. */
static const unsigned int gemapmakerpreviewentrypattern[8] = {
	0x27BDFFA8U,0xAFBF0014U,0x00002025U,0x0C000000U,0x3405FFFFU,0x304E1000U,
	0x11C00022U,0x27A40044U
};
static const unsigned int gemapmakerpreviewentrymask[8] = {
	0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFC000000U,0xFFFFFFFFU,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFFFFFFFFU
};
static const unsigned int gemapmakerpreviewlookpattern[44] = {
	0x0C000000U,0x24050001U,0x3C010000U,0x3C180000U,0xC42C0000U,0x8F180000U,
	0x3C010000U,0xC42A0000U,0xC7A8004CU,0x44983000U,0x3C030000U,0x460A4102U,
	0x24630000U,0xC4680000U,0x3C010000U,0x3C020000U,0x24420000U,0x468030A0U,
	0x46022182U,0xC7A40050U,0x46064281U,0xE46A0000U,0xC4280000U,0x3C010000U,
	0x46082182U,0xC4440000U,0x46023282U,0x460A2200U,0xE4480000U,0xC4400000U,
	0x460C003CU,0x00000000U,0x45000003U,0x00000000U,0xE44C0000U,0xC4400000U,
	0xC4220000U,0x4600103CU,0x00000000U,0x45000002U,0x00000000U,0xE4420000U,
	0x0C000000U,0xC46C0000U
};
static const unsigned int gemapmakerpreviewlookmask[44] = {
	0xFC000000U,0xFFFFFFFFU,0xFFFF0000U,0xFFFF0000U,0xFFFF0000U,0xFFFF0000U,
	0xFFFF0000U,0xFFFF0000U,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFF0000U,0xFFFFFFFFU,
	0xFFFF0000U,0xFFFFFFFFU,0xFFFF0000U,0xFFFF0000U,0xFFFF0000U,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFF0000U,0xFFFF0000U,
	0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
	0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
	0xFFFF0000U,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,
	0xFC000000U,0xFFFFFFFFU
};

static void GE_ResolveMapMakerPreview(GE_ADDRESS_PROFILE *profile,
	const unsigned int input, const unsigned int look, const unsigned int codebase, const int revised)
{
	GE_MAPMAKER_PROFILE *editor = &profile->mapmaker;
	const unsigned int target = GE_MapMakerCallTarget(EMU_ReadROM(input + (revised ? 0x58 : 0x54)));
	unsigned int handler, math, yaw, pitch, minimum, maximum;
	if(target < codebase || target - codebase > GE_ROM_SCAN_LIMIT - 0x154) return;
	handler = target - codebase;
	math = handler + 0xA4;
	if(!GE_ROMPatternMatches(handler, gemapmakerpreviewentrypattern, gemapmakerpreviewentrymask, 8)
		|| !GE_ROMPatternMatches(math, gemapmakerpreviewlookpattern, gemapmakerpreviewlookmask, 44)
		|| GE_MakeAddress(EMU_ReadROM(math + 0x0C), EMU_ReadROM(math + 0x14))
			!= GE_MakeAddress(EMU_ReadROM(look + 0x34), EMU_ReadROM(look + 0x3C))
		|| (EMU_ReadROM(handler + 0x20) & 0xFFFF0000U) != 0x3C010000U
		|| (EMU_ReadROM(handler + 0x24) & 0xFFFF0000U) != 0xAC200000U
		|| GE_MakeAddress(EMU_ReadROM(handler + 0x20), EMU_ReadROM(handler + 0x24)) != editor->preview)
		return;
	yaw = GE_MakeAddress(EMU_ReadROM(math + 0x28), EMU_ReadROM(math + 0x30));
	pitch = GE_MakeAddress(EMU_ReadROM(math + 0x3C), EMU_ReadROM(math + 0x40));
	minimum = GE_MakeAddress(EMU_ReadROM(math + 8), EMU_ReadROM(math + 16));
	maximum = GE_MakeAddress(EMU_ReadROM(math + 0x5C), EMU_ReadROM(math + 0x90));
	if(!GE_MapMakerDataAddress(yaw) || !GE_MapMakerDataAddress(pitch)
		|| !GE_MapMakerDataAddress(minimum) || !GE_MapMakerDataAddress(maximum)
		|| pitch != yaw + 4 || yaw == editor->yaw || pitch == editor->pitch) return;
	editor->preview_yaw = yaw;
	editor->preview_pitch = pitch;
	editor->preview_pitchmin = minimum;
	editor->preview_pitchmax = maximum;
}
