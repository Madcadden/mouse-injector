#!/usr/bin/env python3
"""Production PD input on authentic beta code/data with modeled live RAM.

Usage: test_pd_beta_input.py NTSC-DC.rom PAL-debug.rom
Runs no emulated game frames and no Windows input devices. No ROM is distributed.
"""
from pathlib import Path
import importlib.util
import json
import subprocess
import sys
import tempfile
import zlib
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('gx_test',ROOT/'tests/test_goldeneye_x_input.py')
gx=importlib.util.module_from_spec(spec);spec.loader.exec_module(gx)

HARNESS=r'''
uint32_t *test_rom, test_ram[0x800000 / 4];
static uint32_t original_ram[0x800000 / 4];
size_t test_rom_bytes;
unsigned int test_forbid_writes, test_writes, test_reads;
BUTTONS CONTROLLER[4];
struct PROFILE_STRUCT PROFILE[4];
struct DEVICE_STRUCT DEVICE[4];
const unsigned char **rdramptr, **romptr;
static const unsigned char *test_pages[0x100000];
int mousetoggle=1,emuoverclock=1,overridefov=90;
int overrideratiowidth=16,overrideratioheight=9,geshowcrosshair,bypassviewmodelfovtweak=1;
static const unsigned int expected[2][7]={
 {0x8009E96C,0x8009E724,0x80072E10,0x80086874,0x800649F4,0x800649D4,0x800B1456},
 {0x8009E77C,0x8009E534,0x80072420,0x800864C4,0x80063BA4,0x80063B84,0x800B11C6}
};
static size_t load(const char *path,uint32_t *to,size_t limit){
 FILE*f=fopen(path,"rb");assert(f);fseek(f,0,SEEK_END);size_t n=ftell(f);rewind(f);assert(!(n&3)&&n<=limit);
 for(size_t i=0;i<n/4;i++){unsigned char b[4];assert(fread(b,1,4,f)==4);to[i]=(unsigned)b[0]<<24|(unsigned)b[1]<<16|(unsigned)b[2]<<8|b[3];}fclose(f);return n;
}
static void reset(int version){
 PD_Quit();assert(!pdhookstate&&!pdbetareloadstate&&!pdbetareload.count&&!pdbetaaim.count);test_forbid_writes=0;memcpy(test_ram,original_ram,sizeof(test_ram));
 memset(test_pages,0,sizeof(test_pages));for(unsigned i=0x80000;i<0x80800;i++)test_pages[i]=(unsigned char*)test_ram+(i-0x80000)*0x1000;rdramptr=test_pages;
 memset(PROFILE,0,sizeof(PROFILE));memset(DEVICE,0,sizeof(DEVICE));memset(CONTROLLER,0,sizeof(CONTROLLER));
 const unsigned *e=expected[version];EMU_WriteInt(e[0],1);EMU_WriteInt(e[3],0);EMU_WriteInt(e[4],1);EMU_WriteInt(e[6]&~3U,0);
 for(int p=0;p<4;p++){
  unsigned base=0x80500000+p*0x8000;memset(test_ram+(base&0x7fffff)/4,0,0x8000);
  EMU_WriteInt(e[1]+4*p,base);EMU_WriteInt(e[2]+4*p,1);
  EMU_WriteFloat(base+0x144,45);EMU_WriteFloat(base+0x154,0);EMU_WriteFloat(base+0x1848,60);
  PROFILE[p].SETTINGS[CONFIG]=CUSTOM;PROFILE[p].SETTINGS[SENSITIVITY]=40;PROFILE[p].SETTINGS[CROSSHAIR]=1;
 }
 assert(PD_Status());assert(pdcompat.camera==e[0]&&pdcompat.players==e[1]&&pdcompat.menu==e[2]&&pdcompat.pause==e[3]&&pdcompat.stage==e[4]&&pdcompat.intro==e[5]&&pdcompat.mppause==e[6]);
 assert(pdcompat.camera_beta==(version==0)&&pdcompat.mp_beta==(version==0));
 test_writes=test_reads=0;
}
static void buttons(void){
 for(int p=0;p<4;p++){
  DEVICE[p].XPOS=10*(p+1);DEVICE[p].YPOS=5*(p+1);
  DEVICE[p].BUTTONSEC[FORWARDS]=DEVICE[p].BUTTONSEC[FIRE]=DEVICE[p].BUTTONSEC[AIM]=1;
  DEVICE[p].BUTTONSEC[CANCEL]=DEVICE[p].BUTTONSEC[RELOAD]=DEVICE[p].BUTTONSEC[D_DOWN]=1;
 }
}
static void check_buttons(void){for(int p=0;p<4;p++)assert(CONTROLLER[p].U_CBUTTON&&CONTROLLER[p].Z_TRIG&&CONTROLLER[p].R_TRIG&&CONTROLLER[p].B_BUTTON&&CONTROLLER[p].D_DPAD);}
static void checks(int v){
 reset(v);buttons();PD_Inject();check_buttons();
 for(int p=0;p<4;p++){
  unsigned b=0x80500000+p*0x8000;
  assert(EMU_ReadFloat(b+0x144)==45.f+p+1&&EMU_ReadFloat(b+0x154)==-(p+1)*0.5f);
  assert(EMU_ReadFloat(b+0xCD4)>0&&EMU_ReadFloat(b+0x1478)>0&&EMU_ReadFloat(b+0x1668)>0);
  assert(EMU_ReadFloat(b+0x7F8)>0&&EMU_ReadFloat(b+0xF9C)>0);
  assert(EMU_ReadFloat(b+0xCD8)>0&&EMU_ReadFloat(b+0x147C)>0&&EMU_ReadFloat(b+0x166C)>0);
  assert(EMU_ReadFloat(b+0x7FC)>0&&EMU_ReadFloat(b+0xFA0)>0);
 }
 memset(DEVICE,0,sizeof(DEVICE));PD_Inject();for(int p=0;p<4;p++)assert(!CONTROLLER[p].Value);
 DEVICE[0].BUTTONSEC[CROUCH]=1;PD_Inject();assert(EMU_ReadInt(0x805000AC)==0);
 DEVICE[0].BUTTONSEC[CROUCH]=0;DEVICE[0].BUTTONSEC[KNEEL]=1;PD_Inject();assert(EMU_ReadInt(0x805000AC)==1);
 // Pause, multiplayer pause, frontend, death and invalid player all retain native buttons without writes.
 for(int mode=0;mode<7;mode++){
  reset(v);buttons();
  if(mode==0)EMU_WriteInt(pdcompat.pause,1);
  if(mode==1)EMU_WriteInt(pdcompat.mppause&~3U,0x0100);
  if(mode==2)EMU_WriteInt(pdcompat.camera,0);
  for(int p=0;p<4;p++){
   unsigned b=0x80500000+p*0x8000;
   if(mode==3)EMU_WriteInt(b+0xD8,1);
   if(mode==4)EMU_WriteInt(pdcompat.players+4*p,0x7FFFFFFC);
   if(mode==5)EMU_WriteFloat(b+0x1848,NAN);
   if(mode==6)test_pages[b>>12]=NULL;
  }
  test_forbid_writes=1;PD_Inject();test_forbid_writes=0;check_buttons();
 }
 reset(v);buttons();for(int p=0;p<4;p++)PROFILE[p].SETTINGS[CONFIG]=DISABLED;
 test_forbid_writes=1;PD_Inject();test_forbid_writes=0;for(int p=0;p<4;p++)assert(!CONTROLLER[p].Value);
 // Eyespy angles and matrix updates; normal body camera must remain untouched.
 reset(v);for(int p=1;p<4;p++)PROFILE[p].SETTINGS[CONFIG]=DISABLED;
 EMU_WriteInt(0x80500000,2);EMU_WriteInt(0x80500480,0x80540000);EMU_WriteInt(0x80540034,1);
 EMU_WriteFloat(0x8054001C,40);EMU_WriteFloat(0x80540028,0);DEVICE[0].XPOS=10;DEVICE[0].YPOS=5;PD_Inject();
 assert(EMU_ReadFloat(0x8054001C)==41.f&&EMU_ReadFloat(0x80540028)>359.f&&EMU_ReadFloat(0x80500144)==45.f);
 assert(EMU_ReadFloat(0x8054002C)>0.99f&&EMU_ReadFloat(0x80540030)<0);
 EMU_WriteFloat(0x8054001C,NAN);test_forbid_writes=1;PD_Inject();test_forbid_writes=0;
 // Hoverbike yaw/roll use a validated prop->obj chain and preserve body yaw.
 reset(v);for(int p=1;p<4;p++)PROFILE[p].SETTINGS[CONFIG]=DISABLED;
 EMU_WriteInt(0x805001B0,3);EMU_WriteInt(0x80501A6C,0x80540000);EMU_WriteInt(0x80540004,0x80541000);
 EMU_WriteFloat(0x8054106C,3.f);EMU_WriteFloat(0x805410BC,0);DEVICE[0].XPOS=10;PD_Inject();
 assert(EMU_ReadFloat(0x8054106C)<3.f&&EMU_ReadFloat(0x805410BC)>0&&EMU_ReadFloat(0x80500144)==45.f);
 // Boot settings and beta hooks target only their verified instruction/cave sites.
 reset(v);EMU_WriteInt(pdcompat.stage,0);PD_Inject();assert(pdhookstate==2);
#ifndef SPEEDRUN_BUILD
 assert(pdbetareloadstate);
#else
 assert(!pdbetareloadstate);
#endif
 assert(PD_camspylookspringup && PD_camspylookspringdown);
 assert(!EMU_ReadInt(PD_camspylookspringup)&&!EMU_ReadInt(PD_camspylookspringdown));
#ifndef SPEEDRUN_BUILD
 assert((unsigned)EMU_ReadInt(PD_controlstyle)==0x34020001U&& (unsigned)EMU_ReadInt(PD_reversepitch)==0x34020001U);
 assert((unsigned)EMU_ReadInt(PD_defaultfov)==0x3C0142B4U&& (unsigned)EMU_ReadInt(PD_defaultfovzoom)==0x3C0142B4U);
#endif
 // Actual aiming flag: normal mouse look and the optional cursor mode both work.
 EMU_WriteInt(pdcompat.stage,1);
 for(int p=1;p<4;p++)PROFILE[p].SETTINGS[CONFIG]=DISABLED;
 EMU_WriteInt(0x80500120,1);EMU_WriteInt(0x80501588,7);
 PROFILE[0].SETTINGS[PDAIMMODE]=0;DEVICE[0].XPOS=10;DEVICE[0].YPOS=5;PD_Inject();
 assert(EMU_ReadFloat(0x80500144)>45.f&&EMU_ReadFloat(0x80500154)<0);
 PROFILE[0].SETTINGS[PDAIMMODE]=1;DEVICE[0].XPOS=1000;DEVICE[0].YPOS=100;
 PD_Inject();PD_Inject();
 assert(EMU_ReadFloat(0x80501668)>18.f&&EMU_ReadFloat(0x8050166C)>0);
 assert(EMU_ReadFloat(0x80500144)>46.f&&EMU_ReadFloat(0x80500CD4)>0&&EMU_ReadFloat(0x80501478)>0);
 // A restored or foreign-modified hook loses its unsafe optional input mode.
 unsigned site=pdbetaaim.address[0];EMU_WriteInt(site,pdbetaaim.original[0]);assert(PD_Status());assert(!pdhookstate);
#ifndef SPEEDRUN_BUILD
 site=pdbetareload.words[0].address;EMU_WriteInt(site,0xFFFFFFFF);assert(PD_Status());assert(!pdbetareloadstate);
#endif
 // Whole unpatched RAM restoration also drops cached optional hooks; closing
 // the ROM then reloading pristine code permits a fresh owned installation.
 reset(v);EMU_WriteInt(pdcompat.stage,0);PD_Inject();assert(pdhookstate==2);
 memcpy(test_ram,original_ram,sizeof(test_ram));(void)PD_Status();assert(!pdhookstate&&!pdbetareloadstate);
 reset(v);EMU_WriteInt(pdcompat.stage,0);PD_Inject();assert(pdhookstate==2);
 // Both mandatory variant windows enforce ownership/uniqueness; no header-only recognition.
 for(int key=0;key<2;key++){
  reset(v);unsigned at=pdcompat.matches[key?PDS_MP:PDS_CAMERA];unsigned count=key?(v?12:11):(v?10:30);
  memcpy(test_ram+0x600000/4,test_ram+(at&0x7fffff)/4,count*4);PD_COMPAT_PROFILE p;
  assert(!PD_CompatResolve(PD_ReadMapped,0,&p));
 }
 reset(v);unsigned a=pdcompat.matches[PDS_CAMERA];EMU_WriteInt(a+8,EMU_ReadInt(a+8)^0x40000000U);
 PD_COMPAT_PROFILE p;assert(!PD_CompatResolve(PD_ReadMapped,0,&p));
 reset(v);test_pages[pdcompat.camera>>12]=NULL;assert(!PD_Status());
 puts("PASS beta: production discovery, 4-player mouse/buttons/releases, weapon sway, crouch, eyespy, hoverbike, pause/death/NaN/unmapped guards, disabled profiles, boot settings, normal/cursor aim, ROM lifecycle, restored-hook ownership and malformed/duplicate rejection");
}
int main(int argc,char **argv){assert(argc==5);test_rom=malloc(64*1024*1024);assert(test_rom);
 for(int v=0;v<2;v++){test_rom_bytes=load(argv[1+v*2],test_rom,64*1024*1024);romptr=(const unsigned char**)test_rom;assert(load(argv[2+v*2],original_ram,sizeof(original_ram))==sizeof(original_ram));checks(v);}
 PD_Quit();assert(!pdcompat.valid&&!pdhookstate&&!pdbetareloadstate);free(test_rom);return 0;}
'''

