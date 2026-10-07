#ifndef PERFECTDARK_BETA_RELOAD_H
#define PERFECTDARK_BETA_RELOAD_H

#include "perfectdark.compat.h"

/* Exact NTSC 6.4 / PAL 28.7 beta lv-render and called-function windows, derived
 * from the supplied images. Physical placement comes from the resolved title
 * function; game JAL operands establish the linked 0x7f000000 virtual base.
 * The lib is linked at 0x70001050 and is inspected through its 0x80001050 alias.
 * No retail trampoline address or unverified structure relocation is reused.
 * Cave provenance: the dlights osSyncPrintf strings are unreferenced in both
 * beta images; constants.h removes osSyncPrintf for every build, including DEBUG.
 * Prepare this plan BEFORE another owned patch modifies the shared text cave.
 * Only cave+0x58..0x64 is owned here; +0..0x54 remains available for crosshair.
 */
enum PD_BETA_BUILD { PD_BETA_NONE, PD_BETA_NTSC, PD_BETA_PAL };
#define PD_BETA_RELOAD_WORDS 27

typedef struct PD_BETA_RELOAD_WORD {
    unsigned int address, original, replacement;
} PD_BETA_RELOAD_WORD;
typedef struct PD_BETA_RELOAD_PLAN {
    enum PD_BETA_BUILD build;
    unsigned int gamebase, virtualbase, entry, continuation, cave;
    unsigned int interaction, reload, joy, count;
    PD_BETA_RELOAD_WORD words[PD_BETA_RELOAD_WORDS];
} PD_BETA_RELOAD_PLAN;
typedef void (*PD_BETA_WRITER)(void *, unsigned int, unsigned int);

static const unsigned int pd_beta_dc_block[28] = {
    0x0FC0C4BAU, 0xAFAD0010U, 0x8E500284U, 0x8E0C00D0U, 0x5180000CU, 0x8E020480U,
    0x0FC1883EU, 0x00002025U, 0x10400012U, 0x00000000U, 0x0FC27FE5U, 0x00002025U,
    0x0FC27FE5U, 0x24040001U, 0x1000000CU, 0x00000000U, 0x8E020480U, 0x10400009U,
    0x00000000U, 0x804A0037U, 0x11400006U, 0x00000000U, 0x9058006AU, 0x13000003U,
    0x00000000U, 0x0FC1883EU, 0x24040001U, 0x0FC18D1AU
};

static const unsigned int pd_beta_dc_reloadfunc[28] = {
    0x3C0E800AU, 0x8DCEE944U, 0x27BDFFE0U, 0xAFBF0014U, 0xAFA40020U, 0x0FC27DD7U,
    0xAFAE001CU, 0x00402025U, 0x0FC29DEDU, 0x00002825U, 0x1040000FU, 0x8FB80020U,
    0x0018C900U, 0x0338C823U, 0x0019C880U, 0x0338C821U, 0x8FAF001CU, 0x0019C8C0U,
    0x0338C821U, 0x0019C880U, 0x01F91021U, 0x8C48065CU, 0x24090009U, 0x55000003U,
    0x8FBF0014U, 0xAC49065CU, 0x8FBF0014U, 0x27BD0020U
};

static const unsigned int pd_beta_dc_interactionfunc[16] = {
    0x27BDFFE0U, 0xAFBF0014U, 0x0FC187D0U, 0xAFA00018U, 0x8FA50018U, 0x10400016U,
    0xAFA2001CU, 0x904E0000U, 0x25CFFFFFU, 0x2DE10008U, 0x1020000DU, 0x000F7880U,
    0x3C017F1AU, 0x002F0821U, 0x8C2F4218U, 0x01E00008U
};

static const unsigned int pd_beta_dc_joyfunc[24] = {
    0x3C038006U, 0x8C631250U, 0xAFA40000U, 0xAFA50004U, 0x8C790200U, 0x00047600U,
    0x000E7E03U, 0x30B8FFFFU, 0x03002825U, 0x0721000FU, 0x01E02025U, 0x3C088006U,
    0x9108129CU, 0x3C0C8006U, 0x258C127CU, 0x01E84807U, 0x312A0001U, 0x15400007U,
    0x000F5880U, 0x016C1821U, 0x8C6D0000U, 0x00001025U, 0x25AE0001U, 0x03E00008U
};

static const unsigned int pd_beta_eur_block[28] = {
    0x0FC0C6A7U, 0xAFAC0010U, 0x8E500284U, 0x8E0E00D0U, 0x51C0000CU, 0x8E020480U,
    0x0FC18C09U, 0x00002025U, 0x10400012U, 0x00000000U, 0x0FC28927U, 0x00002025U,
    0x0FC28927U, 0x03C02025U, 0x1000000CU, 0x00000000U, 0x8E020480U, 0x10400009U,
    0x00000000U, 0x804F0037U, 0x11E00006U, 0x00000000U, 0x904A006AU, 0x11400003U,
    0x00000000U, 0x0FC18C09U, 0x03C02025U, 0x0FC190F5U
};

