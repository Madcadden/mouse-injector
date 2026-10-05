#ifndef PERFECTDARK_COMPAT_H
#define PERFECTDARK_COMPAT_H

#include <string.h>

typedef int (*PD_COMPAT_READER)(void *, unsigned int, unsigned int *);
typedef struct PD_SIGNATURE
{
	const unsigned int *words;
	const unsigned int *masks;
	unsigned int count, target, retail;
} PD_SIGNATURE;
#include "perfectdark.signatures.h"

typedef struct PD_COMPAT_PROFILE
{
	unsigned int matches[PDS_COUNT];
	unsigned int original[PDS_COUNT];
	unsigned int camera, players, menu, pause, stage, intro, mppause;
	int camera_beta, mp_beta;
	int valid;
} PD_COMPAT_PROFILE;

static int PD_CompatMatch(PD_COMPAT_READER read, void *context, unsigned int address, const PD_SIGNATURE *signature)
{
	unsigned int index, word;
	if(address > 0x80800000U - signature->count * 4)
		return 0;
	for(index = 0; index < signature->count; index++)
		if(!read(context, address + index * 4, &word) ||
			(word & signature->masks[index]) != (signature->words[index] & signature->masks[index]))
			return 0;
	return 1;
}

static unsigned int PD_CompatAddress(PD_COMPAT_READER read, void *context, unsigned int address, unsigned int high, unsigned int low)
{
	unsigned int a, b, result;
	if(!address || !read(context, address + high, &a) || !read(context, address + low, &b))
		return 0;
	result = ((a & 0xFFFFU) << 16) + (int)(short)(b & 0xFFFFU);
	return result >= 0x80001000U && result < 0x807FF000U && !(result & 3) ? result : 0;
}

/* One bounded pass; duplicates invalidate that feature even at the retail address.
 * The callback must reject unmapped memory pages. No writes occur during discovery.
 * The caller retries while boot is loading, then caches until ROM close. */
static int PD_CompatResolve(PD_COMPAT_READER read, void *context, PD_COMPAT_PROFILE *profile)
{
	unsigned int address, index, word;
	unsigned char counts[PDS_COUNT] = {0};
	memset(profile, 0, sizeof(*profile));
	for(address = 0x80001000U; address < 0x80800000U; address += 4)
	{
		if(!read(context, address, &word))
		{
			address = (address & ~0xFFFU) + 0xFFCU;
			continue;
		}
		for(index = 0; index < PDS_COUNT; index++)
		{
			const PD_SIGNATURE *s = &pd_signatures[index];
			int alternate = 0;
			int match = counts[index] < 2 && (word & s->masks[0]) == (s->words[0] & s->masks[0]) &&
				PD_CompatMatch(read, context, address, s);
			if(!match && counts[index] < 2 && (index == PDS_CAMERA || index == PDS_MP))
			{
				const PD_SIGNATURE *beta = index == PDS_CAMERA ? &pd_beta_camera_signature : &pd_beta_mp_signature;
				match = (word & beta->masks[0]) == (beta->words[0] & beta->masks[0]) &&
					PD_CompatMatch(read, context, address, beta);
				alternate = match;
			}
			if(!match && counts[index] < 2 && (index == PDS_SPYUP || index == PDS_SPYDOWN))
			{
				const PD_SIGNATURE *beta = index == PDS_SPYUP ? &pd_beta_spyup_signature : &pd_beta_spydown_signature;
				const PD_SIGNATURE *pal = index == PDS_SPYUP ? &pd_pal_spyup_signature : &pd_pal_spydown_signature;
				match = (word == beta->words[0] && PD_CompatMatch(read, context, address, beta)) ||
					(word == pal->words[0] && PD_CompatMatch(read, context, address, pal));
			}
			if(match)
			{
				counts[index]++;
				profile->matches[index] = counts[index] == 1 ? address : 0;
				if(index == PDS_CAMERA) profile->camera_beta = alternate;
				if(index == PDS_MP) profile->mp_beta = alternate;
			}
		}
	}
	for(index = 0; index < PDS_COUNT; index++)
		if(profile->matches[index])
			read(context, profile->matches[index] + pd_signatures[index].target * 4, &profile->original[index]);
	profile->camera = PD_CompatAddress(read, context, profile->matches[PDS_CAMERA], profile->camera_beta ? 8 : 0, profile->camera_beta ? 16 : 4);
	profile->players = profile->camera ? profile->camera - 0x248 : 0;
	profile->menu = PD_CompatAddress(read, context, profile->matches[PDS_MENU], 8, 0x18);
	profile->pause = PD_CompatAddress(read, context, profile->matches[PDS_PAUSE], 0, 8);
	profile->stage = PD_CompatAddress(read, context, profile->matches[PDS_TITLE], 0x1C, 0x20);
	profile->intro = PD_CompatAddress(read, context, profile->matches[PDS_TITLE], 0x14, 0x18);
	if(profile->mp_beta && profile->matches[PDS_MP])
	{
		unsigned int hi, lo, hi2, lo2;
		/* Adjacent setter and getter must name the same byte, exactly. */
		unsigned int at = profile->matches[PDS_MP];
		if(read(context, at, &hi) && read(context, at + 12, &lo) &&
			read(context, at + 16, &hi2) && read(context, at + 20, &lo2))
		{
			unsigned int a = ((hi & 0xFFFFU) << 16) + (int)(short)lo;
			unsigned int b = ((hi2 & 0xFFFFU) << 16) + (int)(short)lo2;
			if(a == b && a >= 0x80001000U && a < 0x807FF000U && (a & 3) == 2)
				profile->mppause = a;
		}
	}
	else
	{
		profile->mppause = PD_CompatAddress(read, context, profile->matches[PDS_MP], 0, 4);
		if(profile->mppause) profile->mppause += 0x1E;
	}
	profile->valid = profile->camera && profile->players && profile->menu && profile->pause && profile->stage && profile->intro && profile->mppause;
	if(profile->valid)
	{
		const unsigned int globals[] = {profile->camera, profile->players, profile->players + 12,
			profile->menu, profile->menu + 12, profile->pause, profile->stage, profile->intro, profile->mppause & ~3U};
		for(index = 0; index < sizeof(globals) / sizeof(globals[0]); index++)
			if(!read(context, globals[index], &word))
				profile->valid = 0;
	}
	return profile->valid;
}

