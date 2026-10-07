#ifndef PERFECTDARK_BETA_AIM_H
#define PERFECTDARK_BETA_AIM_H

/* The two beta crosshair routines retain the exact retail instruction stream
 * and player/hand layout. Their physical and virtual placements differ. Keep
 * these 33 replacements separate from the reload patch's cave words +58..64.
 * Call only after the beta reload resolver has validated the whole mapping. */
typedef struct PD_BETA_AIM_PATCH {
 unsigned int count, address[33], original[33], replacement[33];
} PD_BETA_AIM_PATCH;

static int PD_BetaAimResolve(PD_COMPAT_READER read, void *context,
 const PD_COMPAT_PROFILE *profile, unsigned int gamebase, PD_BETA_AIM_PATCH *patch)
{
 static const unsigned int offsets[33] = {
  0x1C,0x20,0x50,0x54,0x60,0x64,0x6C,0x84,0x88,0x90,0x94,
  0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,64,68,72,76,80,84
 };
 static const unsigned int words[33] = {
  0x0BC69E62,0x8EA10120,0x0BC69E67,0x263107A4,0x0BC69E6B,0x4614C500,0x46120682,0x0BC69E6F,0x26100004,0x0BC69E73,0x4614C500,
  0x54200003,0,0xE6B21668,0xE6A8166C,0x0BC281F0,0x8EA10120,0x50200001,0xE6380530,0x0BC281FD,0x8EA10120,0x50200001,0xE6340534,0x0BC28201,0x8EA10120,0x50200001,0xE6380530,0x0BC2820A,0x8EA10120,0x50200001,0xE6340534,0x0BC2820D,0
 };
 unsigned int cross = profile->matches[PDS_CROSSHAIR], cave = profile->matches[PDS_CAVE];
 unsigned int word, i, cv, xv;
 memset(patch,0,sizeof(*patch));
 if(!profile->valid || !gamebase ||
  !((cross == gamebase+0x9E550 && cave == gamebase+0x1A1628) ||
    (cross == gamebase+0xA0A10 && cave == gamebase+0x1A9638)) ||
  !PD_CompatMatch(read,context,cross,&pd_signatures[PDS_CROSSHAIR])) return 0;
 for(i=0;i<22;i++)
  if(!read(context,cave+i*4,&word) || word != pd_cave_words[i]) return 0;
 cv=0x7F000000U+cave-gamebase; xv=0x7F000000U+cross-gamebase;
 for(i=0;i<33;i++) {
  unsigned int replacement=words[i];
  patch->address[i]=(i<11?cross:cave)+offsets[i];
  if(!read(context,patch->address[i],&patch->original[i])) return 0;
  if((replacement>>26)==2) {
   unsigned int to=0x70000000U|((replacement&0x03FFFFFFU)<<2);
   if(to>=0x7F1A7988U && to<0x7F1A79E0U) to=cv+to-0x7F1A7988U;
   else if(to>=0x7F0A079CU && to<0x7F0A0848U) to=xv+to-0x7F0A079CU;
   else return 0;
   replacement=0x08000000U|((to>>2)&0x03FFFFFFU);
  }
  patch->replacement[i]=replacement;
 }
 patch->count=33;
 return 1;
}
#endif
