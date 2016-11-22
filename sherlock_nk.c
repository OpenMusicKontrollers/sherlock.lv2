/*
 * Copyright (c) 2015-2016 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#include <sherlock.h>
#include <osc.lv2/util.h>

#define NK_PUGL_IMPLEMENTATION
#include <sherlock_nk.h>

#define ful 0xff
#define one 0xbb
#define two 0x66
#define non 0x0
const struct nk_color white		= {.r = one, .g = one, .b = one, .a = ful};
const struct nk_color gray		= {.r = two, .g = two, .b = two, .a = ful};
const struct nk_color yellow	= {.r = one, .g = one, .b = non, .a = ful};
const struct nk_color magenta	= {.r = one, .g = two, .b = one, .a = ful};
const struct nk_color green		= {.r = non, .g = one, .b = non, .a = ful};
const struct nk_color blue		= {.r = non, .g = one, .b = one, .a = ful};
const struct nk_color orange	= {.r = one, .g = two, .b = non, .a = ful};
const struct nk_color violet	= {.r = two, .g = two, .b = one, .a = ful};
const struct nk_color red			= {.r = one, .g = non, .b = non, .a = ful};

const char *max_items [5] = {
	"1k", "2k", "4k", "8k", "16k"
};
const int32_t max_values [5] = {
	0x400, 0x800, 0x1000, 0x2000, 0x4000
};

static LV2_Atom_Forge_Ref
_sink(LV2_Atom_Forge_Sink_Handle handle, const void *buf, uint32_t size)
{
	atom_ser_t *ser = handle;

	const LV2_Atom_Forge_Ref ref = ser->offset + 1;

	const uint32_t new_offset = ser->offset + size;
	if(new_offset > ser->size)
	{
		uint32_t new_size = ser->size << 1;
		while(new_offset > new_size)
			new_size <<= 1;

		if(!(ser->buf = realloc(ser->buf, new_size)))
			return 0; // realloc failed

		ser->size = new_size;
	}

	memcpy(ser->buf + ser->offset, buf, size);
	ser->offset = new_offset;

	return ref;
}

static LV2_Atom *
_deref(LV2_Atom_Forge_Sink_Handle handle, LV2_Atom_Forge_Ref ref)
{
	atom_ser_t *ser = handle;

	const uint32_t offset = ref - 1;

	return (LV2_Atom *)(ser->buf + offset);
}

static bool
_ser_malloc(atom_ser_t *ser, size_t size)
{
	ser->size = size;
	ser->offset = 0;
	ser->buf = malloc(ser->size);

	return ser->buf != NULL;
}

static bool
_ser_realloc(atom_ser_t *ser, size_t size)
{
	ser->size = size;
	ser->offset = 0;
	ser->buf = realloc(ser->buf, ser->size);

	return ser->buf != NULL;
}

static void
_ser_free(atom_ser_t *ser)
{
	if(ser->buf)
		free(ser->buf);
}

static inline void
_discover(plughandle_t *handle)
{
	atom_ser_t ser;

	if(_ser_malloc(&ser, 512))
	{
		lv2_atom_forge_set_sink(&handle->forge, _sink, _deref, &ser);
		LV2_Atom_Forge_Frame frame;

		lv2_atom_forge_object(&handle->forge, &frame, 0, handle->props.urid.patch_get);
		lv2_atom_forge_pop(&handle->forge, &frame);

		handle->write_function(handle->controller, 0, lv2_atom_total_size(ser.atom),
			handle->event_transfer, ser.atom);

		_ser_free(&ser);
	}
}

void
_toggle(plughandle_t *handle, LV2_URID property, int32_t val, bool is_bool)
{
	atom_ser_t ser;

	if(_ser_malloc(&ser, 512))
	{
		lv2_atom_forge_set_sink(&handle->forge, _sink, _deref, &ser);
		LV2_Atom_Forge_Frame frame;

		lv2_atom_forge_object(&handle->forge, &frame, 0, handle->props.urid.patch_set);
		lv2_atom_forge_key(&handle->forge, handle->props.urid.patch_property);
		lv2_atom_forge_urid(&handle->forge, property);

		lv2_atom_forge_key(&handle->forge, handle->props.urid.patch_value);
		if(is_bool)
			lv2_atom_forge_bool(&handle->forge, val);
		else
			lv2_atom_forge_int(&handle->forge, val);

		lv2_atom_forge_pop(&handle->forge, &frame);

		handle->write_function(handle->controller, 0, lv2_atom_total_size(ser.atom),
			handle->event_transfer, ser.atom);

		_ser_free(&ser);
	}
}

void
_ruler(struct nk_context *ctx, float line_thickness, struct nk_color color)
{
	struct nk_rect bounds = nk_layout_space_bounds(ctx);
	const enum nk_widget_layout_states states = nk_widget(&bounds, ctx);

	if(states != NK_WIDGET_INVALID)
	{
		struct nk_command_buffer *canv= nk_window_get_canvas(ctx);
		const float x0 = bounds.x;
		const float y0 = bounds.y + bounds.h/2;
		const float x1 = bounds.x + bounds.w;
		const float y1 = y0;

		nk_stroke_line(canv, x0, y0, x1, y1, line_thickness, color);
	}
}

void
_empty(struct nk_context *ctx)
{
	nk_text(ctx, NULL, 0, NK_TEXT_RIGHT);
}

static void
_clear_items(plughandle_t *handle)
{
	if(handle->items)
	{
		for(int i = 0; i < handle->n_item; i++)
		{
			item_t *itm = handle->items[i];

			if(itm)
				free(itm);
		}

		free(handle->items);
		handle->items = NULL;
	}

	handle->n_item = 0;
}

static item_t *
_append_item(plughandle_t *handle, item_type_t type, size_t sz)
{
	handle->items = realloc(handle->items, (handle->n_item + 1)*sizeof(item_t *));
	handle->items[handle->n_item] = malloc(sizeof(item_t) + sz);

	item_t *itm = handle->items[handle->n_item];
	itm->type = type;

	handle->n_item += 1;

	return itm;
}

void
_clear(plughandle_t *handle)
{
	_clear_items(handle);
	nk_str_clear(&handle->str);
	handle->selected = NULL;
	handle->counter = 0;
}

void
_post_redisplay(plughandle_t *handle)
{
	nk_pugl_post_redisplay(&handle->win);
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor, const char *plugin_uri,
	const char *bundle_path, LV2UI_Write_Function write_function,
	LV2UI_Controller controller, LV2UI_Widget *widget,
	const LV2_Feature *const *features)
{
	plughandle_t *handle = calloc(1, sizeof(plughandle_t));
	if(!handle)
		return NULL;

	void *parent = NULL;
	LV2UI_Resize *host_resize = NULL;
	for(int i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
			handle->map = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
			handle->unmap = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__parent))
			parent = features[i]->data;
		else if(!strcmp(features[i]->URI, LV2_UI__resize))
			host_resize = features[i]->data;
  }

	if(!handle->map || !handle->unmap)
	{
		fprintf(stderr, "LV2 URID extension not supported\n");
		free(handle);
		return NULL;
	}
	if(!parent)
	{
		free(handle);
		return NULL;
	}

	handle->event_transfer = handle->map->map(handle->map->handle, LV2_ATOM__eventTransfer);
	lv2_atom_forge_init(&handle->forge, handle->map);
	lv2_osc_urid_init(&handle->osc_urid, handle->map);

	handle->write_function = write_function;
	handle->controller = controller;

	if(!props_init(&handle->props, MAX_NPROPS, plugin_uri, handle->map, handle))
	{
		fprintf(stderr, "failed to allocate property structure\n");
		free(handle);
		return NULL;
	}

	if(  !(handle->urid.overwrite = props_register(&handle->props, &stat_overwrite, &handle->state.overwrite, &handle->stash.overwrite))
		|| !(handle->urid.block = props_register(&handle->props, &stat_block, &handle->state.block, &handle->stash.block))
		|| !(handle->urid.follow = props_register(&handle->props, &stat_follow, &handle->state.follow, &handle->stash.follow)) )
	{
		free(handle);
		return NULL;
	}

	const char *NK_SCALE = getenv("NK_SCALE");
	const float scale = NK_SCALE ? atof(NK_SCALE) : 1.f;
	handle->dy = 20.f * scale;

	nk_pugl_config_t *cfg = &handle->win.cfg;
	cfg->height = 700 * scale;
	cfg->resizable = true;
	cfg->ignore = false;
	cfg->class = "sherlock_inspector";
	cfg->title = "Sherlock Inspector";
	cfg->parent = (intptr_t)parent;
	cfg->data = handle;
	if(!strcmp(plugin_uri, SHERLOCK_MIDI_INSPECTOR_URI))
	{
		handle->type = SHERLOCK_MIDI_INSPECTOR,
		cfg->width = 600 * scale;
		cfg->expose = _midi_inspector_expose;
	}
	else if(!strcmp(plugin_uri, SHERLOCK_ATOM_INSPECTOR_URI))
	{
		handle->type = SHERLOCK_ATOM_INSPECTOR,
		cfg->width = 1200 * scale;
		cfg->expose = _atom_inspector_expose;
	}
	else if(!strcmp(plugin_uri, SHERLOCK_OSC_INSPECTOR_URI))
	{
		handle->type = SHERLOCK_OSC_INSPECTOR,
		cfg->width = 600 * scale;
		cfg->expose = _osc_inspector_expose;
	}

	char *path;
	if(asprintf(&path, "%sCousine-Regular.ttf", bundle_path) == -1)
		path = NULL;

	cfg->font.face = path;
	cfg->font.size = 13 * scale;

	*(intptr_t *)widget = nk_pugl_init(&handle->win);
	nk_pugl_show(&handle->win);

	if(path)
		free(path);

	if(host_resize)
		host_resize->ui_resize(host_resize->handle, cfg->width, cfg->height);

	_clear(handle);
	_discover(handle);

	nk_str_init_default(&handle->str);

	handle->sratom = sratom_new(handle->map);
	sratom_set_pretty_numbers(handle->sratom, false);
	handle->base_uri = "file:///tmp/base";

	return handle;
}

static void
cleanup(LV2UI_Handle instance)
{
	plughandle_t *handle = instance;

	sratom_free(handle->sratom);

	_clear_items(handle);
	nk_str_free(&handle->str);

	nk_pugl_hide(&handle->win);
	nk_pugl_shutdown(&handle->win);

	free(handle);
}

static void
_osc_bundle(plughandle_t *handle, const LV2_Atom_Object *obj);

static void
_osc_packet(plughandle_t *handle, const LV2_Atom_Object *obj)
{
	if(lv2_osc_is_message_type(&handle->osc_urid, obj->body.otype))
	{
		_append_item(handle, ITEM_TYPE_NONE, 0);
	}
	else if(lv2_osc_is_bundle_type(&handle->osc_urid, obj->body.otype))
	{
		_append_item(handle, ITEM_TYPE_NONE, 0);
		_osc_bundle(handle, obj);
	}
}

static void
_osc_bundle(plughandle_t *handle, const LV2_Atom_Object *obj)
{
	const LV2_Atom_Object *timetag = NULL;
	const LV2_Atom_Tuple *items = NULL;
	lv2_osc_bundle_get(&handle->osc_urid, obj, &timetag, &items);

	LV2_ATOM_TUPLE_FOREACH(items, item)
	{
		_osc_packet(handle, (const LV2_Atom_Object *)item);
	}
}

static void
port_event(LV2UI_Handle instance, uint32_t i, uint32_t size, uint32_t urid,
	const void *buf)
{
	plughandle_t *handle = instance;

	if(urid != handle->event_transfer)
		return;

	switch(i)
	{
		case 0:
		case 1:
		{
			atom_ser_t ser;

			if(_ser_malloc(&ser, 512))
			{
				LV2_Atom_Forge_Frame frame;
				lv2_atom_forge_set_sink(&handle->forge, _sink, _deref, &ser);
				LV2_Atom_Forge_Ref ref = lv2_atom_forge_sequence_head(&handle->forge, &frame, 0);

				if(props_advance(&handle->props, &handle->forge, 0, buf, &ref))
					nk_pugl_post_redisplay(&handle->win);

				lv2_atom_forge_pop(&handle->forge, &frame);

				_ser_free(&ser);
			}

			break;
		}
		case 2:
		{
			if(handle->state.block)
			{
				break;
			}

			if( (handle->n_item > MAX_LINES) && handle->state.overwrite)
			{
				_clear(handle);
			}

			const LV2_Atom *atom = buf;
			const LV2_Atom_Tuple *tup = (const LV2_Atom_Tuple *)atom;
			const LV2_Atom_Long *offset = (const LV2_Atom_Long *)lv2_atom_tuple_begin(tup);
			const LV2_Atom_Int *nsamples = (const LV2_Atom_Int *)lv2_atom_tuple_next(&offset->atom);
			const LV2_Atom_Sequence *seq = (const LV2_Atom_Sequence *)lv2_atom_tuple_next(&nsamples->atom);

			// append frame
			{
				item_t *itm = _append_item(handle, ITEM_TYPE_FRAME, 0);
				itm->frame.offset = offset->body;
				itm->frame.counter = handle->counter++;
				itm->frame.nsamples = nsamples->body;
			}

			LV2_ATOM_SEQUENCE_FOREACH(seq, ev)
			{
				const size_t ev_sz = sizeof(LV2_Atom_Event) + ev->body.size;
				item_t *itm = _append_item(handle, ITEM_TYPE_EVENT, ev_sz);
				memcpy(&itm->event.ev, ev, ev_sz);

				switch(handle->type)
				{
					case SHERLOCK_ATOM_INSPECTOR:
					{
						if(handle->state.follow)
						{
							handle->selected = &itm->event.ev.body;
							handle->ttl_dirty = true;
						}
					} break;
					case SHERLOCK_OSC_INSPECTOR:
					{
						// bundles may span over multiple lines
						const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;
						if(lv2_osc_is_bundle_type(&handle->osc_urid, obj->body.otype))
						{
							_osc_bundle(handle, obj);
						}
					} break;
					case SHERLOCK_MIDI_INSPECTOR:
					{
						// sysex messages may span over multiple lines
						const uint8_t *msg = LV2_ATOM_BODY_CONST(&ev->body);
						if( (msg[0] == 0xf0) && (ev->body.size > 4) )
						{
							for(uint32_t j = 4; j < ev->body.size; j += 4)
								_append_item(handle, ITEM_TYPE_NONE, 0); // place holder
						}
					} break;
				}
			}

			nk_pugl_post_redisplay(&handle->win);

			break;
		}
	}
}

static int
_idle(LV2UI_Handle instance)
{
	plughandle_t *handle = instance;

	return nk_pugl_process_events(&handle->win);
}

static const LV2UI_Idle_Interface idle_ext = {
	.idle = _idle
};

static const void *
extension_data(const char *uri)
{
	if(!strcmp(uri, LV2_UI__idleInterface))
		return &idle_ext;

	return NULL;
}

const LV2UI_Descriptor midi_inspector_nk = {
	.URI						= SHERLOCK_MIDI_INSPECTOR_NK_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= extension_data
};

const LV2UI_Descriptor atom_inspector_nk = {
	.URI						= SHERLOCK_ATOM_INSPECTOR_NK_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= extension_data
};

const LV2UI_Descriptor osc_inspector_nk = {
	.URI						= SHERLOCK_OSC_INSPECTOR_NK_URI,
	.instantiate		= instantiate,
	.cleanup				= cleanup,
	.port_event			= port_event,
	.extension_data	= extension_data
};

#ifdef _WIN32
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
const LV2UI_Descriptor*
lv2ui_descriptor(uint32_t index)
{
	switch(index)
	{
		case 0:
			return &midi_inspector_nk;
		case 1:
			return &atom_inspector_nk;
		case 2:
			return &osc_inspector_nk;

		default:
			return NULL;
	}
}
