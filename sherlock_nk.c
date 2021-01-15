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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <limits.h>

#include <sherlock.h>
#include <osc.lv2/util.h>

#define SER_ATOM_IMPLEMENTATION
#include <ser_atom.lv2/ser_atom.h>

#include <encoder.h>

#define NK_PUGL_IMPLEMENTATION
#include <sherlock_nk.h>

static inline void
_discover(plughandle_t *handle)
{
	ser_atom_t ser;

	if(ser_atom_init(&ser) == 0)
	{
		LV2_Atom_Forge_Frame frame;

		ser_atom_reset(&ser, &handle->forge);
		lv2_atom_forge_object(&handle->forge, &frame, 0, handle->props.urid.patch_get);
		lv2_atom_forge_pop(&handle->forge, &frame);

		handle->write_function(handle->controller, 0, lv2_atom_total_size(ser_atom_get(&ser)),
			handle->event_transfer, ser_atom_get(&ser));

		ser_atom_deinit(&ser);
	}
}

void
_set_bool(plughandle_t *handle, LV2_URID property, int32_t val)
{
	ser_atom_t ser;

	if(ser_atom_init(&ser) == 0)
	{
		LV2_Atom_Forge_Frame frame;

		ser_atom_reset(&ser, &handle->forge);
		lv2_atom_forge_object(&handle->forge, &frame, 0, handle->props.urid.patch_set);
		lv2_atom_forge_key(&handle->forge, handle->props.urid.patch_property);
		lv2_atom_forge_urid(&handle->forge, property);

		lv2_atom_forge_key(&handle->forge, handle->props.urid.patch_value);
		lv2_atom_forge_bool(&handle->forge, val);

		lv2_atom_forge_pop(&handle->forge, &frame);

		handle->write_function(handle->controller, 0, lv2_atom_total_size(ser_atom_get(&ser)),
			handle->event_transfer, ser_atom_get(&ser));

		ser_atom_deinit(&ser);
	}
}