static const unsigned int pd_beta_eur_reloadfunc[28] = {
    0x3C0E800AU, 0x8DCEE754U, 0x27BDFFE0U, 0xAFBF0014U, 0xAFA40020U, 0x0FC2870BU,
    0xAFAE001CU, 0x00402025U, 0x0FC2A76CU, 0x00002825U, 0x1040000FU, 0x8FB80020U,
    0x0018C900U, 0x0338C823U, 0x0019C880U, 0x0338C821U, 0x8FAF001CU, 0x0019C8C0U,
    0x0338C821U, 0x0019C880U, 0x01F91021U, 0x8C48065CU, 0x24090009U, 0x55000003U,
    0x8FBF0014U, 0xAC49065CU, 0x8FBF0014U, 0x27BD0020U
};

static const unsigned int pd_beta_eur_interactionfunc[16] = {
    0x27BDFFE0U, 0xAFBF0014U, 0x0FC18B9BU, 0xAFA00018U, 0x8FA50018U, 0x10400016U,
    0xAFA2001CU, 0x904E0000U, 0x25CFFFFFU, 0x2DE10008U, 0x1020000DU, 0x000F7880U,
    0x3C017F1BU, 0x002F0821U, 0x8C2FBBE8U, 0x01E00008U
};

static const unsigned int pd_beta_eur_joyfunc[24] = {
    0x3C038006U, 0x8C6304F0U, 0xAFA40000U, 0xAFA50004U, 0x8C790200U, 0x00047600U,
    0x000E7E03U, 0x30B8FFFFU, 0x03002825U, 0x0721000FU, 0x01E02025U, 0x3C088006U,
    0x9108053CU, 0x3C0C8006U, 0x258C051CU, 0x01E84807U, 0x312A0001U, 0x15400007U,
    0x000F5880U, 0x016C1821U, 0x8C6D0000U, 0x00001025U, 0x25AE0001U, 0x03E00008U
};

static const unsigned int pd_beta_dc_gvarsfunc[8] = {
    0x3C12800AU, 0x35CE0006U, 0x2652E6C0U, 0xAC8E0000U, 0xAC800004U, 0x8E4204B4U, 0x2401005AU, 0x24930008U
};

static const unsigned int pd_beta_eur_gvarsfunc[8] = {
    0x3C12800AU, 0x2652E4D0U, 0x8E4204B4U, 0x2401005AU, 0x10410006U, 0x2401004EU, 0x5441005FU, 0x8E4204B4U
};

typedef struct PD_BETA_RELOAD_LAYOUT {
    enum PD_BETA_BUILD build;
    unsigned int block, reload, interaction, joy, cave, gvars;
    const unsigned int *blockwords, *reloadwords, *interactionwords, *joywords, *gvarswords;
} PD_BETA_RELOAD_LAYOUT;
static const PD_BETA_RELOAD_LAYOUT pd_beta_reload_layouts[2] = {
    {PD_BETA_NTSC, 0x00164F14U, 0x0009FF94U, 0x000620F8U, 0x80015E40U, 0x001A1628U, 0x0016407CU,
        pd_beta_dc_block, pd_beta_dc_reloadfunc, pd_beta_dc_interactionfunc, pd_beta_dc_joyfunc, pd_beta_dc_gvarsfunc},
    {PD_BETA_PAL, 0x0016BCF8U, 0x000A249CU, 0x00063024U, 0x800159A8U, 0x001A9638U, 0x0016AE3CU,
        pd_beta_eur_block, pd_beta_eur_reloadfunc, pd_beta_eur_interactionfunc, pd_beta_eur_joyfunc, pd_beta_eur_gvarsfunc}
};

static int PD_BetaReloadMatch(PD_COMPAT_READER read, void *context,
    unsigned int address, const unsigned int *words, unsigned int count)
{
    unsigned int i, word;
    if(address < 0x80001000U || address > 0x80800000U - count * 4U) return 0;
    for(i = 0; i < count; i++)
        if(!read(context, address + i * 4U, &word) || word != words[i]) return 0;
    return 1;
}

