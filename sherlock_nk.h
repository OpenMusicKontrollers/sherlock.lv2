/*
 * Copyright (c) 2015-2021 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#ifndef _SHERLOCK_NK_H
#define _SHERLOCK_NK_H

#define NK_PUGL_API
#include "nk_pugl/nk_pugl.h"

#include <osc.lv2/osc.h>
#include <sratom/sratom.h>

#define MAX_LINES 2048

typedef struct _item_t item_t;
typedef struct _plughandle_t plughandle_t;

typedef enum _item_type_t {
	ITEM_TYPE_NONE,
	ITEM_TYPE_FRAME,
	ITEM_TYPE_EVENT
} item_type_t;

struct _item_t {
	item_type_t type;

	union {
		struct {
			int64_t offset;
			uint32_t counter;
			int32_t nsamples;
		} frame;

		struct {
			LV2_Atom_Event ev;
			uint8_t body [0];
		} event;
	};
};

typedef enum _plugin_type_t {
	SHERLOCK_ATOM_INSPECTOR,
	SHERLOCK_MIDI_INSPECTOR,
	SHERLOCK_OSC_INSPECTOR
} plugin_type_t;

struct _plughandle_t {
	LV2UI_Write_Function write_function;
	LV2UI_Controller controller;

	LV2_URID_Map *map;
	LV2_URID_Unmap *unmap;
	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Frame frame;
	LV2_URID event_transfer;
	LV2_OSC_URID osc_urid;

	float scale;

	PROPS_T(props, MAX_NPROPS);
	struct {
		LV2_URID overwrite;
		LV2_URID block;
		LV2_URID follow;
		LV2_URID pretty;
		LV2_URID trace;
		LV2_URID filter;
		LV2_URID negate;
	} urid;
	state_t state;
	state_t stash;

	nk_pugl_window_t win;

	bool ttl_dirty;
	const LV2_Atom *selected;
	struct nk_text_edit editor;

	Sratom *sratom;
	const char *base_uri;

	float dy;

	uint32_t counter;
	int n_item;
	item_t **items;

	bool shadow;
	plugin_type_t type;

	char filter_uri [1024];
	LV2_URID filter;
};

extern const char *max_items [5];
extern const int32_t max_values [5];

void
_midi_inspector_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data);

void
_atom_inspector_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data);

void
_osc_inspector_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data);

void
_empty(struct nk_context *ctx);

void
_set_bool(plughandle_t *handle, LV2_URID property, int32_t val);

void
_set_urid(plughandle_t *handle, LV2_URID property, uint32_t val);

void
_clear(plughandle_t *handle);

void
_ruler(struct nk_context *ctx, float line_thickness, struct nk_color color);

void
_post_redisplay(plughandle_t *handle);

float
_get_scale(plughandle_t *handle);

int32_t
_check(struct nk_context *ctx, int32_t state);

#endif // _SHERLOCK_NK_H
