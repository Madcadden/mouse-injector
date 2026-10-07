/* GPL-2.0-or-later. Production Map Maker camera regression.
 * Build with the DLL's strict C11 + x87 flags, not the GNU-dialect default.
 * No ROMs, saves or Windows input devices are required for the synthetic tests.
 * Optional locally supplied word-swapped RDRAM and big-endian ROM replay uses
 * the production resolver and production paged memory accessors as well.
 */
#ifdef _WIN32
#include <windows.h>
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>
#ifndef GE_TEST_SOURCE
#define GE_TEST_SOURCE "../games/goldeneye.c"
#endif
#include GE_TEST_SOURCE

BUTTONS CONTROLLER[4];
struct PROFILE_STRUCT PROFILE[4];
struct DEVICE_STRUCT DEVICE[4];
const unsigned char **rdramptr, **romptr;
int mousetoggle = 1, emuoverclock, overridefov = 90;
int overrideratiowidth = 16, overrideratioheight = 9;
int geshowcrosshair, bypassviewmodelfovtweak;
static uint32_t ram[0x800000/4], before[0x800000/4];
static const unsigned char *pages[0x100000];
static GE_ADDRESS_PROFILE p;
static unsigned int assertions, cases;
static void require(int value, const char *message) {
    ++assertions;
    if(!value) { fprintf(stderr, "FAIL: %s (case %u)\n", message, cases); exit(1); }
}
static uint32_t word(unsigned int at) { return (uint32_t)EMU_ReadInt(at); }
static void put(unsigned int at, uint32_t value) { EMU_WriteInt(at, (int)value); }
static void setup(void) {
    GE_MAPMAKER_PROFILE *e=&p.mapmaker;
    ++cases;
    memset(ram,0,sizeof(ram)); memset(&p,0,sizeof(p));
    memset(PROFILE,0,sizeof(PROFILE)); memset(DEVICE,0,sizeof(DEVICE));
    memset(CONTROLLER,0,sizeof(CONTROLLER));
    p.menupage=0x80001000U; p.maxpage=30;
    e->page=30; e->yaw=0x8006CB0CU; e->pitch=e->yaw+4;
    e->menu=e->yaw+12; e->preview=e->yaw+32; e->freemode=e->yaw+40;
    e->nextpage=p.menupage+4; e->nextpagealt=p.menupage+8;
    e->pitchmin=0x80002000U; e->pitchmax=0x80002004U;
    e->preview_yaw=0x80002010U; e->preview_pitch=0x80002014U;
    e->preview_pitchmin=0x80002020U; e->preview_pitchmax=0x80002024U;
    put(p.menupage,30); put(e->freemode,1);
    put(e->nextpage,0xFFFFFFFFU); put(e->nextpagealt,0xFFFFFFFFU);
    /* Exact values from the reported stationary camera, not decimal literals. */
    put(e->yaw,0x40E154BAU); put(e->pitch,0xBF0CCCCDU);
    put(e->pitchmin,0xBFC90FDAU); put(e->pitchmax,0x3FC90FDAU);
    put(e->preview_yaw,0x40E154BAU); put(e->preview_pitch,0xBF0CCCCDU);
    put(e->preview_pitchmin,0xBFC90FDAU); put(e->preview_pitchmax,0x3FC90FDAU);
    PROFILE[0].SETTINGS[CONFIG]=WASD;
    PROFILE[0].SETTINGS[SENSITIVITY]=20;
    DEVICE[0].XPOS=3; DEVICE[0].YPOS=-5;
    mousetoggle=1; ge_ff_yaw_writes=ge_ff_pitch_writes=0;
}
static void capture(void) { memcpy(before,ram,sizeof(ram)); }
static void check_writes(unsigned int yaw, unsigned int pitch, int changes) {
    unsigned int found=0;
    for(unsigned int i=0;i<sizeof(ram)/sizeof(ram[0]);++i) {
        if(ram[i]!=before[i]) {
            require(changes && (i==(yaw&0x7FFFFFU)/4 || i==(pitch&0x7FFFFFU)/4),
                "no writes outside the selected camera angles");
            ++found;
        }
    }
    require(found==(unsigned int)changes,"expected number of changed camera words");
}
static void look(float sensitivity) { GE_MapMakerLook(&p,sensitivity); }
static void no_write(void) { capture();look(5.0f);check_writes(0,0,0); }
static void exercise(void) {
    setup();capture();look(5.0f);
#ifdef EXPECT_F2_REJECTION
    require(FLT_EVAL_METHOD==2,"baseline must exercise x87 excess precision");
    check_writes(0,0,0);
    require(ge_ff_yaw_writes==0 && ge_ff_pitch_writes==0,"F2 rejection reproduced");
    require(strcmp(ge_ff_result,"rejected by camera pitch/constant validation")==0,"same rejection as user log");
    printf("F2_REPRODUCED: valid camera rejected; no angle writes. FLT_EVAL_METHOD=%d\n",FLT_EVAL_METHOD);
#else
    check_writes(p.mapmaker.yaw,p.mapmaker.pitch,2);
    require(ge_ff_yaw_writes==1 && ge_ff_pitch_writes==1,"both write paths run");
    require(isfinite(EMU_ReadFloat(p.mapmaker.yaw)) && isfinite(EMU_ReadFloat(p.mapmaker.pitch)),"finite output");
    require(EMU_ReadFloat(p.mapmaker.yaw)<1.0f && EMU_ReadFloat(p.mapmaker.pitch)>-0.55f,"expected look direction and yaw wrap");
    setup();put(p.mapmaker.preview,1);put(p.mapmaker.freemode,0);capture();look(5.0f);
    check_writes(p.mapmaker.preview_yaw,p.mapmaker.preview_pitch,2);
    setup();PROFILE[0].SETTINGS[INVERTPITCH]=1;capture();look(5.0f);
    check_writes(p.mapmaker.yaw,p.mapmaker.pitch,2);
    require(EMU_ReadFloat(p.mapmaker.pitch)<-0.55f,"inverted pitch direction");
    for(int direction=-1;direction<=1;direction+=2) {
        setup();DEVICE[0].XPOS=0;DEVICE[0].YPOS=direction*1000000;capture();look(5.0f);
        check_writes(p.mapmaker.yaw,p.mapmaker.pitch,1);
        require(word(p.mapmaker.pitch)==(direction<0?0x3FC90FDAU:0xBFC90FDAU),"clamp to exact native endpoint");
        setup();DEVICE[0].YPOS=0;DEVICE[0].XPOS=direction*10;capture();look(5.0f);
        check_writes(p.mapmaker.yaw,p.mapmaker.pitch,1);
    }
    /* Every wrong bit remains rejected: no epsilon or disabled validation. */
    for(int preview=0;preview<=1;++preview) for(int side=0;side<2;++side) for(int bit=0;bit<32;++bit) {
        setup();put(p.mapmaker.preview,preview);
        unsigned int at=preview?(side?p.mapmaker.preview_pitchmax:p.mapmaker.preview_pitchmin):
            (side?p.mapmaker.pitchmax:p.mapmaker.pitchmin);
        put(at,word(at)^(1U<<bit));no_write();
    }
    for(int bad=0;bad<16;++bad) {
        setup();
        switch(bad) {
        case 0: mousetoggle=0;break;
        case 1: put(p.menupage,11);break;
        case 2: p.mapmaker.page=0;break;
        case 3: put(p.mapmaker.menu,1);break;
        case 4: put(p.mapmaker.freemode,0);break;
        case 5: put(p.mapmaker.preview,2);break;
        case 6: put(p.mapmaker.nextpage,1);break;
        case 7: put(p.mapmaker.nextpagealt,1);break;
        case 8: DEVICE[0].BUTTONPRIM[START]=1;break;
        case 9: DEVICE[0].XPOS=DEVICE[0].YPOS=0;break;
        case 10: p.mapmaker.pitch=0;break;
        case 11: put(p.mapmaker.yaw,0x7FC00000U);break;
        case 12: put(p.mapmaker.pitch,0xFF800000U);break;
        case 13: put(p.mapmaker.pitch,0x40000000U);break;
        case 14: put(p.mapmaker.pitch,0xC0000000U);break;
        case 15: put(p.mapmaker.preview,1);p.mapmaker.preview_yaw=0;break;
        }
        no_write();
    }
    const float sensitivities[]={0.0f,-1.0f,NAN,INFINITY};
    for(unsigned int i=0;i<sizeof(sensitivities)/sizeof(sensitivities[0]);++i) {
        setup();capture();look(sensitivities[i]);check_writes(0,0,0);
    }
    setup();
    for(unsigned int i=0;i<1000;++i) {
        DEVICE[0].XPOS=(i&1)?17:-13;DEVICE[0].YPOS=(i&2)?-11:7;look(5.0f);
        float yaw=EMU_ReadFloat(p.mapmaker.yaw),pitch=EMU_ReadFloat(p.mapmaker.pitch);
        require(isfinite(yaw)&&yaw>=0&&yaw<=2.0f*PI,"yaw remains finite and wrapped");
        require(isfinite(pitch)&&pitch>=EMU_ReadFloat(p.mapmaker.pitchmin)&&pitch<=EMU_ReadFloat(p.mapmaker.pitchmax),"pitch remains clamped");
    }
    printf("F3_PASS: %u cases; %u assertions; FLT_EVAL_METHOD=%d\n",cases,assertions,FLT_EVAL_METHOD);
#endif
}
static void replay(const char *rompath,const char *rampath) {
    FILE *f=fopen(rompath,"rb");require(f!=NULL,"open local ROM");
    fseek(f,0,SEEK_END);long n=ftell(f);rewind(f);require(n>=0x200000 && !(n&3),"ROM length");
    romptr=calloc((size_t)n/4,sizeof(*romptr));require(romptr!=NULL,"allocate ROM word table");
    for(long i=0;i<n/4;++i) {
        unsigned char b[4];require(fread(b,1,4,f)==4,"read ROM word");
        romptr[i]=(const unsigned char*)(uintptr_t)(((uint32_t)b[0]<<24)|((uint32_t)b[1]<<16)|((uint32_t)b[2]<<8)|b[3]);
    }
    fclose(f);f=fopen(rampath,"rb");require(f!=NULL,"open local RDRAM");
    require(fread(ram,1,sizeof(ram),f)==sizeof(ram),"read RDRAM");fclose(f);
    GE_Quit();require(GE_ResolveAddressProfile(&p),"resolve actual supplied ROM");
    require(p.mapmaker.page==30 && p.mapmaker.menu_rows==16,"revised editor resolved");
    require(word(p.menupage)==30 && word(p.mapmaker.menu)==0 && word(p.mapmaker.freemode)==1,"actual Free Fly state");
    mousetoggle=1;PROFILE[0].SETTINGS[CONFIG]=WASD;PROFILE[0].SETTINGS[INVERTPITCH]=0;
    memset(DEVICE,0,sizeof(DEVICE));DEVICE[0].XPOS=3;DEVICE[0].YPOS=-5;
    capture();look(5.0f);
#ifdef EXPECT_F2_REJECTION
    check_writes(0,0,0);puts("LOCAL_STATE_F2_REJECTED");
#else
    check_writes(p.mapmaker.yaw,p.mapmaker.pitch,2);
    printf("LOCAL_STATE_F3_MOVED: yaw=%.9g pitch=%.9g\n",EMU_ReadFloat(p.mapmaker.yaw),EMU_ReadFloat(p.mapmaker.pitch));
#endif
    free(romptr);romptr=NULL;
}
int main(int argc,char **argv) {
    for(unsigned int i=0;i<0x800;++i) pages[0x80000U+i]=(unsigned char*)ram+4096U*i;
    rdramptr=pages;
    exercise();
    if(argc==3)replay(argv[1],argv[2]);else require(argc==1,"optional ROM and RDRAM pair");
    return 0;
}
