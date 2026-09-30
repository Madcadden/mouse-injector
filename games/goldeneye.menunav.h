/* Native multiplayer and frontend panels use direction/button input rather
 * than the frontend crosshair. Keep this adapter local to those active menus. */
typedef struct GE_MENU_NATIVE_STATE
{
	float x, y;
	int context, direction, heldms, releasems, idlems, blockfire, waitfire;
	int cursorrow, cursortarget, listtop, pluscategory;
} GE_MENU_NATIVE_STATE;

static int ge_plus_levelmod_category;

static GE_MENU_NATIVE_STATE ge_menu_native[ALLPLAYERS];

static void GE_MenuNativeReset(void)
{
	const GE_MENU_NATIVE_STATE empty = {0};
	for(int player = PLAYER1; player < ALLPLAYERS; player++)
		ge_menu_native[player] = empty;
}

static int GE_MenuNativeContext(const int player)
{
	const unsigned int base = playerbase[player];
	const GE_ADDRESS_PROFILE *profile = GE_GetAddressProfile();
	const int page = EMU_ReadInt(profile->menupage);
	int dead, multiplayer, ended;
	if(PROFILE[player].SETTINGS[CONFIG] == DISABLED || !mousetoggle)
		return 0;
	/* These frontend panels read each player's left/right controller
	 * direction, while their crosshair is used only for the Back tab. */
	if(page == 15 || page == 16 || page == 17)
		return 100 + page;
	if(player == PLAYER1 && page == 20)
		return 120;
	/* GoldenEye Plus deliberately appends new mod frontend pages after its
	 * Map Maker pages so existing menu IDs stay stable. These appended pages
	 * use native digital navigation instead of the ordinary frontend cursor.
	 * Use the ROM-resolved Map Maker page and menu-table upper bound rather
	 * than hardcoded Level Modifiers IDs, so later appended Plus menus inherit
	 * mouse gesture navigation automatically. Map Maker itself remains owned
	 * by its dedicated cursor/free-fly handlers above this adapter. */
	if(player == PLAYER1 && profile->mapmaker.page
		&& page > (int)profile->mapmaker.page && page <= (int)profile->maxpage)
		return 200 + page;
	if(player == PLAYER1 && page == 5 && profile->erase_selection)
	{
		const int folder = EMU_ReadInt(profile->erase_selection);
		if(folder >= 0 && folder < 4)
			return 105;
	}
	if(page != 11 || (base & 0xFF800003U) != 0x80000000U
		|| (base & 0x7FFFFFU) > 0x800000U - GE_multipausemenu - 4)
		return 0;
	dead = EMU_ReadInt(base + GE_deathflag);
	multiplayer = EMU_ReadInt(base + GE_multipausemenu);
	ended = EMU_ReadInt(GE_matchended);
	/* The gameplay watch keeps its native controls in every ROM, including
	 * Plus Native Test Mode. Map Maker's separate editor pause menu is
	 * handled by GE_MapMakerMenuMouse using its verified page and state. */
	/* A round-end countdown >= 2 does not accept input yet. Ordinary death
	 * without a completed round does not expose a multiplayer menu either. */
	if(multiplayer == 1 && (ended == 0 || ended == 1)
		&& (dead == 0 || (dead == 1 && ended == 1)))
		return 2;
	return 0;
}

/* Latest Plus appends Level Modifiers immediately after Map Maker.
 * Unlike ordinary frontend pages, these screens draw the frontend crosshair
 * but never perform cursor hit-testing: they only consume digital directions.
 * Mirror their authored row geometry so the visible mouse cursor really owns
 * the highlighted row. Future appended pages still fall back to gesture
 * navigation below; these three pages get exact cursor behavior. */
