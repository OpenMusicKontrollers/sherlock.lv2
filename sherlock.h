/*
 * Copyright (c) 2015-2020 Hanspeter Portner (dev@open-music-kontrollers.ch)
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the Artistic License 2.0 as published by
 * The Perl Foundation.
 *
 * This source is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * Artistic License 2.0 for more details.
 *
 * You should have received a copy of the Artistic License 2.0
 * along the source as a COPYING file. If not, obtain it from
 * http://www.perlfoundation.org/artistic_license_2_0.
 */

#ifndef _SHERLOCK_LV2_H
#define _SHERLOCK_LV2_H

#include <stdint.h>
#include <inttypes.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/ext/log/logger.h"
#include "lv2/lv2plug.in/ns/ext/options/options.h"
#include "lv2/lv2plug.in/ns/extensions/units/units.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include <stdio.h>
#include <props.h>
	
#define SHERLOCK_URI										"http://open-music-kontrollers.ch/lv2/sherlock"

#define SHERLOCK_ATOM_INSPECTOR_URI			SHERLOCK_URI"#atom_inspector"
#define SHERLOCK_ATOM_INSPECTOR_NK_URI	SHERLOCK_URI"#atom_inspector_4_nk"

#define SHERLOCK_MIDI_INSPECTOR_URI			SHERLOCK_URI"#midi_inspector"
#define SHERLOCK_MIDI_INSPECTOR_NK_URI	SHERLOCK_URI"#midi_inspector_4_nk"

#define SHERLOCK_OSC_INSPECTOR_URI			SHERLOCK_URI"#osc_inspector"
#define SHERLOCK_OSC_INSPECTOR_NK_URI		SHERLOCK_URI"#osc_inspector_4_nk"

extern const LV2_Descriptor atom_inspector;
extern const LV2_Descriptor midi_inspector;
extern const LV2_Descriptor osc_inspector;

typedef struct _position_t position_t;
typedef struct _state_t state_t;
typedef struct _craft_t craft_t;

struct _position_t {
	uint64_t offset;
	uint32_t nsamples;
};

struct _state_t {
	int32_t overwrite;
	int32_t block;
	int32_t follow;
	int32_t pretty;
	int32_t trace;
	uint32_t filter;
	int32_t negate;
};

struct _craft_t {
	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Frame frame [3];
	LV2_Atom_Forge_Ref ref;
	union {
		LV2_Atom_Sequence *seq;
		uint8_t *buf;
	};
};

#define MAX_NPROPS 7

static const props_def_t defs [MAX_NPROPS] = {
	{
		.property = SHERLOCK_URI"#overwrite",
		.offset = offsetof(state_t, overwrite),
		.type = LV2_ATOM__Bool,
	},
	{
		.property = SHERLOCK_URI"#block",
		.offset = offsetof(state_t, block),
		.type = LV2_ATOM__Bool,
	},
	{
		.property = SHERLOCK_URI"#follow",
		.offset = offsetof(state_t, follow),
		.type = LV2_ATOM__Bool,
	},
	{
		.property = SHERLOCK_URI"#pretty",
		.offset = offsetof(state_t, pretty),
		.type = LV2_ATOM__Bool,
	},
	{
		.property = SHERLOCK_URI"#trace",
		.offset = offsetof(state_t, trace),
		.type = LV2_ATOM__Bool,
	},
	{
		.property = SHERLOCK_URI"#filter",
		.offset = offsetof(state_t, filter),
		.type = LV2_ATOM__URID,
	},
	{
		.property = SHERLOCK_URI"#negate",
		.offset = offsetof(state_t, negate),
		.type = LV2_ATOM__Bool,
	}
};

// there is a bug in LV2 <= 0.10
#if defined(LV2_ATOM_TUPLE_FOREACH)
#	undef LV2_ATOM_TUPLE_FOREACH
#	define LV2_ATOM_TUPLE_FOREACH(tuple, iter) \
	for (LV2_Atom* (iter) = lv2_atom_tuple_begin(tuple); \
	     !lv2_atom_tuple_is_end(LV2_ATOM_BODY(tuple), (tuple)->atom.size, (iter)); \
	     (iter) = lv2_atom_tuple_next(iter))
#endif

#endif // _SHERLOCK_LV2_H
