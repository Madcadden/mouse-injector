/* The Plus editor and its chooser draw the ordinary red cursor, but their
 * selectors read digital buttons only. Bridge only the verified layouts. */
static int ge_menu_mouse_context, ge_menu_mouse_hover = -1, ge_menu_mouse_adjust;
static int ge_menu_mouse_previousfire, ge_menu_mouse_firepressed;
static int ge_menu_mouse_clickcontext, ge_menu_mouse_clickadjust, ge_menu_mouse_blockfire;

static void GE_MenuMouseReset(void)
{
	ge_menu_mouse_context = ge_menu_mouse_adjust = 0;
	ge_menu_mouse_hover = -1;
	ge_menu_mouse_previousfire = ge_menu_mouse_firepressed = 0;
	ge_menu_mouse_clickcontext = ge_menu_mouse_clickadjust = ge_menu_mouse_blockfire = 0;
}

static void GE_MenuMouseFrameBegin(void)
{
	const int fire = DEVICE[PLAYER1].BUTTONPRIM[FIRE] || DEVICE[PLAYER1].BUTTONSEC[FIRE];
	ge_menu_mouse_context = ge_menu_mouse_adjust = 0;
	ge_menu_mouse_hover = -1;
	ge_menu_mouse_firepressed = fire && !ge_menu_mouse_previousfire;
	ge_menu_mouse_previousfire = fire;
	if(!fire)
		ge_menu_mouse_clickcontext = ge_menu_mouse_clickadjust = ge_menu_mouse_blockfire = 0;
	if(!mousetoggle || PROFILE[PLAYER1].SETTINGS[CONFIG] == DISABLED)
	{
		GE_MenuMouseReset();
		ge_menu_mouse_previousfire = fire;
		ge_menu_mouse_blockfire = fire; // recapturing cannot create a new held click
	}
}

static int GE_MapMakerMenuMouse(const GE_ADDRESS_PROFILE *profile, const float sensitivity)
{
	const GE_MAPMAKER_PROFILE *editor = &profile->mapmaker;
	const int page = EMU_ReadInt(profile->menupage);
	const float dx = DEVICE[PLAYER1].XPOS, dy = DEVICE[PLAYER1].YPOS;
	unsigned int selector = 0;
	int selected, tool = 0, hover = -1;
	float x, y, right = 354;
	if(!mousetoggle || !editor->page
		|| EMU_ReadInt(editor->nextpage) != -1 || EMU_ReadInt(editor->nextpagealt) != -1)
		return 0;
	if(page == (int)editor->page && editor->menu_selection && editor->menu_tool
		&& EMU_ReadInt(editor->menu) == 1 && EMU_ReadInt(editor->preview) == 0)
	{
		selector = editor->menu_selection;
		selected = EMU_ReadInt(selector);
		tool = EMU_ReadInt(editor->menu_tool);
		if(selected < 0 || selected >= 14 || tool < 0 || tool >= 5) return 0;
		ge_menu_mouse_context = 1;
		right = tool == 1 ? 310 : 354;
	}
	else if(editor->chooser_selection && page == (int)editor->chooser_page)
	{
		selector = editor->chooser_selection;
		selected = EMU_ReadInt(selector);
		if(selected < 0 || selected > 1) return 0;
		ge_menu_mouse_context = 2;
	}
	else return 0;
	x = EMU_ReadFloat(profile->menux);
	y = EMU_ReadFloat(profile->menuy);
	if(!isfinite(x) || !isfinite(y) || !isfinite(dx) || !isfinite(dy)
		|| !isfinite(sensitivity) || sensitivity < 0
		|| x < 20 || x > 420 || y < 20 || y > 310)
		return 1;
	if(dx || dy)
	{
		const float nextx = x + dx / 10.0f * sensitivity * 6;
		const float nexty = y + dy / 10.0f * sensitivity * (400.0f / 290.0f * 6);
		if(!isfinite(nextx) || !isfinite(nexty)) return 1;
		x = ClampFloat(nextx, 20, 420);
		y = ClampFloat(nexty, 20, 310);
		EMU_WriteFloat(profile->menux, x);
		EMU_WriteFloat(profile->menuy, y);
	}
	if(ge_menu_mouse_context == 1)
	{
		if(x >= 86 && x <= right && y >= 51 && y < 317)
			hover = (int)((y - 51) / 19);
		/* Grid has no A/Z action. Material/music values have native +/-
		 * input, whereas clicking their labels keeps Z's original action. */
		if(hover == 10 || ((hover == 5 || hover == 8) && x >= 224))
			ge_menu_mouse_adjust = x >= 224 && x < 260 ? -1 : 1;
	}
	else if(x >= 70 && x <= 400)
	{
		if(y >= 90 && y < 110) hover = 0;
		else if(y >= 120 && y < 140) hover = 1;
	}
	ge_menu_mouse_hover = hover;
	/* Stationary mouse input must not undo keyboard navigation. A fresh
	 * click selects its visible row before native activation is delivered. */
	if(hover >= 0 && (dx || dy || ge_menu_mouse_firepressed)
		&& (!ge_menu_mouse_clickcontext || ge_menu_mouse_firepressed)
		&& selected != hover)
		EMU_WriteInt(selector, hover);
	return 1;
}

static void GE_MenuMouseInputs(void)
{
	const int fire = DEVICE[PLAYER1].BUTTONPRIM[FIRE] || DEVICE[PLAYER1].BUTTONSEC[FIRE];
	if(ge_menu_mouse_context)
	{
		/* Weapon-wheel aliases must not also accept a row or place a block.
		 * The separately bound Accept/Cancel and native buttons still work. */
		CONTROLLER[PLAYER1].A_BUTTON = DEVICE[PLAYER1].BUTTONPRIM[ACCEPT] || DEVICE[PLAYER1].BUTTONSEC[ACCEPT];
		CONTROLLER[PLAYER1].Z_TRIG = 0;
		CONTROLLER[PLAYER1].B_BUTTON |= DEVICE[PLAYER1].BUTTONPRIM[AIM] || DEVICE[PLAYER1].BUTTONSEC[AIM];
#if !PD_DECOMP
		CONTROLLER[PLAYER1].R_TRIG = DEVICE[PLAYER1].BUTTONPRIM[R_SHOULDER] || DEVICE[PLAYER1].BUTTONSEC[R_SHOULDER];
#else
		CONTROLLER[PLAYER1].R_TRIG = 0;
#endif
		if(fire)
		{
			ge_menu_mouse_blockfire = 1;
			if(ge_menu_mouse_firepressed && ge_menu_mouse_hover >= 0)
			{
				ge_menu_mouse_clickcontext = ge_menu_mouse_context;
				ge_menu_mouse_clickadjust = ge_menu_mouse_adjust;
			}
			if(ge_menu_mouse_clickcontext == ge_menu_mouse_context)
			{
				if(ge_menu_mouse_clickadjust < 0) CONTROLLER[PLAYER1].L_DPAD = 1;
				else if(ge_menu_mouse_clickadjust > 0) CONTROLLER[PLAYER1].R_DPAD = 1;
				else CONTROLLER[PLAYER1].Z_TRIG = 1;
			}
		}
	}
	else
	{
		ge_menu_mouse_clickcontext = 0;
		if(ge_menu_mouse_blockfire && fire)
			CONTROLLER[PLAYER1].Z_TRIG = 0;
	}
}