static int GE_PlusLevelModifierCursor(GE_MENU_NATIVE_STATE *state,
	const GE_ADDRESS_PROFILE *profile, const int page, const int elapsedms)
{
	const int relative = page - (int)profile->mapmaker.page;
	const float y = EMU_ReadFloat(profile->menuy);
	int target = -1, count = 0, direction = 0;

	if(relative < 1 || relative > 3 || !(y >= 20.0f && y <= 310.0f))
		return -1;

	/* Finish the current digital press and neutral interval before moving the
	 * highlight again. This guarantees joyGetButtonsPressedThisFrame sees a
	 * fresh edge rather than one long held press. */
	if(state->heldms > 0)
	{
		direction = state->direction;
		state->heldms -= elapsedms;
		if(state->heldms <= 0) state->releasems = 35;
		return direction;
	}
	if(state->releasems > 0)
	{
		state->releasems -= elapsedms;
		return 0;
	}

	if(relative == 1)
	{
		/* Category rows are drawn at y=86,116,146. Split at their midpoints
		 * so every visible gap still belongs to the nearest row. */
		if(y >= 70.0f && y < 176.0f)
			target = ClampInt((int)((y - 71.0f) / 30.0f), 0, 2);
	}
	else if(relative == 2)
	{
		/* Level rows are drawn at y=78 + 16*n, ten visible at once. */
		count = ge_plus_levelmod_category == 0 ? 20 :
			(ge_plus_levelmod_category == 1 ? 6 : 2);
		if(y >= 69.0f && y < 238.0f)
		{
			const int visible = ClampInt((int)((y - 70.0f) / 16.0f), 0, 9);
			target = state->listtop + visible;
			if(target >= count) target = count - 1;
		}
		/* Pushing the cursor beyond the list while continuing to move gives
		 * mouse-only access to SP rows 10..19 without changing wheel binds. */
		else if(y >= 238.0f && DEVICE[PLAYER1].YPOS > 0)
			target = state->cursorrow + 1 < count ? state->cursorrow + 1 : state->cursorrow;
		else if(y < 69.0f && DEVICE[PLAYER1].YPOS < 0)
			target = state->cursorrow > 0 ? state->cursorrow - 1 : 0;
	}
	else
	{
		/* Detail pages have one already-highlighted option. Left click maps to
		 * A below; right click maps to B. Horizontal movement is unnecessary. */
		state->cursortarget = state->cursorrow = 0;
		return 0;
	}

	state->cursortarget = target;
	if(target < 0 || target == state->cursorrow)
		return 0;
	if(target > state->cursorrow)
	{
		direction = 2;
		state->cursorrow++;
	}
	else
	{
		direction = 1;
		state->cursorrow--;
	}

	if(relative == 1)
	{
		ge_plus_levelmod_category = state->cursorrow;
		state->pluscategory = state->cursorrow;
	}
	else
	{
		if(state->cursorrow < state->listtop)
			state->listtop = state->cursorrow;
		if(state->cursorrow >= state->listtop + 10)
			state->listtop = state->cursorrow - 9;
	}

	state->direction = direction;
	state->heldms = 35 - elapsedms;
	if(state->heldms <= 0) state->releasems = 35;
	return direction;
}

/* One gesture produces one direction, held long enough for the emulation
 * thread to sample it. A neutral interval produces a fresh native edge.
 * At most one following gesture can accumulate, and idle input expires. */
static int GE_MenuNativePulse(GE_MENU_NATIVE_STATE *state, const float dx,
	const float dy, const int elapsedms)
{
	const float threshold = 8.0f;
	int direction;
	if(!(dx >= -1000000.0f && dx <= 1000000.0f
		&& dy >= -1000000.0f && dy <= 1000000.0f)
		|| elapsedms <= 0 || elapsedms > 50)
	{
		state->x = state->y = 0;
		state->direction = state->heldms = state->releasems = state->idlems = 0;
		return 0;
	}
	if(dx != 0 || dy != 0)
	{
		const float x = state->x + dx, y = state->y + dy;
		state->idlems = 0;
		/* Bound the pending gesture without changing its dominant axis. */
		if(fabsf(x) > threshold && fabsf(x) >= fabsf(y))
		{
			state->x = x < 0 ? -threshold : threshold;
			state->y = y * threshold / fabsf(x);
		}
		else if(fabsf(y) > threshold)
		{
			state->x = x * threshold / fabsf(y);
			state->y = y < 0 ? -threshold : threshold;
		}
		else
			state->x = x, state->y = y;
	}
	else
	{
		state->idlems = ClampInt(state->idlems + elapsedms, 0, 100);
		if(state->idlems == 100)
			state->x = state->y = 0;
	}
	if(state->heldms > 0)
	{
		direction = state->direction;
		state->heldms -= elapsedms;
		if(state->heldms <= 0)
			state->releasems = 50;
		return direction;
	}
	if(state->releasems > 0)
	{
		state->releasems -= elapsedms;
		return 0;
	}
	if(fabsf(state->x) < threshold && fabsf(state->y) < threshold)
		return 0;
	/* Prefer the vertical axis on a tie to avoid changing a value while
	 * scrolling its row. Do not emit both axes for a diagonal gesture. */
	if(fabsf(state->y) >= fabsf(state->x))
		direction = state->y < 0 ? 1 : 2;
	else
		direction = state->x < 0 ? 3 : 4;
	state->x = state->y = 0;
	state->direction = direction;
	state->heldms = 50 - elapsedms;
	if(state->heldms <= 0)
		state->releasems = 50;
	return direction;
}

