#!/usr/bin/env python3
"""Execute production injector against real ROM code/data and modeled live RAM.
Not an interactive emulator test. Usage: test_plus24_candidate.py ROM [ROM...]
"""
from pathlib import Path
import subprocess,sys,tempfile,zlib
root=Path(__file__).resolve().parents[1]
original=root/'tests/test_goldeneye_resolver.py'
ns={'__file__':str(original)}
s=original.read_text();exec(s.split("harness = r'''")[0],ns)
h=s.split("harness = r'''",1)[1].split('static void duplicate_words',1)[0]
source=ns['source'].replace('static void EMU_WriteInt(unsigned int at, int v) {','static unsigned int writes;\nstatic void EMU_WriteInt(unsigned int at, int v) { writes++;')
source=source.replace('static unsigned int EMU_ReadROM(unsigned int at) {', '#include <time.h>\nstatic unsigned long rom_reads;\nstatic unsigned int EMU_ReadROM(unsigned int at) { rom_reads++;')
main=r'''
static void seed(const GE_ADDRESS_PROFILE *p,int page) {
 memset(PROFILE,0,sizeof(PROFILE));memset(DEVICE,0,sizeof(DEVICE));memset(CONTROLLER,0,sizeof(CONTROLLER));
 PROFILE[0].SETTINGS[CONFIG]=CUSTOM;PROFILE[0].SETTINGS[SENSITIVITY]=40;
 EMU_WriteInt(p->bonddata,0x80100000);EMU_WriteInt(p->camera,4);EMU_WriteInt(p->exit,1);EMU_WriteInt(p->pause,0);
 EMU_WriteInt(p->menupage,page);EMU_WriteFloat(p->menux,200);EMU_WriteFloat(p->menuy,160);EMU_WriteInt(p->matchended,0);
 EMU_WriteFloat(0x80100000+GE_camx,45);EMU_WriteFloat(0x80100000+GE_camy,0);EMU_WriteFloat(0x80100000+GE_fov,60);
 if(p->mapmaker.page) {
 const GE_MAPMAKER_PROFILE *m=&p->mapmaker;
 EMU_WriteInt(m->nextpage,-1);EMU_WriteInt(m->nextpagealt,-1);EMU_WriteInt(m->menu,0);EMU_WriteInt(m->preview,0);EMU_WriteInt(m->freemode,1);
 EMU_WriteFloat(m->yaw,1);EMU_WriteFloat(m->pitch,0);EMU_WriteInt(m->pitchmin,0xBFC90FDA);EMU_WriteInt(m->pitchmax,0x3FC90FDA);
 if(m->preview_yaw) {EMU_WriteFloat(m->preview_yaw,1);EMU_WriteFloat(m->preview_pitch,0);EMU_WriteInt(m->preview_pitchmin,0xBFC90FDA);EMU_WriteInt(m->preview_pitchmax,0x3FC90FDA);}
 }
}
int main(int argc,char **argv) {
 for(int image=1;image<argc;image++) {
 loadrom(argv[image]); const GE_ADDRESS_PROFILE *p=GE_GetAddressProfile();assert(p->bonddata); if(p->maxpage<28) assert(!p->mapmaker.page && !p->levelmod_page && !p->levelmod_category);
 if(p->maxpage==37) {
 GE_PLUS_RELOAD_PROFILE reload={0};assert(GE_ResolvePlus24Reload(&reload));assert(reload.count==42);
 const char *out=getenv("PLUS24_PATCH_OUTPUT");
 if(out) {FILE *f=fopen(out,"w");assert(f);for(unsigned int j=0;j<reload.count;j++)fprintf(f,"%08X %08X %08X\n",reload.address[j],reload.original[j],reload.replacement[j]);fclose(f);}
 }
 printf("ROM %s maxpage=%u editor=%u legacy-levelmod=%u\n",argv[image],p->maxpage,p->mapmaker.page,p->levelmod_page);
 for(int page=1;page<=(int)p->maxpage;page++) {
  if(page==11)continue;seed(p,page);assert(GAME_Status());
  DEVICE[0].XPOS=20;DEVICE[0].YPOS=4;DEVICE[0].BUTTONSEC[CANCEL]=1;
  GAME_Inject();assert(CONTROLLER[0].B_BUTTON);
  if(page>=31)assert(!CONTROLLER[0].U_DPAD&&!CONTROLLER[0].D_DPAD&&!CONTROLLER[0].L_DPAD&&!CONTROLLER[0].R_DPAD);
  DEVICE[0].BUTTONSEC[CANCEL]=0;DEVICE[0].XPOS=DEVICE[0].YPOS=0;GAME_Inject();assert(!CONTROLLER[0].B_BUTTON);
  DEVICE[0].BUTTONSEC[D_UP]=1;GAME_Inject();assert(CONTROLLER[0].U_DPAD);DEVICE[0].BUTTONSEC[D_UP]=0;GAME_Inject();assert(!CONTROLLER[0].U_DPAD);
 }
 if(p->mapmaker.page) {
  seed(p,p->mapmaker.page);assert(GAME_Status());DEVICE[0].XPOS=5;DEVICE[0].YPOS=3;
  test_editor_yaw=p->mapmaker.yaw;test_editor_pitch=p->mapmaker.pitch;test_write_mode=3;GAME_Inject();test_write_mode=0;
  assert(EMU_ReadFloat(p->mapmaker.yaw)!=1 && EMU_ReadFloat(p->mapmaker.pitch)!=0);
  EMU_WriteInt(p->mapmaker.preview,1);test_editor_yaw=p->mapmaker.preview_yaw;test_editor_pitch=p->mapmaker.preview_pitch;
  test_write_mode=3;GAME_Inject();test_write_mode=0;assert(EMU_ReadFloat(test_editor_yaw)!=1);
  EMU_WriteInt(p->mapmaker.preview_pitchmin,0);test_write_mode=2;GAME_Inject();test_write_mode=0;
  seed(p,p->mapmaker.page);EMU_WriteInt(p->mapmaker.menu,1);EMU_WriteInt(p->mapmaker.menu_selection,0);EMU_WriteInt(p->mapmaker.menu_tool,0);
  EMU_WriteFloat(p->menux,120);EMU_WriteFloat(p->menuy,p->mapmaker.menu_top+4*p->mapmaker.menu_stride+3);
  DEVICE[0].XPOS=1;GAME_Inject();assert(EMU_ReadInt(p->mapmaker.menu_selection)==4);
  DEVICE[0].XPOS=0;EMU_WriteInt(p->mapmaker.menu_selection,6);GAME_Inject();assert(EMU_ReadInt(p->mapmaker.menu_selection)==6);
 }
 seed(p,11);EMU_WriteInt(0x80100000+GE_watch,1);DEVICE[0].XPOS=10;DEVICE[0].YPOS=5;
 test_write_mode=2;assert(GAME_Status());GAME_Inject();test_write_mode=0;EMU_WriteInt(0x80100000+GE_watch,0);
 // A later/invalid page loses adaptation but retains fresh native controls.
 seed(p,p->maxpage+1);assert(!GAME_Status());DEVICE[0].BUTTONSEC[START]=1;DEVICE[0].BUTTONPRIM[FIRE]=1;
 DEVICE[0].BUTTONSEC[RELOAD]=1;DEVICE[0].BUTTONPRIM[DOWN]=1;
 test_write_mode=2;GAME_Inject();test_write_mode=0;
 assert(CONTROLLER[0].START_BUTTON&&CONTROLLER[0].Z_TRIG&&CONTROLLER[0].RELOAD_HACK&&CONTROLLER[0].X_AXIS==-127);
 memset(DEVICE,0,sizeof(DEVICE));test_write_mode=2;GAME_Inject();test_write_mode=0;assert(CONTROLLER[0].Value==0);
 PROFILE[1].SETTINGS[CONFIG]=DISABLED;DEVICE[1].BUTTONSEC[FIRE]=1;GAME_Inject();assert(CONTROLLER[1].Value==0);
 DEVICE[0].BUTTONSEC[FIRE]=1;GAME_Inject();assert(CONTROLLER[0].Z_TRIG);GAME_ClearInput();assert(CONTROLLER[0].Value==0);
 // Missing optional menu bounds cannot suppress raw buttons or permit RAM writes.
 seed(p,5);EMU_WriteFloat(p->menux,NAN);assert(!GAME_Status());DEVICE[0].BUTTONSEC[CANCEL]=1;
 test_write_mode=2;GAME_Inject();test_write_mode=0;assert(CONTROLLER[0].B_BUTTON);
 seed(p,6);assert(GAME_Status());rom_reads=0;clock_t began=clock();
 for(int tick=0;tick<10000;tick++) { assert(GAME_Status());GAME_Inject(); }
 assert(rom_reads<2000000UL);
 printf("10000 cached idle input polls: %.3f ms; %lu ROM reads (no repeated discovery)\n",1000.0*(clock()-began)/CLOCKS_PER_SEC,rom_reads);
 // Code patches are checked, owned, idempotent and restored on ROM lifecycle.
 loadrom(argv[image]);unsigned int *saved=malloc(test_rom_bytes);memcpy(saved,test_rom,test_rom_bytes);
 GE_PrepareROM();GE_PrepareROM();GE_Quit();assert(!memcmp(saved,test_rom,test_rom_bytes));free(saved);
 puts("PASS: every page routes/releases buttons; camera, preview, editor selection, watch scope, fallback, lifecycle");
 }
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='plus24-test-') as temp:
 c=Path(temp)/'test.c';c.write_text(source+'\n'+ns['game_source']+'\n'+h+main)
 exe=Path(temp)/'test'
 subprocess.run(['cc','-std=c11','-O2','-fgnu89-inline','-I',str(root/'games'),str(c),'-lm','-o',str(exe)],check=True)
 subprocess.run([str(exe),*sys.argv[1:]],check=True)
