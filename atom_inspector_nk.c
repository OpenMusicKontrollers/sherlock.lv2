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

#include <ctype.h>
#include <inttypes.h>

#include <sherlock.h>
#include <sherlock_nk.h>
#include <encoder.h>

#ifdef Bool // hack for xlib
#	undef Bool
#endif

#define NS_RDF (const uint8_t*)"http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_RDFS (const uint8_t*)"http://www.w3.org/2000/01/rdf-schema#"
#define NS_XSD (const uint8_t*)"http://www.w3.org/2001/XMLSchema#"
#define NS_OSC (const uint8_t*)"http://open-music-kontrollers.ch/lv2/osc#"
#define NS_XPRESS (const uint8_t*)"http://open-music-kontrollers.ch/lv2/xpress#"
#define NS_SPOD (const uint8_t*)"http://open-music-kontrollers.ch/lv2/synthpod#"
#define NS_CANVAS (const uint8_t*)"http://open-music-kontrollers.ch/lv2/canvas#"

// copyied and adapted from libsratom 
static inline char *
_sratom_to_turtle(Sratom*         sratom,
                 LV2_URID_Unmap* unmap,
                 const char*     base_uri,
                 const SerdNode* subject,
                 const SerdNode* predicate,
                 uint32_t        type,
                 uint32_t        size,
                 const void*     body)
{
	SerdURI   buri = SERD_URI_NULL;
	SerdNode  base = serd_node_new_uri_from_string((uint8_t *)(base_uri), NULL, &buri);
	SerdEnv*  env  = serd_env_new(&base);
	SerdChunk str  = { NULL, 0 };

	serd_env_set_prefix_from_strings(env, (const uint8_t *)"rdf", NS_RDF);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"rdfs", NS_RDFS);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"xsd", NS_XSD);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"lv2", (const uint8_t *)LV2_CORE_PREFIX);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"midi", (const uint8_t *)LV2_MIDI_PREFIX);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"atom", (const uint8_t *)LV2_ATOM_PREFIX);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"units", (const uint8_t *)LV2_UNITS_PREFIX);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"ui", (const uint8_t *)LV2_UI_PREFIX);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"time", (const uint8_t *)LV2_TIME_URI"#");
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"patch", (const uint8_t *)LV2_PATCH_PREFIX);

	serd_env_set_prefix_from_strings(env, (const uint8_t *)"osc", NS_OSC);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"xpress", NS_XPRESS);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"spod", NS_SPOD);
	serd_env_set_prefix_from_strings(env, (const uint8_t *)"canvas", NS_CANVAS);

	SerdWriter* writer = serd_writer_new(
		SERD_TURTLE,
		(SerdStyle)(SERD_STYLE_ABBREVIATED |
		            SERD_STYLE_RESOLVED |
		            SERD_STYLE_CURIED |
								SERD_STYLE_ASCII),
		env, &buri, serd_chunk_sink, &str);

	// Write @prefix directives
	serd_env_foreach(env,
	                 (SerdPrefixSink)serd_writer_set_prefix,
	                 writer);

	sratom_set_sink(sratom, base_uri,
	                (SerdStatementSink)serd_writer_write_statement,
	                (SerdEndSink)serd_writer_end_anon,
	                writer);
	sratom_write(sratom, unmap, SERD_EMPTY_S,
	             subject, predicate, type, size, body);
	serd_writer_finish(writer);

	serd_writer_free(writer);
	serd_env_free(env);
	serd_node_free(&base);
	return (char*)serd_chunk_sink_finish(&str);
}

static void
_set_string(struct nk_str *str, uint32_t size, const char *body)
{
	nk_str_clear(str);

	// replace tab with 2 spaces
	const char *end = body + size - 1;
	const char *from = body;
	for(const char *to = strchr(from, '\t');
		to && (to < end);
		from = to + 1, to = strchr(from, '\t'))
	{
		nk_str_append_text_utf8(str, from, to-from);
		nk_str_append_text_utf8(str, "  ", 2);
	}
	nk_str_append_text_utf8(str, from, end-from);
}

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

