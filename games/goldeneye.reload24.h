/* Plus 2.4 adds two native context actions before its tank/interaction path.
 * Keep those calls for B; reload-only bypasses both and carries bit 0x40 to
 * the existing reload/interaction dispatcher. No extra code cave is used. */
static int GE_ResolvePlus24Reload(GE_PLUS_RELOAD_PROFILE *r)
{
 static const unsigned int ip[10] = {0x8E180000,0x8FA20064,0x8F030124,0x304A4000,0x000A482B,0x2C650001,0xAFA501FC,0xAFA50188,0xAFA901F0,0xAFA90044};
 static const unsigned int im[10] = {~0U,~0U,~0U,~0U,~0U,~0U,~0U,~0U,~0U,~0U};
 static const unsigned int ep[7] = {0x97AD020E,0x8FB90060,0xA7A3013C,0x01A0C027,0x03385024,0x0C000000,0xAFAA0064};
 static const unsigned int em[7] = {~0U,~0U,~0U,~0U,~0U,0xFC000000,~0U};
 static const unsigned int ap[26] = {
  0x8FAB01F0,0x516000AE,0x8FB90184,0x0C000000,0,0x10400005,0,0x0C000000,0,
  0x100000A6,0x8FB90184,0x0C000000,0,0x10400005,0x3C190000,0x0C000000,0,
  0x1000009E,0x8FB90184,0x8F390000,0x24010001,0x3C180000,0x1721002F,0x3C020000,0x8F180000,0x24040020};
 static const unsigned int am[26] = {
  ~0U,~0U,~0U,0xFC000000,~0U,~0U,~0U,0xFC000000,~0U,~0U,~0U,0xFC000000,~0U,~0U,
  0xFFFF0000,0xFC000000,~0U,~0U,~0U,0xFFFF0000,~0U,0xFFFF0000,~0U,0xFFFF0000,0xFFFF0000,~0U};
 static const unsigned int fp[8] = {0xAC200000,0x10000005,0x8FB90184,0x8E0B0000,0x240F0001,0xAD6F00D0,0x8FB90184,0x1720000B};
 const GE_ADDRESS_PROFILE *p = GE_GetAddressProfile();
 unsigned int input, action, flag, weapon, logic, current, codebase, end, nontank, high, low, prop, n = 0;
 unsigned int replacement[26];
 if(!GE_PlusReloadTitle() || !p->bonddata || GE_GetReloadHackProfile()) return 0;
 input = GE_FindUniqueROMPattern(ip, im, 10);
 action = GE_FindUniqueROMPattern(ap, am, 26);
 flag = GE_FindUniqueROMPattern(fp, gereloadflagmask, 8);
 weapon = GE_FindUniqueROMPattern(gereloadweaponpattern, gereloadweaponmask, 8);
 logic = GE_FindUniqueROMPattern(gereloadlogicpattern, gereloadlogicmask, 11);
 if(input < 0x5C || !action || !flag || !weapon || logic < 0x164
  || !GE_ROMPatternMatches(input - 0x5C, ep, em, 7)
  || action <= input || action - input > 0x1000 || flag <= action || flag - action > 0x800) return 0;
 end = action + 8 + (short)EMU_ReadROM(action + 4) * 4;
 nontank = action + 23 * 4 + (short)EMU_ReadROM(action + 22 * 4) * 4;
 high = EMU_ReadROM(action + 14 * 4) & 0xFFFF;
 low = EMU_ReadROM(action + 19 * 4) & 0xFFFF;
 prop = EMU_ReadROM(action + 24 * 4) & 0xFFFF;
 if(end != flag + 28 || action + 40 + (short)EMU_ReadROM(action + 36) * 4 != end
  || action + 72 + (short)EMU_ReadROM(action + 68) * 4 != end
  || nontank <= action + 104 || nontank >= flag
  || GE_MakeAddress(EMU_ReadROM(action + 56), EMU_ReadROM(action + 76)) != p->tankflag
  || (EMU_ReadROM(action + 84) & 0xFFFF) != high || (EMU_ReadROM(action + 92) & 0xFFFF) != high
  || !GE_ValidDataAddress(GE_MakeAddress(EMU_ReadROM(action + 84), EMU_ReadROM(action + 96)),4)
  || EMU_ReadROM(logic + 28) != EMU_ReadROM(logic + 36)
  || (EMU_ReadROM(logic - 4) & 0xFC000000) != 0x0C000000) return 0;
 codebase = (0x80000000U | ((EMU_ReadROM(logic - 4) & 0x03FFFFFFU) << 2)) - weapon;
 current = GE_MakeAddress(EMU_ReadROM(weapon), EMU_ReadROM(weapon + 4));
 if((codebase & 0xFF800003U) != 0x80000000U || !GE_ValidDataAddress(current,4)
  || GE_MakeAddress(EMU_ReadROM(weapon + 16), EMU_ReadROM(weapon + 20)) != current
  || (EMU_ReadROM(logic - 0x164) & 0xFFFF0000U) != 0x3C100000U
  || (EMU_ReadROM(logic - 0x160) & 0xFFFF0000U) != 0x26100000U
  || GE_MakeAddress(EMU_ReadROM(logic - 0x164), EMU_ReadROM(logic - 0x160)) != current) return 0;
 /* The tank flag is boolean, as in the native legacy reload adapter. The
  * shared high half also supplies v0 for the untouched non-tank path. */
 replacement[0]=0x8FAB0064; replacement[1]=0x31790040;
 replacement[2]=0x17200000U | ((flag + 12 - (action + 12)) / 4);
 replacement[3]=0x316B4000;
 replacement[4]=0x11600000U | ((end - (action + 20)) / 4);
 replacement[5]=0x8FB90184;
 replacement[6]=EMU_ReadROM(action+12); replacement[7]=0;
 replacement[8]=0x10400005; replacement[9]=0;
 replacement[10]=EMU_ReadROM(action+28); replacement[11]=0;
 replacement[12]=0x10000000U | ((end-(action+52))/4); replacement[13]=0x8FB90184;
 replacement[14]=EMU_ReadROM(action+44); replacement[15]=0;
 replacement[16]=0x10400005; replacement[17]=0x3C020000U|high;
 replacement[18]=EMU_ReadROM(action+60); replacement[19]=0;
 replacement[20]=0x10000000U | ((end-(action+84))/4); replacement[21]=0x8FB90184;
 replacement[22]=0x8C590000U|low; replacement[23]=0x8C580000U|prop;
 replacement[24]=0x13200000U | ((nontank-(action+100))/4); replacement[25]=0x24040020;
 for(unsigned int i=0;i<26;i++) { r->address[n]=action+i*4; r->replacement[n++]=replacement[i]; }
 r->address[n]=input+12; r->replacement[n++]=EMU_ReadROM(input+12)|0x40;
 r->address[n]=flag+16; r->replacement[n++]=0x8FAF0064;
 r->address[n]=weapon; r->replacement[n++]=0x8C4200D0;
 r->address[n]=weapon+4; r->replacement[n++]=0x304B0040;
 r->address[n]=weapon+12; r->replacement[n++]=0x304A4000;
 { const unsigned int tail[11] = {0x8E020000,0x51600005,0,EMU_ReadROM(logic+28),0x00002025,
    EMU_ReadROM(logic+36),0x24040001,0x11400003,0,EMU_ReadROM(logic+12),0};
   for(unsigned int i=0;i<11;i++) {r->address[n]=logic+i*4;r->replacement[n++]=tail[i];}
 }
 r->count=n;
 for(unsigned int i=0;i<n;i++) r->original[i]=EMU_ReadROM(r->address[i]);
 return 1;
}
