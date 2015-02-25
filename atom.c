/*
 * Copyright (c) 2015 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#include <stdio.h>
#include <stdlib.h>

#include <sherlock.h>

typedef struct _handle_t handle_t;

struct _handle_t {
	LV2_URID_Map *map;
	struct {
		LV2_URID event_transfer;
		LV2_URID sherlock_object;
		LV2_URID sherlock_frametime;
		LV2_URID sherlock_event;
	} uris;

	const LV2_Atom_Sequence *event_in;
	LV2_Atom_Sequence *event_out;
	LV2_Atom_Forge forge;
};

static LV2_Handle
instantiate(const LV2_Descriptor* descriptor, double rate, const char *bundle_path, const LV2_Feature *const *features)
{
	int i;
	handle_t *handle = calloc(1, sizeof(handle_t));
	if(!handle)
		return NULL;

	for(i=0; features[i]; i++)
		if(!strcmp(features[i]->URI, LV2_URID__map))
			handle->map = (LV2_URID_Map *)features[i]->data;

	if(!handle->map)
	{
		fprintf(stderr, "%s: Host does not support urid:map\n", descriptor->URI);
		free(handle);
		return NULL;
	}

	handle->uris.event_transfer = handle->map->map(handle->map->handle, LV2_ATOM__eventTransfer);
	handle->uris.sherlock_object = handle->map->map(handle->map->handle, SHERLOCK_OBJECT_URI);
	handle->uris.sherlock_frametime = handle->map->map(handle->map->handle, SHERLOCK_FRAMETIME_URI);
	handle->uris.sherlock_event = handle->map->map(handle->map->handle, SHERLOCK_EVENT_URI);
	lv2_atom_forge_init(&handle->forge, handle->map);

	return handle;
}

static void
connect_port(LV2_Handle instance, uint32_t port, void *data)
{
	handle_t *handle = (handle_t *)instance;

	switch(port)
	{
		case 0:
			handle->event_in = (const LV2_Atom_Sequence *)data;
			break;
		case 1:
			handle->event_out = (LV2_Atom_Sequence *)data;
			break;
		default:
			break;
	}
}

static void
activate(LV2_Handle instance)
{
	handle_t *handle = (handle_t *)instance;
	//nothing
}

static void
run(LV2_Handle instance, uint32_t nsamples)
{
	handle_t *handle = (handle_t *)instance;

	// prepare osc atom forge
	const uint32_t capacity = handle->event_out->atom.size;
	LV2_Atom_Forge *forge = &handle->forge;
	lv2_atom_forge_set_buffer(forge, (uint8_t *)handle->event_out, capacity);
	LV2_Atom_Forge_Frame frame;
	lv2_atom_forge_sequence_head(forge, &frame, 0);
	
	LV2_Atom_Event *ev = NULL;
	LV2_ATOM_SEQUENCE_FOREACH(handle->event_in, ev)
	{
		LV2_Atom_Forge_Frame obj;
		int64_t frames = ev->time.frames;
		size_t size = ev->body.size;

		/*
		lv2_atom_forge_frame_time(forge, frames);
		lv2_atom_forge_raw(forge, &ev->body, size + sizeof(LV2_Atom));
		lv2_atom_forge_pad(forge, size);
		*/

		lv2_atom_forge_frame_time(forge, frames);
		lv2_atom_forge_object(forge, &obj, 0, handle->uris.sherlock_object);
		{
			lv2_atom_forge_key(forge, handle->uris.sherlock_frametime);
			lv2_atom_forge_long(forge, frames);

			lv2_atom_forge_key(forge, handle->uris.sherlock_event);
			lv2_atom_forge_raw(forge, &ev->body, size + sizeof(LV2_Atom));

			lv2_atom_forge_pad(forge, size);
		}
		lv2_atom_forge_pop(forge, &obj);
	}

	lv2_atom_forge_pop(forge, &frame);
}

static void
deactivate(LV2_Handle instance)
{
	handle_t *handle = (handle_t *)instance;
	//nothing
}

static void
cleanup(LV2_Handle instance)
{
	handle_t *handle = (handle_t *)instance;

	free(handle);
}

static const void*
extension_data(const char* uri)
{
	return NULL;
}

const LV2_Descriptor atom = {
	.URI						= SHERLOCK_ATOM_URI,
	.instantiate		= instantiate,
	.connect_port		= connect_port,
	.activate				= activate,
	.run						= run,
	.deactivate			= deactivate,
	.cleanup				= cleanup,
	.extension_data	= extension_data
};
