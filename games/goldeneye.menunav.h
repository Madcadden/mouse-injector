/* Native watch and multiplayer menus use direction/button input rather than
 * the frontend crosshair. Keep this adapter local to those active menus. */
typedef struct GE_MENU_NATIVE_STATE
{
	float x, y;
	int context, direction, heldms, releasems, idlems, blockfire, waitfire;
} GE_MENU_NATIVE_STATE;

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
	int dead, watch, multiplayer, ended;
	if(PROFILE[player].SETTINGS[CONFIG] == DISABLED || !mousetoggle)
		return 0;
	/* These frontend panels read each player's left/right controller
	 * direction, while their crosshair is used only for the Back tab. */
	if(page == 15 || page == 16 || page == 17)
		return 100 + page;
	if(player == PLAYER1 && page == 20)
		return 120;
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
	watch = EMU_ReadInt(base + GE_watch);
	multiplayer = EMU_ReadInt(base + GE_multipausemenu);
	ended = EMU_ReadInt(GE_matchended);
	/* The native watch handler runs only in animation state 5. The other
	 * nonzero values are opening, closing or mission-exit transitions. */
	if(player == PLAYER1 && watch == 5 && dead == 0 && multiplayer == 0)
		return 1;
	/* A round-end countdown >= 2 does not accept input yet. Ordinary death
	 * without a completed round does not expose a multiplayer menu either. */
	if(multiplayer == 1 && (ended == 0 || ended == 1)
		&& (dead == 0 || (dead == 1 && ended == 1)))
		return 2;
	return 0;
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
			*state = empty;
			state->context = context;
			state->waitfire = fire; /* Do not accept with a click held while the menu opened. */
		}
		if(!fire) state->waitfire = 0;
		/* Fire clicks also need A: multiplayer menus do not accept Z.
		 * Leave the separately bound N64 buttons and keyboard arrows intact. */
		CONTROLLER[player].A_BUTTON = DEVICE[player].BUTTONPRIM[ACCEPT] || DEVICE[player].BUTTONSEC[ACCEPT]
			|| (fire && !state->waitfire);
		CONTROLLER[player].Z_TRIG = 0; // weapon-wheel aliases must not accept or fire in menus
		CONTROLLER[player].B_BUTTON |= DEVICE[player].BUTTONPRIM[AIM] || DEVICE[player].BUTTONSEC[AIM];
		/* Aim is Back here. Sending its usual R at the same time would turn
		 * the watch page as well. Preserve the dedicated physical shoulder. */
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
		direction = GE_MenuNativePulse(state, DEVICE[player].XPOS / 10.0f * sensitivity,
			(context == 105 || context == 115 || context == 116 || context == 117)
				? 0 : DEVICE[player].YPOS / 10.0f * sensitivity, TICKRATE);
		CONTROLLER[player].U_DPAD |= direction == 1;
		CONTROLLER[player].D_DPAD |= direction == 2;
		CONTROLLER[player].L_DPAD |= direction == 3;
		CONTROLLER[player].R_DPAD |= direction == 4;
	}
}