def fixture(path,v):
 b=path.read_bytes()
 if b[:4]==bytes.fromhex('37804012'):
  x=bytearray(len(b));x[0::2],x[1::2]=b[1::2],b[0::2];b=bytes(x)
 assert b[:4]==bytes.fromhex('80371240')
 assert b[0x10:0x18].hex()==['2ce75aefe16825bc','f9864452890a5ea3'][v]
 data_at,game_at=(0x30850,0x43c40) if v==0 else (0x39850,0x4fc40)
 lib=b[0x1050:0x3050]+zlib.decompress(b[0x3055:],-15)
 data=zlib.decompress(b[data_at+5:],-15);game=bytearray()
 for i in range(0,0x4000,4):
  at=game_at+int.from_bytes(b[game_at+i:game_at+i+4],'big')+2
  if b[at:at+2]!=b'\x11\x73':break
  part=zlib.decompress(b[at+5:at+0x1000],-15);game.extend(part)
  if len(part)!=0x1000:break
 assert (len(lib),len(data),len(game))==[(370448,204016,1785856),(366896,207232,1818048)][v]
 ram=bytearray(0x800000)
 for at,part in [(0x1050,lib),(0x1050+len(lib),data),(0x220000,game)]:ram[at:at+len(part)]=part
 return b,ram

def main():
 with tempfile.TemporaryDirectory(prefix='pd-beta-') as temp:
  temp=Path(temp);(temp/'fixture_memory.h').write_text(gx.MEMORY)
  (temp/'test.c').write_text(gx.instrument((ROOT/'games/perfectdark.c').read_text())+'\n'+HARNESS)
  args=[]
  for v,path in enumerate(sys.argv[1:]):
   b,ram=fixture(Path(path),v);rp=temp/f'{v}.z64';mp=temp/f'{v}.ram';rp.write_bytes(b);mp.write_bytes(ram);args.extend([str(rp),str(mp)])
  for mode,extra in [('normal',[]),('speedrun',['-DSPEEDRUN_BUILD'])]:
   exe=temp/'test';subprocess.run(['cc','-std=c11','-O2','-Dinline=static inline',*extra,'-I',str(temp),'-I',str(ROOT/'games'),str(temp/'test.c'),'-lm','-o',str(exe)],check=True)
   subprocess.run([str(exe),*args],check=True);print(mode+' passed',flush=True)
 print('Limits: real production C and decoded ROM code/data; modeled runtime state, no emulated game frames or Windows input.')
if __name__=='__main__':main()
