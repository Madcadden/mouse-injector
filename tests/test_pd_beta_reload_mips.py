#!/usr/bin/env python3
"""Execute actual beta reload patch words with stubbed external engine functions.
Usage: python tests/test_pd_beta_reload_mips.py GAME.bin PATCHPLAN.tsv
PATCHPLAN.tsv is emitted by test_pd_beta_reload.c. Requires pip package unicorn.
This verifies branch/call routing, not full emulator gameplay or reload timing.
"""
from pathlib import Path
from itertools import product
import struct,sys
from unicorn import Uc, UC_ARCH_MIPS, UC_MODE_MIPS32, UC_MODE_BIG_ENDIAN, UC_HOOK_CODE
from unicorn.mips_const import *

# Run the actual patch instruction words with the common linked address high
# nibble 7 -> 1. This preserves J/JAL low26 targets and every relative branch,
# while avoiding an N64 TLB implementation in this bounded instruction harness.
def p32(x):return struct.pack('>I',x)
def relocate(a):return a-0x60000000 if 0x70000000<=a<0x80000000 else a

def test(gamefile,planfile):
 raw=Path(planfile).read_text().splitlines();fields=raw[0].split();q={fields[i]:int(fields[i+1],16) for i in range(0,len(fields),2)}
 game=bytearray(Path(gamefile).read_bytes())
 for row in raw[1:]:
  a,old,new=[int(x,16) for x in row.split()];off=a-q['gamebase'];assert game[off:off+4]==p32(old);game[off:off+4]=p32(new)
 total=0
 for player,b,r,eye,interact_result in product(range(4),range(2),range(2),range(4),range(2)):
  u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_BIG_ENDIAN)
  u.mem_map(0,0x100000);u.mem_map(0x10000000,0x100000);u.mem_map(0x1f000000,0x200000)
  u.mem_write(0x1f000000,bytes(game));gvars=0x90000;playerbase=0xa0000;eyespy=0xb0000
  u.mem_write(gvars+0x284,p32(playerbase));u.mem_write(gvars+0x28c,p32(player));u.mem_write(playerbase+0xd0,p32(b));u.mem_write(playerbase+0x480,p32(eyespy if eye else 0))
  if eye:u.mem_write(eyespy+0x34,bytes([0,0,0,1 if eye>=2 else 0]));u.mem_write(eyespy+0x6a,bytes([1 if eye==3 else 0]))
  u.reg_write(UC_MIPS_REG_S2,gvars);u.reg_write(UC_MIPS_REG_SP,0xf0000);u.reg_write(UC_MIPS_REG_FP,1)
  calls=[];targets={relocate(q[k]):k for k in ('interact','reload','joy')}
  for addr in targets:u.mem_write(addr,p32(0x03e00008)+p32(0))
  end=relocate(q['continuation'])
  def hook(uc,addr,size,data):
   if addr==end:uc.emu_stop();return
   if addr not in targets:return
   what=targets[addr];a0=uc.reg_read(UC_MIPS_REG_A0);a1=uc.reg_read(UC_MIPS_REG_A1);calls.append((what,a0))
   if what=='joy':
    assert a0==player and a1==0x40,(what,a0,a1,player)
    uc.reg_write(UC_MIPS_REG_V0,r*0x40)
   elif what=='interact':uc.reg_write(UC_MIPS_REG_V0,interact_result)
  u.hook_add(UC_HOOK_CODE,hook)
  start=relocate(q['virtualbase']+q['entry']-q['gamebase']-8)
  u.emu_start(start,end,count=150)
  assert u.reg_read(UC_MIPS_REG_PC)==end,(q,b,r,eye,calls,hex(u.reg_read(UC_MIPS_REG_PC)))
  actual_inter=[v for k,v in calls if k=='interact'];actual_reload=[v for k,v in calls if k=='reload']
  expected_inter=([0] if b else [])+([1] if eye==3 else [])
  expected_reload=[0,1] if r and eye<2 else []
  assert actual_inter==expected_inter,(q,b,r,eye,calls,expected_inter)
  assert actual_reload==expected_reload,(q,b,r,eye,calls,expected_reload)
  total+=1
 print(f'PASS beta{q["build"]}: {total} actual MIPS paths (four controllers; none/E/R/E+R; eyespy absent/inactive/active/door; interaction success/failure). E never causes reload; R never adds interaction; active eyespy reload blocked.')
 return total
if __name__=='__main__':test(sys.argv[1],sys.argv[2])
