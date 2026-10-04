from pathlib import Path
import struct,json,sys
from unicorn import *
from unicorn.mips_const import *
root=Path(__file__).resolve().parents[1]
b=Path(sys.argv[1]).read_bytes(); base=0x805cb4d0
patch=[tuple(int(x,16) for x in l.split()) for l in Path(sys.argv[2]).read_text().splitlines()]
assert len(patch)==42
p32=lambda v:struct.pack('>I',v&0xffffffff)
def machine(patched=True):
 u=Uc(UC_ARCH_MIPS,UC_MODE_MIPS32|UC_MODE_BIG_ENDIAN);u.mem_map(0,0x800000)
 u.mem_write(0x600000,b[0x34b30:0x200000])
 for off,original,new in patch:
  assert struct.unpack_from('>I',b,off)[0]==original
  if patched:u.mem_write((base+off)&0x7fffff,p32(new))
 u.reg_write(UC_MIPS_REG_SP,0x805f0000);u.reg_write(UC_MIPS_REG_S0,0x80082290)
 u.mem_write(0x82290,p32(0x80200000));u.reg_write(UC_MIPS_REG_RA,0x80001000)
 return u
action=base+0xe57f0; end=base+0xe5ab0; nontank=base+0xe5908; tank=base+0xe5858
helpers=[0x80603388,0x806035d4,0x80603420,0x806034fc]
results=[]
for raw in [0,0x4000,0x40,0x4040]:
 for one in [0,1]:
  for two in [0,1]:
   for tankflag in [0,1]:
    u=machine();calls=[];endpoint=[]
    u.mem_write(0x5f0064,p32(raw));u.mem_write(0x5f01f0,p32(bool(raw)));u.mem_write(0x370b8,p32(tankflag))
    u.mem_write(0x370c0,p32(0x80210000))
    def hook(u,at,size,data):
     if at in [end,nontank,tank]:endpoint.append(at);u.emu_stop()
     elif at in helpers:
      calls.append(at);u.reg_write(UC_MIPS_REG_V0,one if at==helpers[0] else two if at==helpers[2] else 0);u.reg_write(UC_MIPS_REG_PC,u.reg_read(UC_MIPS_REG_RA))
    u.hook_add(UC_HOOK_CODE,hook)
    u.emu_start(action,0x80001000,count=1000)
    if raw&0x40:
     assert not calls and endpoint==[end];assert struct.unpack('>I',u.mem_read(0x2000d0,4))[0]==raw
    elif raw==0:assert not calls and endpoint==[end]
    else:
     expected=[helpers[0],helpers[1]] if one else [helpers[0],helpers[2],helpers[3]] if two else [helpers[0],helpers[2]]
     assert calls==expected,(calls,expected)
     assert endpoint==[end if one or two else tank if tankflag else nontank]
    results.append({'raw':raw,'context1':one,'context2':two,'tank':tankflag,'passed':True})
# Execute actual patched native action getter and downstream dispatcher.
weapon=next(off for off,orig,new in patch if new==0x8c4200d0);logic=next(off for off,orig,new in patch if new==0x8e020000)
orig=lambda off:struct.unpack_from('>I',b,off)[0]
calltarget=lambda word:0x80000000|((word&0x3ffffff)<<2)
interaction=calltarget(orig(logic+12));state=calltarget(orig(logic+28))
for raw in [0,0x4000,0x40,0x4040]:
 u=machine();u.mem_write(0x2000d0,p32(raw));u.reg_write(UC_MIPS_REG_V0,0x80200000)
 u.emu_start(base+weapon,0x80001000,count=50)
 assert u.reg_read(UC_MIPS_REG_T3)==raw&0x40 and u.reg_read(UC_MIPS_REG_T2)==raw&0x4000
 calls=[]
 def hook(u,at,size,data):
  if at in [interaction,state]:
   calls.append((at,u.reg_read(UC_MIPS_REG_A0)));u.reg_write(UC_MIPS_REG_PC,u.reg_read(UC_MIPS_REG_RA))
 u.hook_add(UC_HOOK_CODE,hook);u.emu_start(base+logic,base+logic+44,count=100)
 assert [a for t,a in calls if t==state]==([0,1] if raw&0x40 else [])
 assert len([1 for t,a in calls if t==interaction])==bool(raw&0x4000)
print(json.dumps({'test':'Real-ROM MIPS reload/control dispatch with stubbed callees','cases':len(results)+4,'passed':True}))
