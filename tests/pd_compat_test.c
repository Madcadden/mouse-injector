#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../games/perfectdark.compat.h"

static unsigned char ram[0x800000], original[0x800000];
static int pagehole;
static int readram(void *unused, unsigned int address, unsigned int *value)
{
	unsigned int a = address - 0x80000000U;
	(void)unused;
	if((address & 3) || a > sizeof(ram) - 4 || (pagehole && (a >> 12) == 0x9A)) return 0;
	*value = ((unsigned int)ram[a] << 24) | ((unsigned int)ram[a + 1] << 16) | ((unsigned int)ram[a + 2] << 8) | ram[a + 3];
	return 1;
}
static void put(unsigned int a, unsigned int v)
{
	a -= 0x80000000U; ram[a] = v >> 24; ram[a + 1] = v >> 16; ram[a + 2] = v >> 8; ram[a + 3] = v;
}
static void load(const char *path, unsigned int offset)
{
	FILE *f = fopen(path, "rb"); size_t n;
	assert(f); n = fread(ram + offset, 1, sizeof(ram) - offset, f); assert(n); assert(feof(f)); fclose(f);
}
static void reset(void) { memcpy(ram, original, sizeof(ram)); pagehole = 0; }
static void readdress(unsigned int base, unsigned int hi, unsigned int lo, unsigned int target)
{
	unsigned int a,b; assert(readram(0,base+hi,&a)); assert(readram(0,base+lo,&b));
	put(base+hi,(a&0xFFFF0000U)|((target+0x8000U)>>16)); put(base+lo,(b&0xFFFF0000U)|(target&0xFFFFU));
}
int main(int argc, char **argv)
{
	PD_COMPAT_PROFILE p; unsigned int delta, i;
	assert(argc == 4);
	load(argv[1], 0x1050); load(argv[2], 0x59FE0); load(argv[3], 0x220000); memcpy(original,ram,sizeof(ram));
	assert(PD_CompatResolve(readram, 0, &p));
	for(i=0;i<PDS_COUNT;i++) assert(p.matches[i] == pd_signatures[i].retail);
	assert(p.camera==0x8009A26C && p.players==0x8009A024 && p.menu==0x80070750 && p.pause==0x80084014 && p.stage==0x800624E4 && p.intro==0x800624C4 && p.mppause==0x800ACBA6);
	assert(PD_CompatLegacyHooks(readram,0,&p,&delta) && delta==0);
	puts("PASS retail: all signatures unique, all globals exact, hooks fully validated");

	/* An independent FOV function move does not require a shared code delta. */
	memcpy(ram+0x600000,ram+pd_signatures[PDS_FOV].retail-0x80000000U,pd_signatures[PDS_FOV].count*4);
	memset(ram+pd_signatures[PDS_FOV].retail-0x80000000U,0,pd_signatures[PDS_FOV].count*4);
	assert(PD_CompatResolve(readram,0,&p)); assert(PD_CompatTarget(&p,PDS_FOV)==0x8060000C);
	puts("PASS independently moved FOV function");

	reset(); memcpy(ram+0x600000,ram+pd_signatures[PDS_FOV].retail-0x80000000U,pd_signatures[PDS_FOV].count*4);
	assert(PD_CompatResolve(readram,0,&p)); assert(PD_CompatTarget(&p,PDS_FOV)==0);
	puts("PASS duplicate FOV rejects override even with original address present");

	reset(); ram[pd_signatures[PDS_FOV].retail-0x80000000U+15]^=1;
	assert(PD_CompatResolve(readram,0,&p)); assert(PD_CompatTarget(&p,PDS_FOV)==0);
	puts("PASS changed FOV instruction rejected");

	reset(); memmove(ram+0x230000,ram+0x220000,0x1B99E0); memset(ram+0x220000,0,0x10000);
	assert(PD_CompatResolve(readram,0,&p)); assert(PD_CompatTarget(&p,PDS_FOV)==0x802FAA5C);
	assert(!PD_CompatLegacyHooks(readram,0,&p,&delta));
	puts("PASS whole game move resolves FOV but skips unverified virtual trampolines");

	reset(); readdress(pd_signatures[PDS_CAMERA].retail,0,4,0x800CA26C);
	readdress(pd_signatures[PDS_MENU].retail,8,0x18,0x800A0750);
	readdress(pd_signatures[PDS_PAUSE].retail,0,8,0x800B4014);
	readdress(pd_signatures[PDS_TITLE].retail,0x1C,0x20,0x800924E4);
	readdress(pd_signatures[PDS_TITLE].retail,0x14,0x18,0x800924C4);
	readdress(pd_signatures[PDS_MP].retail,0,4,0x800DCB88);
	assert(PD_CompatResolve(readram,0,&p)); assert(p.camera==0x800CA26C && p.menu==0x800A0750 && p.pause==0x800B4014 && p.stage==0x800924E4 && p.intro==0x800924C4 && p.mppause==0x800DCBA6);
	assert(!PD_CompatLegacyHooks(readram,0,&p,&delta));
	puts("PASS relocated globals resolved, absolute legacy trampolines disabled");

	reset(); assert(PD_CompatResolve(readram,0,&p)); ram[0x3C7988]^=1;
	assert(!PD_CompatLegacyHooks(readram,0,&p,&delta));
	puts("PASS cave ownership rechecked immediately before writes");

	reset(); pagehole=1; assert(!PD_CompatResolve(readram,0,&p));
	puts("PASS unmapped runtime globals reject profile without dereferencing page");

	memset(ram,0,sizeof(ram));pagehole=0;assert(!PD_CompatResolve(readram,0,&p));
	puts("PASS empty RAM rejects all patches");
	return 0;
}