static int PD_BetaReloadPrepare(PD_COMPAT_READER read, void *context,
    const PD_COMPAT_PROFILE *profile, PD_BETA_RELOAD_PLAN *plan)
{
    unsigned int i, j, base, virtualbase, currentplayer, code[23], cavecode[4];
    const PD_BETA_RELOAD_LAYOUT *layout = 0;
    memset(plan, 0, sizeof(*plan));
    if(!profile->valid || !profile->matches[PDS_TITLE] || !profile->matches[PDS_CAVE]) return 0;
    base = profile->matches[PDS_TITLE] - 0x90U;
    if(base < 0x80001000U || base > 0x80600000U) return 0;
    for(i = 0; i < 2; i++) {
        const PD_BETA_RELOAD_LAYOUT *candidate = &pd_beta_reload_layouts[i];
        if(profile->matches[PDS_CAVE] != base + candidate->cave ||
            !PD_BetaReloadMatch(read, context, base + candidate->block, candidate->blockwords, 28) ||
            !PD_BetaReloadMatch(read, context, base + candidate->reload, candidate->reloadwords, 28) ||
            !PD_BetaReloadMatch(read, context, base + candidate->interaction, candidate->interactionwords, 16) ||
            !PD_BetaReloadMatch(read, context, candidate->joy, candidate->joywords, 24) ||
            !PD_BetaReloadMatch(read, context, base + candidate->gvars, candidate->gvarswords, 8) ||
            !PD_CompatMatch(read, context, profile->matches[PDS_CAVE], &pd_signatures[PDS_CAVE])) continue;
        if(layout) return 0;
        layout = candidate;
    }
    if(!layout) return 0;
    currentplayer = ((layout->reloadwords[0] & 0xFFFFU) << 16) +
        (int)(short)(layout->reloadwords[1] & 0xFFFFU);
    if(currentplayer != profile->players + 0x220U) return 0;
    /* Derive the linked game base independently from both original JALs. */
    virtualbase = (0x70000000U | ((layout->blockwords[10] & 0x03FFFFFFU) << 2)) - layout->reload;
    if(virtualbase != 0x7F000000U ||
        (0x70000000U | ((layout->blockwords[6] & 0x03FFFFFFU) << 2)) - layout->interaction != virtualbase)
        return 0;
    plan->build = layout->build;
    plan->gamebase = base;
    plan->virtualbase = virtualbase;
    plan->entry = base + layout->block + 16U;
    plan->continuation = virtualbase + layout->block + 108U;
    plan->cave = base + layout->cave + 0x58U;
    plan->interaction = virtualbase + layout->interaction;
    plan->reload = virtualbase + layout->reload;
    plan->joy = layout->joy - 0x10000000U;
    /* Keep B's native interaction and eyespy door checks. Only reserved bit0x40
     * reaches either hand's native reload function. Active eyespy cannot reload.
     * s2 is g_Vars in BOTH validated beta handlers; +0x28c is currentplayernum.
     */
    code[0] = 0x10000003U | (layout->blockwords[4] & 0x03E00000U); /* beq Bflag,zero,check-eyespy */
    code[1] = 0;
    code[2] = layout->blockwords[6];
    code[3] = 0x00002025U;
    code[4] = 0x8E020480U;
    code[5] = 0x5440000BU;
    code[6] = layout->blockwords[19]; /* lb native eyespy-active temporary */
    code[7] = 0x8E44028CU; /* lw a0,0x28c(s2) */
    code[8] = 0;
    code[9] = 0x0C000000U | ((plan->joy >> 2) & 0x03FFFFFFU);
    code[10] = 0x34050040U;
    code[11] = 0x1040000BU;
    code[12] = 0;
    code[13] = layout->blockwords[10];
    code[14] = 0x00002025U;
    code[15] = 0x08000000U | (((virtualbase + layout->cave + 0x58U) >> 2) & 0x03FFFFFFU);
    code[16] = 0;
    code[17] = (layout->blockwords[20] & 0xFFFF0000U) | 0xFFF5U; /* inactive eyespy -> reload check */
    code[18] = layout->blockwords[22];
    code[19] = layout->blockwords[23];
    code[20] = 0;
    code[21] = layout->blockwords[25];
    code[22] = 0x34040001U;
    cavecode[0] = layout->blockwords[10];
    cavecode[1] = 0x34040001U;
    cavecode[2] = 0x08000000U | ((plan->continuation >> 2) & 0x03FFFFFFU);
    cavecode[3] = 0;
    for(i = 0; i < 23; i++) {
        plan->words[i].address = plan->entry + i * 4U;
        plan->words[i].original = layout->blockwords[4 + i];
        plan->words[i].replacement = code[i];
    }
    for(j = 0; j < 4; j++, i++) {
        plan->words[i].address = plan->cave + j * 4U;
        plan->words[i].original = pd_cave_words[0x58U / 4U + j];
        plan->words[i].replacement = cavecode[j];
    }
    plan->count = PD_BETA_RELOAD_WORDS;
    return 1;
}

static int PD_BetaReloadApply(PD_COMPAT_READER read, PD_BETA_WRITER write,
    void *context, const PD_BETA_RELOAD_PLAN *plan)
{
    unsigned int i, word;
    if(plan->build == PD_BETA_NONE || plan->count != PD_BETA_RELOAD_WORDS) return 0;
    /* Refuse to overwrite a foreign patch. Full preflight precedes all writes;
     * the cave is installed first and entry branch last. Reapplication is safe.
     */
    for(i = 0; i < plan->count; i++)
        if(!read(context, plan->words[i].address, &word) ||
            (word != plan->words[i].original && word != plan->words[i].replacement)) return 0;
    for(i = plan->count; i-- > 0;)
        write(context, plan->words[i].address, plan->words[i].replacement);
    return 1;
}
#endif