static unsigned int PD_CompatTarget(const PD_COMPAT_PROFILE *profile, unsigned int index)
{
	return profile->matches[index] ? profile->matches[index] + pd_signatures[index].target * 4 : 0;
}

/* Existing trampolines use original virtual game/lib calls and g_Vars operands.
 * Only permit them at the canonical cached physical addresses with the original
 * data globals and untouched debug-text cave. A common physical move alone does
 * not establish the virtual mapping needed by the fixed J/JAL instructions.
 * Recompiled/reordered code still gets separately resolved FOV/settings, but no
 * guessed trampolines. */
static int PD_CompatLegacyHooks(PD_COMPAT_READER read, void *context, const PD_COMPAT_PROFILE *profile, unsigned int *delta)
{
	const unsigned int indices[] = {PDS_CROSSHAIR, PDS_RELOAD, PDS_RELOADFUNC, PDS_CAVE};
	static const unsigned int libwords[] = {0x3C038006U, 0x8C63EE60U, 0xAFA40000U, 0xAFA50004U, 0x8C790200U, 0x00047600U, 0x000E7E03U, 0x30B8FFFFU, 0x03002825U, 0x0721000FU, 0x01E02025U, 0x3C088006U};
	unsigned int i, shift, word;
	for(i = 0; i < sizeof(libwords) / sizeof(libwords[0]); i++)
		if(!read(context, 0x80015020U + i * 4, &word) || word != libwords[i])
			return 0;
	if(!profile->valid || profile->camera != 0x8009A26CU || profile->players != 0x8009A024U ||
		profile->menu != 0x80070750U || profile->pause != 0x80084014U ||
		profile->stage != 0x800624E4U || profile->mppause != 0x800ACBA6U)
		return 0;
	shift = profile->matches[PDS_CAVE] - pd_signatures[PDS_CAVE].retail;
	if(shift != 0)
		return 0;
	for(i = 0; i < sizeof(indices) / sizeof(indices[0]); i++)
	{
		unsigned int index = indices[i];
		if(!profile->matches[index] || profile->matches[index] - pd_signatures[index].retail != shift ||
			!PD_CompatMatch(read, context, profile->matches[index], &pd_signatures[index]))
			return 0;
	}
	*delta = shift;
	return 1;
}
#endif
