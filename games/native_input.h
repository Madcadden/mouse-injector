/* Standard 1.2 N64 button fallback: no ROM/RAM reads or writes. Profiles
 * already contain the user's primary/secondary bindings and device routing. */
static int GAME_Button(const int player, const int button)
{
 return DEVICE[player].BUTTONPRIM[button] || DEVICE[player].BUTTONSEC[button];
}
extern void GE_InputLost(void);
void GAME_ClearInput(void)
{
 GE_InputLost();
 for(int player=0; player<ALLPLAYERS; player++) CONTROLLER[player].Value = 0;
}
static void GAME_NativeInput(void)
{
 GAME_ClearInput();
 for(int player=0; player<ALLPLAYERS; player++)
 {
  if(PROFILE[player].SETTINGS[CONFIG] == DISABLED) continue;
  CONTROLLER[player].U_CBUTTON=GAME_Button(player,FORWARDS);
  CONTROLLER[player].D_CBUTTON=GAME_Button(player,BACKWARDS);
  CONTROLLER[player].L_CBUTTON=GAME_Button(player,STRAFELEFT);
  CONTROLLER[player].R_CBUTTON=GAME_Button(player,STRAFERIGHT);
  CONTROLLER[player].U_DPAD=GAME_Button(player,D_UP);
  CONTROLLER[player].D_DPAD=GAME_Button(player,D_DOWN);
  CONTROLLER[player].L_DPAD=GAME_Button(player,D_LEFT);
  CONTROLLER[player].R_DPAD=GAME_Button(player,D_RIGHT);
  CONTROLLER[player].Z_TRIG=GAME_Button(player,FIRE)||GAME_Button(player,PREVIOUSWEAPON);
  CONTROLLER[player].R_TRIG=GAME_Button(player,AIM);
#if !PD_DECOMP
  CONTROLLER[player].R_TRIG |= GAME_Button(player,R_SHOULDER);
#endif
  CONTROLLER[player].L_TRIG=GAME_Button(player,L_SHOULDER);
  CONTROLLER[player].A_BUTTON=GAME_Button(player,ACCEPT)||GAME_Button(player,PREVIOUSWEAPON)||GAME_Button(player,NEXTWEAPON);
  CONTROLLER[player].B_BUTTON=GAME_Button(player,CANCEL);
  CONTROLLER[player].START_BUTTON=GAME_Button(player,START);
#ifndef SPEEDRUN_BUILD
  CONTROLLER[player].RELOAD_HACK=GAME_Button(player,RELOAD);
#endif
  /* The plugin ABI names these axes in its historical order. Match both
   * game drivers; menus must never receive the overflowing -128 down value. */
  CONTROLLER[player].X_AXIS=127*(GAME_Button(player,UP)-GAME_Button(player,DOWN));
  CONTROLLER[player].Y_AXIS=127*GAME_Button(player,RIGHT)-128*GAME_Button(player,LEFT);
 }
}
