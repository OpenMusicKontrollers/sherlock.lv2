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

#include <time.h>
#include <math.h>
#include <inttypes.h>

#include <sherlock.h>
#include <sherlock_nk.h>
#include <encoder.h>

#include <osc.lv2/util.h>

typedef struct _mem_t mem_t;

struct _mem_t {
	size_t size;
	char *buf;
};

static inline void
_shadow(struct nk_context *ctx, bool *shadow)
{
	if(*shadow)
	{
		struct nk_style *style = &ctx->style;
		const struct nk_vec2 group_padding = style->window.group_padding;
		struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);

		struct nk_rect b = nk_widget_bounds(ctx);
		b.x -= group_padding.x;
		b.w *= 10;
		b.w += 5*group_padding.x;
		nk_fill_rect(canvas, b, 0.f, nk_rgb(0x28, 0x28, 0x28));
	}

	*shadow = !*shadow;
}

static inline void
_mem_printf(mem_t *mem, const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);

	char *src;
	if(vasprintf(&src, fmt, args) != -1)
	{
		const size_t sz = strlen(src);

		mem->buf = realloc(mem->buf, mem->size + sz);
		if(mem->buf)
		{
			char *dst = mem->buf + mem->size - 1;

			strncpy(dst, src, sz + 1);
			mem->size += sz;
		}
		else
			mem->size = 0;

		free(src);
	}

	va_end(args);
}

static void
_osc_timetag(mem_t *mem, LV2_OSC_Timetag *tt)
{
	if(tt->integral <= 1UL)
	{
		_mem_printf(mem, "t : %s", "immediate");
	}
	else
	{
		const uint32_t us = floor(tt->fraction * 0x1p-32 * 1e6);
		const time_t ttime = tt->integral - 0x83aa7e80;
		const struct tm *ltime = localtime(&ttime);

		char tmp [32];
		if(strftime(tmp, 32, "%d-%b-%Y %T", ltime))
			_mem_printf(mem, "t : %s.%06"PRIu32, tmp, us);
	}
}

static void
_osc_argument(plughandle_t *handle, struct nk_context *ctx, const LV2_Atom *arg,
	const char *path, bool first)
{
	const LV2_OSC_Type type = lv2_osc_argument_type(&handle->osc_urid, arg);

	mem_t mem = {
		.size = 1,
		.buf = calloc(1, sizeof(char))
	};

	switch(type)
	{
		case LV2_OSC_INT32:
		{
			int32_t i;
			lv2_osc_int32_get(&handle->osc_urid, arg, &i);
			_mem_printf(&mem, "i : %"PRIi32, i);
		} break;
		case LV2_OSC_FLOAT:
		{
			float f;
			lv2_osc_float_get(&handle->osc_urid, arg, &f);
			_mem_printf(&mem, "f : %f", f);
		} break;
		case LV2_OSC_STRING:
		{
			const char *s;
			lv2_osc_string_get(&handle->osc_urid, arg, &s);
			_mem_printf(&mem, "s : %s", s);
		} break;
		case LV2_OSC_BLOB:
		{
			uint32_t sz;
			const uint8_t *b;
			lv2_osc_blob_get(&handle->osc_urid, arg, &sz, &b);
			_mem_printf(&mem, "b : (%"PRIu32") ", sz);
			for(unsigned i=0; i<sz; i++)
				_mem_printf(&mem, "%02"PRIx8, b[i]);
		} break;

		case LV2_OSC_TRUE:
		{
			_mem_printf(&mem, "T : true");
		} break;
		case LV2_OSC_FALSE:
		{
			_mem_printf(&mem, "F : false");
		} break;
		case LV2_OSC_NIL:
		{
			_mem_printf(&mem, "N : nil");
		} break;
		case LV2_OSC_IMPULSE:
		{
			_mem_printf(&mem, "I : impulse");
		} break;

		case LV2_OSC_INT64:
		{
			int64_t h;
			lv2_osc_int64_get(&handle->osc_urid, arg, &h);
			_mem_printf(&mem, "h : %"PRIi64, h);
		} break;
		case LV2_OSC_DOUBLE:
		{
			double d;
			lv2_osc_double_get(&handle->osc_urid, arg, &d);
			_mem_printf(&mem, "d : %lf", d);
		} break;
		case LV2_OSC_TIMETAG:
		{
			LV2_OSC_Timetag tt;
			lv2_osc_timetag_get(&handle->osc_urid, arg, &tt);
			_osc_timetag(&mem, &tt);
		} break;

		case LV2_OSC_SYMBOL:
		{
			LV2_URID S;
			lv2_osc_symbol_get(&handle->osc_urid, arg, &S);
			_mem_printf(&mem, "S : %s", handle->unmap->unmap(handle->unmap->handle, S));
		} break;
		case LV2_OSC_CHAR:
		{
			char c;
			lv2_osc_char_get(&handle->osc_urid, arg, &c);
			_mem_printf(&mem, "c : %c", c);
		} break;
		case LV2_OSC_RGBA:
		{
			uint8_t r [4];
			lv2_osc_rgba_get(&handle->osc_urid, arg, r+0, r+1, r+2, r+3);
			_mem_printf(&mem, "r : ");
			for(unsigned i=0; i<4; i++)
				_mem_printf(&mem, "%02"PRIx8, r[i]);
		} break;
		case LV2_OSC_MIDI:
		{
			uint32_t sz;
			const uint8_t *m;
			lv2_osc_midi_get(&handle->osc_urid, arg, &sz, &m);
			_mem_printf(&mem, "m : (%"PRIu32") ", sz);
			for(unsigned i=0; i<sz; i++)
				_mem_printf(&mem, "%02"PRIx8, m[i]);
		} break;
	}

	if(!first)
	{
		_shadow(ctx, &handle->shadow);
		_empty(ctx);
	}

	if(first)
	{
		nk_label_colored(ctx, path, NK_TEXT_LEFT, magenta);
	}
	else
	{
		_empty(ctx);
	}

	if(mem.buf)
	{
		nk_label_colored(ctx, mem.buf, NK_TEXT_LEFT, cwhite);
		free(mem.buf);
	}
	else
	{
		_empty(ctx);
	}

	nk_labelf_colored(ctx, NK_TEXT_RIGHT, blue, "%"PRIu32, arg->size);
}

