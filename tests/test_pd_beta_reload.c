/* Exact beta patch ownership regression, using user-supplied decompressed data.
 * cc -std=c11 -O2 -Wall -Wextra -Wno-unused-function \
 *   tests/test_pd_beta_reload.c -o /tmp/pd_beta_reload_test
 * /tmp/pd_beta_reload_test LIB.bin GAME.bin PATCHPLAN.tsv
 * Tests installation, idempotence, corruptions, and independent physical move.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../games/perfectdark.beta_reload.h"
static unsigned char ram[0x800000],backup[0x800000];static unsigned writes;
static int rd(void*c,unsigned a,unsigned*v){unsigned o=a-0x80000000U;(void)c;if((a&3)||o>sizeof(ram)-4)return 0;*v=(unsigned)ram[o]<<24|(unsigned)ram[o+1]<<16|(unsigned)ram[o+2]<<8|ram[o+3];return 1;}
static void wr(void*c,unsigned a,unsigned v){unsigned o=a-0x80000000U;(void)c;assert(o<=sizeof(ram)-4);ram[o]=v>>24;ram[o+1]=v>>16;ram[o+2]=v>>8;ram[o+3]=v;writes++;}
static void load(char*path,unsigned o){FILE*f=fopen(path,"rb");assert(f);size_t n=fread(ram+o,1,sizeof(ram)-o,f);assert(n);assert(feof(f));fclose(f);}
int main(int ac,char**av){assert(ac==4);load(av[1],0x1050);load(av[2],0x220000);memcpy(backup,ram,sizeof(ram));
PD_COMPAT_PROFILE p;PD_BETA_RELOAD_PLAN q,again;assert(PD_CompatResolve(rd,0,&p));assert(PD_BetaReloadPrepare(rd,0,&p,&q));assert(q.count==27);FILE*f=fopen(av[3],"w");assert(f);fprintf(f,"build %d gamebase %08x virtualbase %08x entry %08x continuation %08x cave %08x interact %08x reload %08x joy %08x\n",q.build,q.gamebase,q.virtualbase,q.entry,q.continuation,q.cave,q.interaction,q.reload,q.joy);for(unsigned i=0;i<q.count;i++)fprintf(f,"%08x %08x %08x\n",q.words[i].address,q.words[i].original,q.words[i].replacement);fclose(f);
assert(PD_BetaReloadApply(rd,wr,0,&q));unsigned w=0;for(unsigned i=0;i<q.count;i++){assert(rd(0,q.words[i].address,&w));assert(w==q.words[i].replacement);}assert(PD_BetaReloadApply(rd,wr,0,&q));
memcpy(ram,backup,sizeof(ram));ram[q.entry-0x80000000U]^=1;writes=0;assert(!PD_BetaReloadApply(rd,wr,0,&q)&&writes==0);
for(unsigned i=0;i<q.count;i++){memcpy(ram,backup,sizeof(ram));ram[q.words[i].address-0x80000000U]^=1;assert(!PD_BetaReloadPrepare(rd,0,&p,&again));}
memcpy(ram,backup,sizeof(ram));ram[q.joy-0x70000000U]^=1;assert(!PD_BetaReloadPrepare(rd,0,&p,&again));
memcpy(ram,backup,sizeof(ram));ram[q.gamebase-0x80000000U+(q.reload-q.virtualbase)]^=1;assert(!PD_BetaReloadPrepare(rd,0,&p,&again));
memcpy(ram,backup,sizeof(ram));memmove(ram+0x230000,ram+0x220000,0x1c0000);memset(ram+0x220000,0,0x10000);assert(PD_CompatResolve(rd,0,&p));assert(PD_BetaReloadPrepare(rd,0,&p,&again));assert(again.gamebase==q.gamebase+0x10000&&again.virtualbase==q.virtualbase&&again.reload==q.reload&&again.joy==q.joy);
printf("PASS beta%d: full original ROM guards, all27 installed words, idempotence, every owned-word corruption rejects, dependency corruption rejects, physical relocation with linked virtual addresses preserved\n",q.build);return 0;}