void
_atom_inspector_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data)
{
	plughandle_t *handle = data;

	handle->dy = 20.f * _get_scale(handle);
	const float widget_h = handle->dy;
	struct nk_style *style = &ctx->style;
  const struct nk_vec2 window_padding = style->window.padding;
  const struct nk_vec2 group_padding = style->window.group_padding;

	style->selectable.normal.data.color.a = 0x0;
	style->selectable.hover.data.color.a = 0x0;

	const char *window_name = "Sherlock";
	if(nk_begin(ctx, window_name, wbounds, NK_WINDOW_NO_SCROLLBAR))
	{
		struct nk_panel *panel= nk_window_get_panel(ctx);
		struct nk_command_buffer *canvas = nk_window_get_canvas(ctx);

		const float body_h = panel->bounds.h - 2*window_padding.y;
		nk_layout_row_dynamic(ctx, body_h, 2);
		if(nk_group_begin(ctx, "Left", NK_WINDOW_NO_SCROLLBAR))
		{
			{
				// has filter URID been updated meanwhile ?
				if(handle->filter != handle->state.filter)
				{
					const char *uri = handle->unmap->unmap(handle->unmap->handle, handle->state.filter);

					if(uri)
					{
						strncpy(handle->filter_uri, uri, sizeof(handle->filter_uri) - 1);
					}
					else
					{
						handle->filter_uri[0] = '\0';
					}

					handle->filter = handle->state.filter;
				}

				bool dirty = false;

				const float n = 4;
				const float r0 = 1.f / n;
				const float r1 = 0.1f / 3;
				const float r2 = r0 - r1;
				const float footer [5] = {r1+r2, r1+r2, r1+r2, r1, r2};
				nk_layout_row(ctx, NK_DYNAMIC, widget_h, 5, footer);
				{
					nk_label(ctx, "match:", NK_TEXT_LEFT);

					if(nk_button_label(ctx, "time"))
					{
						strncpy(handle->filter_uri, LV2_TIME__Position, sizeof(handle->filter_uri) - 1);
						dirty = true;
					}

					if(nk_button_label(ctx, "all"))
					{
						strncpy(handle->filter_uri, SHERLOCK_URI"#matchAll", sizeof(handle->filter_uri) - 1);
						dirty = true;
					}

					const int32_t state_negate = _check(ctx, handle->state.negate);
					if(state_negate != handle->state.negate)
					{
						handle->state.negate = state_negate;
						_set_bool(handle, handle->urid.negate, handle->state.negate);
					}
					nk_label(ctx, "negate", NK_TEXT_LEFT);
				}

				nk_layout_row_dynamic(ctx, widget_h, 1);
				{
					const nk_flags flags = NK_EDIT_FIELD
						| NK_EDIT_AUTO_SELECT
						| NK_EDIT_SIG_ENTER;
					//nk_edit_focus(ctx, flags);
					nk_flags mode = nk_edit_string_zero_terminated(ctx, flags, handle->filter_uri, sizeof(handle->filter_uri) - 1, nk_filter_ascii);
					if(mode & NK_EDIT_COMMITED)
					{
						if(strlen(handle->filter_uri) == 0)
						{
							strncpy(handle->filter_uri, SHERLOCK_URI"#matchAll", sizeof(handle->filter_uri) - 1);
						}
						dirty = true;
					}
				}

				if(dirty)
				{
					handle->state.filter = handle->map->map(handle->map->handle, handle->filter_uri);
					_set_urid(handle, handle->urid.filter, handle->state.filter);
				}
			}

			const float content_h = nk_window_get_height(ctx) - 3*window_padding.y - 7*group_padding.y - 4*widget_h;
			nk_layout_row_dynamic(ctx, content_h, 1);
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
							// skip, never reached
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
							const LV2_Atom *body = &ev->body;
							const int64_t frames = ev->time.frames;
							const char *uri = NULL;
							if(lv2_atom_forge_is_object_type(&handle->forge, body->type))
							{
								const LV2_Atom_Object *obj = (const LV2_Atom_Object *)body;

								if(obj->body.otype)
									uri = handle->unmap->unmap(handle->unmap->handle, obj->body.otype);
								else if(obj->body.id)
									uri = handle->unmap->unmap(handle->unmap->handle, obj->body.id);
								else
									uri = "Unknown";
							}
							else // not an object
							{
								uri = handle->unmap->unmap(handle->unmap->handle, body->type);
							}

							const float entry [4] = {0.1, 0.65, 0.15, 0.1};
							nk_layout_row(ctx, NK_DYNAMIC, widget_h, 4, entry);
							{
								_shadow(ctx, &handle->shadow);
								nk_labelf_colored(ctx, NK_TEXT_LEFT, yellow, "+%04"PRIi64, frames);

								if(nk_select_label(ctx, uri, NK_TEXT_LEFT, handle->selected == body))
								{
									handle->ttl_dirty = handle->ttl_dirty
										|| (handle->selected != body); // has selection actually changed?
									handle->selected = body;
								}

								if(body->type == handle->forge.Bool)
								{
									const LV2_Atom_Bool *ref = (const LV2_Atom_Bool *)body;
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, violet, "%s", ref->body ? "true" : "false");
								}
								else if(body->type == handle->forge.Int)
								{
									const LV2_Atom_Int *ref = (const LV2_Atom_Int *)body;
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, green, "%"PRIi32, ref->body);
								}
								else if(body->type == handle->forge.Long)
								{
									const LV2_Atom_Long *ref = (const LV2_Atom_Long *)body;
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, green, "%"PRIi64, ref->body);
								}
								else if(body->type == handle->forge.Float)
								{
									const LV2_Atom_Float *ref = (const LV2_Atom_Float *)body;
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, green, "%f", ref->body);
								}
								else if(body->type == handle->forge.Double)
								{
									const LV2_Atom_Double *ref = (const LV2_Atom_Double *)body;
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, green, "%lf", ref->body);
								}
								/*
								else if(body->type == handle->forge.String)
								{
									nk_label_colored(ctx, LV2_ATOM_BODY_CONST(body), NK_TEXT_RIGHT, red);
								}
								else if(body->type == handle->forge.URI)
								{
									nk_label_colored(ctx, LV2_ATOM_BODY_CONST(body), NK_TEXT_RIGHT, yellow);
								}
								else if(body->type == handle->forge.URID)
								{
									const LV2_Atom_URID *urid = (const LV2_Atom_URID *)body;
									const char *_uri = handle->unmap->unmap(handle->unmap->handle, urid->body);
									nk_label_colored(ctx, _uri, NK_TEXT_RIGHT, yellow);
								}
								else if(body->type == handle->forge.Path)
								{
									nk_label_colored(ctx, LV2_ATOM_BODY_CONST(body), NK_TEXT_RIGHT, red);
								}
								else if(body->type == handle->forge.Literal)
								{
									nk_label_colored(ctx, LV2_ATOM_CONTENTS_CONST(LV2_Atom_Literal, body), NK_TEXT_RIGHT, red);
								}
								*/
								else
								{
									nk_spacing(ctx, 1);
								}

								nk_labelf_colored(ctx, NK_TEXT_RIGHT, blue, "%"PRIu32, body->size);
							}
						} break;
					}
				}

				nk_list_view_end(&lview);
			}

			const float n = 4;
			const float r0 = 1.f / n;
			const float r1 = 0.1f / 3;
			const float r2 = r0 - r1;
			const float footer [8] = {r1, r2, r1, r2, r1, r2, r1, r2};
			nk_layout_row(ctx, NK_DYNAMIC, widget_h, 8, footer);
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

				const int32_t state_pretty = _check(ctx, handle->state.pretty);
				if(state_pretty != handle->state.pretty)
				{
					handle->state.pretty = state_pretty;
					_set_bool(handle, handle->urid.pretty, handle->state.pretty);

					handle->ttl_dirty = true;
				}
				nk_label(ctx, "pretty", NK_TEXT_LEFT);
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

			nk_group_end(ctx);
		}

		if(nk_group_begin(ctx, "Right", NK_WINDOW_NO_SCROLLBAR))
		{
			const LV2_Atom *atom = handle->selected;
			if(handle->ttl_dirty && atom)
			{
				sratom_set_pretty_numbers(handle->sratom, handle->state.pretty);

				char *ttl = _sratom_to_turtle(handle->sratom, handle->unmap,
					handle->base_uri, NULL, NULL,
					atom->type, atom->size, LV2_ATOM_BODY_CONST(atom));
				if(ttl)
				{
					struct nk_str *str = &handle->editor.string;
					const size_t len = strlen(ttl);

					_set_string(str, len, ttl);

					handle->editor.lexer.needs_refresh = 1;

					free(ttl);
				}

				handle->ttl_dirty = false;
			}

			const nk_flags flags = NK_EDIT_EDITOR
				| NK_EDIT_READ_ONLY;
			int len = nk_str_len(&handle->editor.string);

			if(len > 0) //FIXME
			{
				const float content_h = nk_window_get_height(ctx) - 2*window_padding.y - 2*group_padding.y;
				nk_layout_row_dynamic(ctx, content_h, 1);
				const nk_flags mode = nk_edit_buffer(ctx, flags, &handle->editor, nk_filter_default);
				(void)mode;
			}

			nk_group_end(ctx);
		}
	}
	nk_end(ctx);
}