static void
_osc_message(plughandle_t *handle, struct nk_context *ctx, const LV2_Atom_Object *obj)
{
	const LV2_Atom_String *path = NULL;
	const LV2_Atom_Tuple *args = NULL;
	lv2_osc_message_get(&handle->osc_urid, obj, &path, &args);

	bool first = true;
	LV2_ATOM_TUPLE_FOREACH(args, arg)
	{
		_osc_argument(handle, ctx, arg, LV2_ATOM_BODY_CONST(path), first);

		if(first)
			first = false;
	}
}

static void
_osc_packet(plughandle_t *handle, struct nk_context *ctx, int64_t frames,
	const LV2_Atom_Object *obj, float offset);

static void
_osc_bundle(plughandle_t *handle, struct nk_context *ctx, const LV2_Atom_Object *obj, float offset)
{
	const LV2_Atom_Object *timetag = NULL;
	const LV2_Atom_Tuple *items = NULL;
	lv2_osc_bundle_get(&handle->osc_urid, obj, &timetag, &items);

	// format bundle timestamp
	LV2_OSC_Timetag tt;
	lv2_osc_timetag_get(&handle->osc_urid, &timetag->atom, &tt);

	mem_t mem = {
		.size = 1,
		.buf = calloc(1, sizeof(char))
	};

	nk_label_colored(ctx, "#bundle", NK_TEXT_LEFT, red);

	_osc_timetag(&mem, &tt);
	if(mem.buf)
	{
		nk_label_colored(ctx, mem.buf, NK_TEXT_LEFT, cwhite);
		free(mem.buf);
	}
	else
	{
		_empty(ctx);
	}

	nk_labelf_colored(ctx, NK_TEXT_RIGHT, blue, "%"PRIu32, obj->atom.size);

	LV2_ATOM_TUPLE_FOREACH(items, item)
	{
		_osc_packet(handle, ctx, -1, (const LV2_Atom_Object *)item, offset + 0.025);
	}
}

static void
_osc_packet(plughandle_t *handle, struct nk_context *ctx, int64_t frames,
	const LV2_Atom_Object *obj, float offset)
{
	const float widget_h = handle->dy;

	const float ratios [4] = {offset, 0.3f - offset, 0.6f, 0.1f};
	nk_layout_row(ctx, NK_DYNAMIC, widget_h, 4, ratios);

	_shadow(ctx, &handle->shadow);
	if(frames >= 0)
	{
		nk_labelf_colored(ctx, NK_TEXT_LEFT, yellow, "+%04"PRIi64, frames);
	}
	else
	{
		_empty(ctx);
	}

	if(lv2_osc_is_message_type(&handle->osc_urid, obj->body.otype))
	{
		_osc_message(handle, ctx, obj);
	}
	else if(lv2_osc_is_bundle_type(&handle->osc_urid, obj->body.otype))
	{
		_osc_bundle(handle, ctx, obj, offset);
	}
}