static void GE_MenuNativeInputs(void)
{
	const GE_MENU_NATIVE_STATE empty = {0};
	for(int player = PLAYER1; player < ALLPLAYERS; player++)
	{
		GE_MENU_NATIVE_STATE *state = &ge_menu_native[player];
		const int context = GE_MenuNativeContext(player);
		const int fire = DEVICE[player].BUTTONPRIM[FIRE] || DEVICE[player].BUTTONSEC[FIRE];
		int direction;
		float sensitivity;
		if(!context)
		{
			const int blockfire = state->blockfire && fire;
			*state = empty;
			state->blockfire = blockfire;
			if(blockfire)
				CONTROLLER[player].Z_TRIG = 0;
			continue;
		}
		if(state->context != context)
		{
			const GE_ADDRESS_PROFILE *profile = GE_GetAddressProfile();
			const int page = EMU_ReadInt(profile->menupage);
			*state = empty;
			state->context = context;
			state->waitfire = fire; /* Do not accept with a click held while the menu opened. */
			if(player == PLAYER1 && profile->mapmaker.page)
			{
				const int relative = page - (int)profile->mapmaker.page;
				if(relative == 1)
					ge_plus_levelmod_category = 0; /* Plus init resets the category. */
				else if(relative == 2)
					state->pluscategory = ge_plus_levelmod_category; /* Level list resets row/top only. */
			}
		}
		if(!fire) state->waitfire = 0;
		/* A is assigned after cursor alignment below. Do not let a click accept
		 * a stale row while the visible highlight is still catching the cursor. */
		CONTROLLER[player].A_BUTTON = DEVICE[player].BUTTONPRIM[ACCEPT] || DEVICE[player].BUTTONSEC[ACCEPT];
		CONTROLLER[player].Z_TRIG = 0; // weapon-wheel aliases must not accept or fire in menus
		CONTROLLER[player].B_BUTTON |= DEVICE[player].BUTTONPRIM[AIM] || DEVICE[player].BUTTONSEC[AIM];
		/* Aim is Back here. Suppress its gameplay R alias while preserving
		 * the dedicated physical shoulder. */
#if !PD_DECOMP
		CONTROLLER[player].R_TRIG = DEVICE[player].BUTTONPRIM[R_SHOULDER] || DEVICE[player].BUTTONSEC[R_SHOULDER];
#else
		CONTROLLER[player].R_TRIG = 0;
#endif
		state->blockfire = fire;
		if(fire)
			CONTROLLER[player].Z_TRIG = 0;
		sensitivity = PROFILE[player].SETTINGS[SENSITIVITY] / 40.0f;
		if(PROFILE[player].SETTINGS[MOUSE] < 0 || !(sensitivity > 0 && sensitivity <= 1000))
		{
			state->x = state->y = 0;
			state->direction = state->heldms = state->releasems = state->idlems = 0;
			continue; // zero sensitivity disables movement, not fresh clicks
		}
		if(player == PLAYER1)
		{
			const GE_ADDRESS_PROFILE *profile = GE_GetAddressProfile();
			const int page = EMU_ReadInt(profile->menupage);
			direction = GE_PlusLevelModifierCursor(state, profile, page, TICKRATE);
		}
		else
			direction = -1;
		if(direction < 0)
			direction = GE_MenuNativePulse(state, DEVICE[player].XPOS / 10.0f * sensitivity,
				(context == 105 || context == 115 || context == 116 || context == 117)
					? 0 : DEVICE[player].YPOS / 10.0f * sensitivity, TICKRATE);
		CONTROLLER[player].U_DPAD |= direction == 1;
		CONTROLLER[player].D_DPAD |= direction == 2;
		CONTROLLER[player].L_DPAD |= direction == 3;
		CONTROLLER[player].R_DPAD |= direction == 4;

		/* Click once the hovered row is already aligned. Detail pages have one
		 * row, so click toggles immediately. */
		if(fire && !state->waitfire)
		{
			const GE_ADDRESS_PROFILE *profile = GE_GetAddressProfile();
			const int relative = player == PLAYER1 && profile->mapmaker.page
				? EMU_ReadInt(profile->menupage) - (int)profile->mapmaker.page : 0;
			if(relative < 1 || relative > 3 || relative == 3
				|| state->cursortarget < 0 || state->cursorrow == state->cursortarget)
				CONTROLLER[player].A_BUTTON = 1;
		}
	}
}