void
_set_urid(plughandle_t *handle, LV2_URID property, uint32_t val)
{
	ser_atom_t ser;

	if(ser_atom_init(&ser) == 0)
	{
		LV2_Atom_Forge_Frame frame;

		ser_atom_reset(&ser, &handle->forge);
		lv2_atom_forge_object(&handle->forge, &frame, 0, handle->props.urid.patch_set);
		lv2_atom_forge_key(&handle->forge, handle->props.urid.patch_property);
		lv2_atom_forge_urid(&handle->forge, property);

		lv2_atom_forge_key(&handle->forge, handle->props.urid.patch_value);
		lv2_atom_forge_urid(&handle->forge, val);

		lv2_atom_forge_pop(&handle->forge, &frame);

		handle->write_function(handle->controller, 0, lv2_atom_total_size(ser_atom_get(&ser)),
			handle->event_transfer, ser_atom_get(&ser));

		ser_atom_deinit(&ser);
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
	nk_spacing(ctx, 1);
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
	struct nk_str *str = &handle->editor.string;
	nk_str_clear(str);
	handle->selected = NULL;
	handle->counter = 1;
}

void
_post_redisplay(plughandle_t *handle)
{
	nk_pugl_post_redisplay(&handle->win);
}

float
_get_scale (plughandle_t *handle)
{
	return handle->scale;
}

static int
_dial_bool(struct nk_context *ctx, int32_t *val, struct nk_color color, bool editable)
{
	const int32_t tmp = *val;
	struct nk_rect bounds;
	const bool left_mouse_click_in_cursor = nk_widget_is_mouse_clicked(ctx, NK_BUTTON_LEFT);
	const enum nk_widget_layout_states layout_states = nk_widget(&bounds, ctx);

	if(layout_states != NK_WIDGET_INVALID)
	{
		enum nk_widget_states states = NK_WIDGET_STATE_INACTIVE;
		struct nk_input *in = (ctx->current->layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;

		if(in && editable)
		{
			bool mouse_has_scrolled = false;

			if(left_mouse_click_in_cursor)
			{
				states = NK_WIDGET_STATE_ACTIVED;
			}
			else if(nk_input_is_mouse_hovering_rect(in, bounds))
			{
				if(in->mouse.scroll_delta.y != 0.f) // has scrolling
				{
					mouse_has_scrolled = true;
					in->mouse.scroll_delta.y = 0.f;
				}

				states = NK_WIDGET_STATE_HOVER;
			}

			if(left_mouse_click_in_cursor || mouse_has_scrolled)
			{
				*val = !*val;
			}
		}

		const struct nk_style_item *fg = NULL;

		switch(states)
		{
			case NK_WIDGET_STATE_HOVER:
			{
				fg = &ctx->style.progress.cursor_hover;
			}	break;
			case NK_WIDGET_STATE_ACTIVED:
			{
				fg = &ctx->style.progress.cursor_active;
			}	break;
			default:
			{
				fg = &ctx->style.progress.cursor_normal;
			}	break;
		}

		struct nk_color fg_color = fg->data.color;

		fg_color.r = (int)fg_color.r * color.r / 0xff;
		fg_color.g = (int)fg_color.g * color.g / 0xff;
		fg_color.b = (int)fg_color.b * color.b / 0xff;
		fg_color.a = (int)fg_color.a * color.a / 0xff;

		struct nk_command_buffer *canv= nk_window_get_canvas(ctx);
		const float w2 = bounds.w/2;
		const float h2 = bounds.h/2;
		const float r1 = NK_MIN(w2, h2);
		const float r2 = r1 / 2;
		const float cx = bounds.x + w2;
		const float cy = bounds.y + h2;

		nk_stroke_arc(canv, cx, cy, r2 - 0, 0.f, 2*M_PI, 2.f, fg_color);
		if(*val)
			nk_fill_arc(canv, cx, cy, r2 - 2, 0.f, 2*M_PI, fg_color);
	}

	return tmp != *val;
}

int32_t
_check(struct nk_context *ctx, int32_t state)
{
	_dial_bool(ctx, &state, nk_rgb(0xff, 0xff, 0xff), true);

	return state;
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor *descriptor __attribute__((unused)),
	const char *plugin_uri, const char *bundle_path __attribute__((unused)),
	LV2UI_Write_Function write_function, LV2UI_Controller controller,
	LV2UI_Widget *widget, const LV2_Feature *const *features)
{
	plughandle_t *handle = calloc(1, sizeof(plughandle_t));
	if(!handle)
		return NULL;

	void *parent = NULL;
	LV2UI_Resize *host_resize = NULL;
	LV2_Options_Option *opts = NULL;
	for(int i=0; features[i]; i++)
	{
		if(!strcmp(features[i]->URI, LV2_URID__map))
		{
			handle->map = features[i]->data;
		}
		else if(!strcmp(features[i]->URI, LV2_URID__unmap))
		{
			handle->unmap = features[i]->data;
		}
		else if(!strcmp(features[i]->URI, LV2_UI__parent))
		{
			parent = features[i]->data;
		}
		else if(!strcmp(features[i]->URI, LV2_UI__resize))
		{
			host_resize = features[i]->data;
		}
		else if(!strcmp(features[i]->URI, LV2_OPTIONS__options))
		{
			opts = features[i]->data;
		}
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

	if(!props_init(&handle->props, plugin_uri,
		defs, MAX_NPROPS, &handle->state, &handle->stash,
		handle->map, handle))
	{
		fprintf(stderr, "failed to allocate property structure\n");
		free(handle);
		return NULL;
	}

	handle->urid.overwrite = props_map(&handle->props, SHERLOCK_URI"#overwrite");
	handle->urid.block = props_map(&handle->props, SHERLOCK_URI"#block");
	handle->urid.follow = props_map(&handle->props, SHERLOCK_URI"#follow");
	handle->urid.pretty = props_map(&handle->props, SHERLOCK_URI"#pretty");
	handle->urid.trace = props_map(&handle->props, SHERLOCK_URI"#trace");
	handle->urid.filter = props_map(&handle->props, SHERLOCK_URI"#filter");
	handle->urid.negate = props_map(&handle->props, SHERLOCK_URI"#negate");

	nk_pugl_config_t *cfg = &handle->win.cfg;
	cfg->height = 700;
	cfg->resizable = true;
	cfg->ignore = false;
	cfg->class = "sherlock_inspector";
	cfg->title = "Sherlock Inspector";
	cfg->parent = (intptr_t)parent;
	cfg->host_resize = host_resize;
	cfg->data = handle;
	if(!strcmp(plugin_uri, SHERLOCK_MIDI_INSPECTOR_URI))
	{
		handle->type = SHERLOCK_MIDI_INSPECTOR,
		cfg->width = 600;
		cfg->expose = _midi_inspector_expose;
	}
	else if(!strcmp(plugin_uri, SHERLOCK_ATOM_INSPECTOR_URI))
	{
		handle->type = SHERLOCK_ATOM_INSPECTOR,
		cfg->width = 1200;
		cfg->expose = _atom_inspector_expose;
	}
	else if(!strcmp(plugin_uri, SHERLOCK_OSC_INSPECTOR_URI))
	{
		handle->type = SHERLOCK_OSC_INSPECTOR,
		cfg->width = 600;
		cfg->expose = _osc_inspector_expose;
	}

	if(asprintf(&cfg->font.face, "%sCousine-Regular.ttf", bundle_path) == -1)
		cfg->font.face= NULL;
	cfg->font.size = 13;

	*(intptr_t *)widget = nk_pugl_init(&handle->win);

	const LV2_URID ui_scaleFactor = handle->map->map(handle->map->handle,
		LV2_UI__scaleFactor);

	for(LV2_Options_Option *opt = opts;
		opt && (opt->key != 0) && (opt->value != NULL);
		opt++)
	{
		if( (opt->key == ui_scaleFactor) && (opt->type == handle->forge.Float) )
		{
			handle->scale = *(float*)opt->value;
		}
	}

	if(handle->scale == 0.f)
	{
		handle->scale = nk_pugl_get_scale(&handle->win);
	}

	nk_pugl_show(&handle->win);

	_clear(handle);
	_discover(handle);

	nk_textedit_init_default(&handle->editor);
	handle->editor.lexer.lex = ttl_lex;
	handle->editor.lexer.data = NULL;

	handle->sratom = sratom_new(handle->map);
	sratom_set_pretty_numbers(handle->sratom, handle->state.pretty);
	handle->base_uri = "file:///tmp/base";

	return handle;
}

static void
cleanup(LV2UI_Handle instance)
{
	plughandle_t *handle = instance;

	sratom_free(handle->sratom);

	_clear_items(handle);
	nk_textedit_free(&handle->editor);

	if(handle->editor.lexer.tokens)
		free(handle->editor.lexer.tokens);

	if(handle->win.cfg.font.face)
		free(handle->win.cfg.font.face);
	nk_pugl_hide(&handle->win);
	nk_pugl_shutdown(&handle->win);

	free(handle);
}

static void
_osc_bundle(plughandle_t *handle, const LV2_Atom_Object *obj);

static void
_osc_message(plughandle_t *handle, const LV2_Atom_Object *obj)
{
	const LV2_Atom_String *path = NULL;
	const LV2_Atom_Tuple *args = NULL;
	lv2_osc_message_get(&handle->osc_urid, obj, &path, &args);

	bool first = true;

	LV2_ATOM_TUPLE_FOREACH(args, arg)
	{
		if(first)
		{
			first = false;
		}
		else
		{
			_append_item(handle, ITEM_TYPE_NONE, 0);
		}
	}
}

static void
_osc_packet(plughandle_t *handle, const LV2_Atom_Object *obj)
{
	if(lv2_osc_is_message_type(&handle->osc_urid, obj->body.otype))
	{
		_append_item(handle, ITEM_TYPE_NONE, 0);
		_osc_message(handle, obj);
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
port_event(LV2UI_Handle instance, uint32_t i, uint32_t size __attribute__((unused)),
	uint32_t urid, const void *buf)
{
	plughandle_t *handle = instance;

	if(urid != handle->event_transfer)
		return;

	switch(i)
	{
		case 0:
		case 2:
		{
			const LV2_Atom_Tuple *tup = buf;

			if(tup->atom.type != handle->forge.Tuple)
			{
				ser_atom_t ser;

				if(ser_atom_init(&ser) == 0)
				{
					LV2_Atom_Forge_Frame frame;

					ser_atom_reset(&ser, &handle->forge);
					LV2_Atom_Forge_Ref ref = lv2_atom_forge_sequence_head(&handle->forge, &frame, 0);

					if(props_advance(&handle->props, &handle->forge, 0, buf, &ref))
					{
						nk_pugl_post_redisplay(&handle->win);
					}

					lv2_atom_forge_pop(&handle->forge, &frame);

					ser_atom_deinit(&ser);
				}

				break;
			}

			const LV2_Atom_Long *offset = NULL;
			const LV2_Atom_Int *nsamples = NULL;
			const LV2_Atom_Sequence *seq = NULL;

			unsigned k = 0;
			LV2_ATOM_TUPLE_FOREACH(tup, item)
			{
				switch(k)
				{
					case 0:
					{
						if(item->type == handle->forge.Long)
							offset = (const LV2_Atom_Long *)item;
					} break;
					case 1:
					{
						if(item->type == handle->forge.Int)
							nsamples = (const LV2_Atom_Int *)item;
					} break;
					case 2:
					{
						if(item->type == handle->forge.Sequence)
							seq = (const LV2_Atom_Sequence *)item;
					} break;
				}

				k++;
			}

			const bool overflow = handle->n_item > MAX_LINES;

			if(!offset || !nsamples || !seq || (seq->atom.size <= sizeof(LV2_Atom_Sequence_Body)) )
			{
				break;
			}

			if(overflow && handle->state.overwrite)
			{
				_clear(handle);
			}

			if(overflow || handle->state.block)
			{
				break;
			}

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
						const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;

						// bundles may span over multiple lines
						if(lv2_osc_is_bundle_type(&handle->osc_urid, obj->body.otype))
						{
							_osc_bundle(handle, obj);
						}
						else
						{
							_osc_message(handle, obj);
						}
					} break;
					case SHERLOCK_MIDI_INSPECTOR:
					{
						const uint8_t *msg = LV2_ATOM_BODY_CONST(&ev->body);

						// sysex messages may span over multiple lines
						if( (msg[0] == LV2_MIDI_MSG_SYSTEM_EXCLUSIVE) && (ev->body.size > 4) )
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

static int
_resize(LV2UI_Handle instance, int width, int height)
{
	plughandle_t *handle = instance;

	return nk_pugl_resize(&handle->win, width, height);
}

static const LV2UI_Resize resize_ext = {
	.ui_resize = _resize
};

static const void *
extension_data(const char *uri)
{
	if(!strcmp(uri, LV2_UI__idleInterface))
		return &idle_ext;
	else if(!strcmp(uri, LV2_UI__resize))
		return &resize_ext;

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