void
_osc_inspector_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data)
{
	plughandle_t *handle = data;

	handle->dy = 20.f * _get_scale(handle);
	const float widget_h = handle->dy;
	struct nk_style *style = &ctx->style;
  const struct nk_vec2 window_padding = style->window.padding;
  const struct nk_vec2 group_padding = style->window.group_padding;

	const char *window_name = "Sherlock";
	if(nk_begin(ctx, window_name, wbounds, NK_WINDOW_NO_SCROLLBAR))
	{
		struct nk_panel *panel = nk_window_get_panel(ctx);
		struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);

		const float body_h = panel->bounds.h - 4*window_padding.y - 2*widget_h;
		nk_layout_row_dynamic(ctx, body_h, 1);
		nk_flags flags = NK_WINDOW_BORDER;
		if(handle->state.follow)
		{
			flags |= NK_WINDOW_NO_SCROLLBAR;
		}
		else
		{
			handle->shadow = false;
		}
		struct nk_list_view lview;
		if(nk_list_view_begin(ctx, &lview, "Events", flags, widget_h, NK_MIN(handle->n_item, MAX_LINES)))
		{
			if(handle->state.follow)
			{
				lview.end = NK_MAX(handle->n_item, 0);
				lview.begin = NK_MAX(lview.end - lview.count, 0);
			}
			for(int l = lview.begin; (l < lview.end) && (l < handle->n_item); l++)
			{
				item_t *itm = handle->items[l];

				switch(itm->type)
				{
					case ITEM_TYPE_NONE:
					{
						// skip, was bundle payload
					} break;
					case ITEM_TYPE_FRAME:
					{
						nk_layout_row_dynamic(ctx, widget_h, 3);
						{
							struct nk_rect b = nk_widget_bounds(ctx);
							b.x -= group_padding.x;
							b.w *= 3;
							b.w += 4*group_padding.x;
							nk_fill_rect(canvas, b, 0.f, nk_rgb(0x18, 0x18, 0x18));
						}

						nk_labelf_colored(ctx, NK_TEXT_LEFT, orange, "@%"PRIi64, itm->frame.offset);
						nk_labelf_colored(ctx, NK_TEXT_CENTERED, green, "-%"PRIu32"-", itm->frame.counter);
						nk_labelf_colored(ctx, NK_TEXT_RIGHT, violet, "%"PRIi32, itm->frame.nsamples);

						handle->shadow = false;
					} break;

					case ITEM_TYPE_EVENT:
					{
						LV2_Atom_Event *ev = &itm->event.ev;
						const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;
						const int64_t frames = ev->time.frames;
						const float offset = 0.1;

						_osc_packet(handle, ctx, frames, obj, offset);
					} break;
				}
			}

			nk_list_view_end(&lview);
		}

		const float n = 3;
		const float r0 = 1.f / n;
		const float r1 = 0.1f / 3;
		const float r2 = r0 - r1;
		const float footer [6] = {r1, r2, r1, r2, r1, r2};
		nk_layout_row(ctx, NK_DYNAMIC, widget_h, 6, footer);
		{
			const int32_t state_overwrite = _check(ctx, handle->state.overwrite);
			if(state_overwrite != handle->state.overwrite)
			{
				handle->state.overwrite = state_overwrite;
				_set_bool(handle, handle->urid.overwrite, handle->state.overwrite);
			}
			nk_label(ctx, "overwrite", NK_TEXT_LEFT);

			const int32_t state_block = _check(ctx, handle->state.block);
			if(state_block != handle->state.block)
			{
				handle->state.block = state_block;
				_set_bool(handle, handle->urid.block, handle->state.block);
			}
			nk_label(ctx, "block", NK_TEXT_LEFT);

			const int32_t state_follow = _check(ctx, handle->state.follow);
			if(state_follow != handle->state.follow)
			{
				handle->state.follow = state_follow;
				_set_bool(handle, handle->urid.follow, handle->state.follow);
			}
			nk_label(ctx, "follow", NK_TEXT_LEFT);
		}

		const bool max_reached = handle->n_item >= MAX_LINES;
		nk_layout_row_dynamic(ctx, widget_h, 2);
		if(nk_button_symbol_label(ctx,
			max_reached ? NK_SYMBOL_TRIANGLE_RIGHT: NK_SYMBOL_NONE,
			"clear", NK_TEXT_LEFT))
		{
			_clear(handle);
		}
		nk_label(ctx, "Sherlock.lv2: "SHERLOCK_VERSION, NK_TEXT_RIGHT);
	}
	nk_end(ctx);
}
