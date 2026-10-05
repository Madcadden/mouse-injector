#!/usr/bin/env python3
"""Execute actual beta cursor-aim patch sites in MIPS with aiming off/on.
Usage: test_pd_beta_aim_mips.py NTSC-DC.rom PAL-debug.rom
Requires Unicorn; compiles production headers to produce the patch plans.
"""
from pathlib import Path
import importlib.util
import struct
import subprocess
import sys
import tempfile
from unicorn import *
from unicorn.mips_const import *
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('beta_input',ROOT/'tests/test_pd_beta_input.py')
beta=importlib.util.module_from_spec(spec);spec.loader.exec_module(beta)
C=r'''
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "perfectdark.compat.h"
#include "perfectdark.beta_reload.h"
#include "perfectdark.beta_aim.h"
static unsigned char ram[0x800000];
static int readram(void*c,unsigned a,unsigned*v){(void)c;unsigned o=a-0x80000000U;if((a&3)||o>sizeof(ram)-4)return 0;*v=(unsigned)ram[o]<<24|(unsigned)ram[o+1]<<16|(unsigned)ram[o+2]<<8|ram[o+3];return 1;}
int main(int ac,char**av){assert(ac==2);FILE*f=fopen(av[1],"rb");assert(f&&fread(ram,1,sizeof(ram),f)==sizeof(ram));fclose(f);PD_COMPAT_PROFILE p;PD_BETA_RELOAD_PLAN r;PD_BETA_AIM_PATCH a;assert(PD_CompatResolve(readram,0,&p));assert(PD_BetaReloadPrepare(readram,0,&p,&r));assert(PD_BetaAimResolve(readram,0,&p,r.gamebase,&a));printf("%x %x %x\n",r.gamebase,p.matches[PDS_CROSSHAIR],p.matches[PDS_CAVE]);for(unsigned i=0;i<a.count;i++)printf("%x %x %x\n",a.address[i],a.original[i],a.replacement[i]);return 0;}
'''
p32=lambda v:struct.pack('>I',v&0xffffffff)

with tempfile.TemporaryDirectory(prefix='pd-beta-aim-mips-') as tmp:
 tmp=Path(tmp);(tmp/'plan.c').write_text(C)
 subprocess.run(['cc','-std=c11','-O2','-I',str(ROOT/'games'),str(tmp/'plan.c'),'-o',str(tmp/'plan')],check=True)
 cases=0
 for v,path in enumerate(sys.argv[1:]):
  rom,ram=beta.fixture(Path(path),v);(tmp/'ram.bin').write_bytes(ram)
  lines=subprocess.check_output([str(tmp/'plan'),str(tmp/'ram.bin')],text=True).splitlines()
  gamebase,cross,cave=[int(w,16) for w in lines[0].split()]
  plan=[[int(w,16) for w in line.split()] for line in lines[1:]]
  virtualcross=0x1f000000+cross-gamebase
  def execute(patched,aim,entry,end):
   u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_BIG_ENDIAN)
   u.mem_map(0,0x800000);u.mem_map(0x1f000000,0x200000)
   u.mem_write(0x1f000000,bytes(ram[gamebase-0x80000000:gamebase-0x80000000+0x200000]))
   for at,old,new in plan:
    va=0x1f000000+at-gamebase
    assert u.mem_read(va,4)==p32(old)
    if patched:u.mem_write(va,p32(new))
   # CU1 allows the actual single-precision instructions in the patch windows.
   u.reg_write(UC_MIPS_REG_CP0_STATUS,u.reg_read(UC_MIPS_REG_CP0_STATUS)|0x20000000)
   player=0x80050000
   u.mem_write(0x50000,p32(0x3f800000)*(0x1c70//4));u.mem_write(0x50120,p32(aim))
   u.reg_write(UC_MIPS_REG_S5,player)
   u.reg_write(UC_MIPS_REG_S1,player+(0x7a4 if entry==0x60 else 0xf48 if entry in [0x84,0x90] else 0))
   u.reg_write(UC_MIPS_REG_S0,0x80060000);u.reg_write(UC_MIPS_REG_S2,0x80060010);u.reg_write(UC_MIPS_REG_V0,0x80060010)
   for reg,n in [(UC_MIPS_REG_F0,1),(UC_MIPS_REG_F8,5),(UC_MIPS_REG_F18,4),(UC_MIPS_REG_F20,3),(UC_MIPS_REG_F24,2),(UC_MIPS_REG_F26,2)]:u.reg_write(reg,struct.unpack('>I',struct.pack('>f',n))[0])
   try:u.emu_start(virtualcross+entry,virtualcross+end,count=100)
   except UcError as e:raise RuntimeError((v,patched,aim,hex(entry),hex(u.reg_read(UC_MIPS_REG_PC)),str(e)))
   assert u.reg_read(UC_MIPS_REG_PC) in [virtualcross+end-4,virtualcross+end],(v,patched,aim,hex(entry),hex(u.reg_read(UC_MIPS_REG_PC)),hex(u.reg_read(UC_MIPS_REG_CP0_STATUS)))
   return bytes(u.mem_read(0x50000,0x1c70))
  for entry,end,targets in [(0x1c,0x24,[0x1668,0x166c]),(0x50,0x58,[0xcd4]),(0x60,0x70,[0xcd8]),(0x84,0x8c,[0x1478]),(0x90,0x98,[0x147c])]:
   for aim in [0,1]:
    original=execute(False,aim,entry,end);patched=execute(True,aim,entry,end)
    if not aim:assert original==patched,(v,entry,'native behavior changed')
    else:
     expected=bytearray(original)
     for at in targets:expected[at:at+4]=p32(0x3f800000)
     assert bytes(expected)==patched,(v,entry,'aim overwrite not suppressed')
    cases+=1
 print(f'PASS {cases} actual MIPS cursor-aim cases: both beta builds, every patched store path, aiming off preserves original writes, aiming on preserves injected crosshair/gun positions.')
