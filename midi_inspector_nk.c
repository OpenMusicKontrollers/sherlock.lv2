/*
 * Copyright (c) 2015-2019 Hanspeter Portner (dev@open-music-kontrollers.ch)
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

#include <inttypes.h>

#include <sherlock.h>
#include <sherlock_nk.h>
#include <encoder.h>

typedef struct _midi_msg_t midi_msg_t;

struct _midi_msg_t {
	uint8_t type;
	const char *key;
};

#define COMMANDS_NUM 18
static const midi_msg_t commands [COMMANDS_NUM] = {
	{ LV2_MIDI_MSG_NOTE_OFF							, "NoteOff" },
	{ LV2_MIDI_MSG_NOTE_ON							, "NoteOn" },
	{ LV2_MIDI_MSG_NOTE_PRESSURE				, "NotePressure" },
	{ LV2_MIDI_MSG_CONTROLLER						, "Controller" },
	{ LV2_MIDI_MSG_PGM_CHANGE						, "ProgramChange" },
	{ LV2_MIDI_MSG_CHANNEL_PRESSURE			, "ChannelPressure" },
	{ LV2_MIDI_MSG_BENDER								, "Bender" },
	{ LV2_MIDI_MSG_SYSTEM_EXCLUSIVE			, "SystemExclusive" },
	{ LV2_MIDI_MSG_MTC_QUARTER					, "QuarterFrame" },
	{ LV2_MIDI_MSG_SONG_POS							, "SongPosition" },
	{ LV2_MIDI_MSG_SONG_SELECT					, "SongSelect" },
	{ LV2_MIDI_MSG_TUNE_REQUEST					, "TuneRequest" },
	{ LV2_MIDI_MSG_CLOCK								, "Clock" },
	{ LV2_MIDI_MSG_START								, "Start" },
	{ LV2_MIDI_MSG_CONTINUE							, "Continue" },
	{ LV2_MIDI_MSG_STOP									, "Stop" },
	{ LV2_MIDI_MSG_ACTIVE_SENSE					, "ActiveSense" },
	{ LV2_MIDI_MSG_RESET								, "Reset" },
};

#define CONTROLLERS_NUM 72
static const midi_msg_t controllers [CONTROLLERS_NUM] = {
	{ LV2_MIDI_CTL_MSB_BANK             , "BankSelection_MSB" },
	{ LV2_MIDI_CTL_MSB_MODWHEEL         , "Modulation_MSB" },
	{ LV2_MIDI_CTL_MSB_BREATH           , "Breath_MSB" },
	{ LV2_MIDI_CTL_MSB_FOOT             , "Foot_MSB" },
	{ LV2_MIDI_CTL_MSB_PORTAMENTO_TIME  , "PortamentoTime_MSB" },
	{ LV2_MIDI_CTL_MSB_DATA_ENTRY       , "DataEntry_MSB" },
	{ LV2_MIDI_CTL_MSB_MAIN_VOLUME      , "MainVolume_MSB" },
	{ LV2_MIDI_CTL_MSB_BALANCE          , "Balance_MSB" },
	{ LV2_MIDI_CTL_MSB_PAN              , "Panpot_MSB" },
	{ LV2_MIDI_CTL_MSB_EXPRESSION       , "Expression_MSB" },
	{ LV2_MIDI_CTL_MSB_EFFECT1          , "Effect1_MSB" },
	{ LV2_MIDI_CTL_MSB_EFFECT2          , "Effect2_MSB" },
	{ LV2_MIDI_CTL_MSB_GENERAL_PURPOSE1 , "GeneralPurpose1_MSB" },
	{ LV2_MIDI_CTL_MSB_GENERAL_PURPOSE2 , "GeneralPurpose2_MSB" },
	{ LV2_MIDI_CTL_MSB_GENERAL_PURPOSE3 , "GeneralPurpose3_MSB" },
	{ LV2_MIDI_CTL_MSB_GENERAL_PURPOSE4 , "GeneralPurpose4_MSB" },

	{ LV2_MIDI_CTL_LSB_BANK             , "BankSelection_LSB" },
	{ LV2_MIDI_CTL_LSB_MODWHEEL         , "Modulation_LSB" },
	{ LV2_MIDI_CTL_LSB_BREATH           , "Breath_LSB" },
	{ LV2_MIDI_CTL_LSB_FOOT             , "Foot_LSB" },
	{ LV2_MIDI_CTL_LSB_PORTAMENTO_TIME  , "PortamentoTime_LSB" },
	{ LV2_MIDI_CTL_LSB_DATA_ENTRY       , "DataEntry_LSB" },
	{ LV2_MIDI_CTL_LSB_MAIN_VOLUME      , "MainVolume_LSB" },
	{ LV2_MIDI_CTL_LSB_BALANCE          , "Balance_LSB" },
	{ LV2_MIDI_CTL_LSB_PAN              , "Panpot_LSB" },
	{ LV2_MIDI_CTL_LSB_EXPRESSION       , "Expression_LSB" },
	{ LV2_MIDI_CTL_LSB_EFFECT1          , "Effect1_LSB" },
	{ LV2_MIDI_CTL_LSB_EFFECT2          , "Effect2_LSB" },
	{ LV2_MIDI_CTL_LSB_GENERAL_PURPOSE1 , "GeneralPurpose1_LSB" },
	{ LV2_MIDI_CTL_LSB_GENERAL_PURPOSE2 , "GeneralPurpose2_LSB" },
	{ LV2_MIDI_CTL_LSB_GENERAL_PURPOSE3 , "GeneralPurpose3_LSB" },
	{ LV2_MIDI_CTL_LSB_GENERAL_PURPOSE4 , "GeneralPurpose4_LSB" },

	{ LV2_MIDI_CTL_SUSTAIN              , "SustainPedal" },
	{ LV2_MIDI_CTL_PORTAMENTO           , "Portamento" },
	{ LV2_MIDI_CTL_SOSTENUTO            , "Sostenuto" },
	{ LV2_MIDI_CTL_SOFT_PEDAL           , "SoftPedal" },
	{ LV2_MIDI_CTL_LEGATO_FOOTSWITCH    , "LegatoFootSwitch" },
	{ LV2_MIDI_CTL_HOLD2                , "Hold2" },

	{ LV2_MIDI_CTL_SC1_SOUND_VARIATION  , "SC1_SoundVariation" },
	{ LV2_MIDI_CTL_SC2_TIMBRE           , "SC2_Timbre" },
	{ LV2_MIDI_CTL_SC3_RELEASE_TIME     , "SC3_ReleaseTime" },
	{ LV2_MIDI_CTL_SC4_ATTACK_TIME      , "SC4_AttackTime" },
	{ LV2_MIDI_CTL_SC5_BRIGHTNESS       , "SC5_Brightness" },
	{ LV2_MIDI_CTL_SC6                  , "SC6" },
	{ LV2_MIDI_CTL_SC7                  , "SC7" },
	{ LV2_MIDI_CTL_SC8                  , "SC8" },
	{ LV2_MIDI_CTL_SC9                  , "SC9" },
	{ LV2_MIDI_CTL_SC10                 , "SC10" },

	{ LV2_MIDI_CTL_GENERAL_PURPOSE5     , "GeneralPurpose5" },
	{ LV2_MIDI_CTL_GENERAL_PURPOSE6     , "GeneralPurpose6" },
	{ LV2_MIDI_CTL_GENERAL_PURPOSE7     , "GeneralPurpose7" },
	{ LV2_MIDI_CTL_GENERAL_PURPOSE8     , "GeneralPurpose8" },
	{ LV2_MIDI_CTL_PORTAMENTO_CONTROL   , "PortamentoControl" },

	{ LV2_MIDI_CTL_E1_REVERB_DEPTH      , "E1_ReverbDepth" },
	{ LV2_MIDI_CTL_E2_TREMOLO_DEPTH     , "E2_TremoloDepth" },
	{ LV2_MIDI_CTL_E3_CHORUS_DEPTH      , "E3_ChorusDepth" },
	{ LV2_MIDI_CTL_E4_DETUNE_DEPTH      , "E4_DetuneDepth" },
	{ LV2_MIDI_CTL_E5_PHASER_DEPTH      , "E5_PhaserDepth" },

	{ LV2_MIDI_CTL_DATA_INCREMENT       , "DataIncrement" },
	{ LV2_MIDI_CTL_DATA_DECREMENT       , "DataDecrement" },

	{ LV2_MIDI_CTL_NRPN_LSB             , "NRPN_LSB" },
	{ LV2_MIDI_CTL_NRPN_MSB             , "NRPN_MSB" },

	{ LV2_MIDI_CTL_RPN_LSB              , "RPN_LSB" },
	{ LV2_MIDI_CTL_RPN_MSB              , "RPN_MSB" },

	{ LV2_MIDI_CTL_ALL_SOUNDS_OFF       , "AllSoundsOff" },
	{ LV2_MIDI_CTL_RESET_CONTROLLERS    , "ResetControllers" },
	{ LV2_MIDI_CTL_LOCAL_CONTROL_SWITCH , "LocalControlSwitch" },
	{ LV2_MIDI_CTL_ALL_NOTES_OFF        , "AllNotesOff" },
	{ LV2_MIDI_CTL_OMNI_OFF             , "OmniOff" },
	{ LV2_MIDI_CTL_OMNI_ON              , "OmniOn" },
	{ LV2_MIDI_CTL_MONO1                , "Mono1" },
	{ LV2_MIDI_CTL_MONO2                , "Mono2" },
};

#define TIMECODES_NUM 8
static const midi_msg_t timecodes [TIMECODES_NUM] = {
	{ 0 , "FrameNumber_LSB" },
	{ 1 , "FrameNumber_MSB" },
	{ 2 , "Second_LSB" },
	{ 3 , "Second_MSB" },
	{ 4 , "Minute_LSB" },
	{ 5 , "Minute_MSB" },
	{ 6 , "Hour_LSB" },
	{ 7 , "RateAndHour_MSB" },
};

static int
_cmp_search(const void *itm1, const void *itm2)
{
	const midi_msg_t *msg1 = itm1;
	const midi_msg_t *msg2 = itm2;

	if(msg1->type < msg2->type)
		return -1;
	else if(msg1->type > msg2->type)
		return 1;

	return 0;
}

static inline const midi_msg_t *
_search_command(uint8_t type)
{
	return bsearch(&type, commands, COMMANDS_NUM, sizeof(midi_msg_t), _cmp_search);
}

static inline const midi_msg_t *
_search_controller(uint8_t type)
{
	return bsearch(&type, controllers, CONTROLLERS_NUM, sizeof(midi_msg_t), _cmp_search);
}

static inline const midi_msg_t *
_search_timecode(uint8_t type)
{
	return bsearch(&type, timecodes, TIMECODES_NUM, sizeof(midi_msg_t), _cmp_search);
}

static const char *keys [12] = {
	"C", "C#",
	"D", "D#",
	"E",
	"F", "F#",
	"G", "G#",
	"A", "A#",
	"B"
};

static inline const char *
_note(uint8_t val, int8_t *octave)
{
	*octave = val / 12 - 1;

	return keys[val % 12];
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
_midi_inspector_expose(struct nk_context *ctx, struct nk_rect wbounds, void *data)
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
						// skip, was sysex payload
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
						const uint8_t *msg = LV2_ATOM_BODY_CONST(body);
						const uint8_t cmd = (msg[0] & 0xf0) == 0xf0
							? msg[0]
							: msg[0] & 0xf0;

						const midi_msg_t *command_msg = _search_command(cmd);
						const char *command_str = command_msg
							? command_msg->key
							: "Unknown";

						char tmp [16];
						nk_layout_row_begin(ctx, NK_DYNAMIC, widget_h, 7);
						{
							nk_layout_row_push(ctx, 0.1);
							_shadow(ctx, &handle->shadow);
							nk_labelf_colored(ctx, NK_TEXT_LEFT, yellow, "+%04"PRIi64, frames);

							nk_layout_row_push(ctx, 0.2);
							const unsigned rem = body->size;
							const unsigned to = rem >= 4 ? 4 : rem % 4;
							for(unsigned i=0, ptr=0; i<to; i++, ptr+=3)
								sprintf(&tmp[ptr], "%02"PRIX8" ", msg[i]);
							tmp[to*3 - 1] = '\0';
							nk_label_colored(ctx, tmp, NK_TEXT_LEFT, cwhite);

							nk_layout_row_push(ctx, 0.2);
							nk_label_colored(ctx, command_str, NK_TEXT_LEFT, magenta);

							switch(cmd)
							{
								case LV2_MIDI_MSG_NOTE_OFF:
									// fall-through
								case LV2_MIDI_MSG_NOTE_ON:
									// fall-through
								case LV2_MIDI_MSG_NOTE_PRESSURE:
								{
									nk_layout_row_push(ctx, 0.1);
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, cwhite, "Ch:%02"PRIu8,
										(msg[0] & 0x0f) + 1);

									nk_layout_row_push(ctx, 0.2);
									int8_t octave;
									const char *key = _note(msg[1], &octave);
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, cwhite, "%s%+"PRIi8, key, octave);

									nk_layout_row_push(ctx, 0.1);
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, cwhite, "%"PRIu8, msg[2]);
								} break;
								case LV2_MIDI_MSG_CONTROLLER:
								{
									nk_layout_row_push(ctx, 0.1);
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, cwhite, "Ch:%02"PRIu8,
										(msg[0] & 0x0f) + 1);

									const midi_msg_t *controller_msg = _search_controller(msg[1]);
									const char *controller_str = controller_msg
										? controller_msg->key
										: "Unknown";
									nk_layout_row_push(ctx, 0.2);
									nk_label_colored(ctx, controller_str, NK_TEXT_RIGHT, cwhite);

									nk_layout_row_push(ctx, 0.1);
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, cwhite, "%"PRIu8, msg[2]);
								} break;
								case LV2_MIDI_MSG_PGM_CHANGE:
									// fall-through
								case LV2_MIDI_MSG_CHANNEL_PRESSURE:
								{
									nk_layout_row_push(ctx, 0.1);
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, cwhite, "Ch:%02"PRIu8,
										(msg[0] & 0x0f) + 1);

									nk_layout_row_push(ctx, 0.2);
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, cwhite, "%"PRIu8, msg[1]);

									nk_layout_row_push(ctx, 0.1);
									_empty(ctx);
								}	break;
								case LV2_MIDI_MSG_BENDER:
								{
									const int16_t bender = (((int16_t)msg[2] << 7) | msg[1]) - 0x2000;

									nk_layout_row_push(ctx, 0.1);
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, cwhite, "Ch:%02"PRIu8,
										(msg[0] & 0x0f) + 1);

									nk_layout_row_push(ctx, 0.2);
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, cwhite, "%"PRIi16, bender);

									nk_layout_row_push(ctx, 0.1);
									_empty(ctx);
								}	break;
								case LV2_MIDI_MSG_MTC_QUARTER:
								{
									const uint8_t msg_type = msg[1] >> 4;
									const uint8_t msg_val = msg[1] & 0xf;

									nk_layout_row_push(ctx, 0.1);
									_empty(ctx);

									const midi_msg_t *timecode_msg = _search_timecode(msg_type);
									const char *timecode_str = timecode_msg
										? timecode_msg->key
										: "Unknown";
									nk_layout_row_push(ctx, 0.2);
									nk_label_colored(ctx, timecode_str, NK_TEXT_RIGHT, cwhite);

									nk_layout_row_push(ctx, 0.1);
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, cwhite, "%"PRIu8, msg_val);
								} break;
								case LV2_MIDI_MSG_SONG_POS:
								{
									const int16_t song_pos= (((int16_t)msg[2] << 7) | msg[1]);

									nk_layout_row_push(ctx, 0.1);
									_empty(ctx);

									nk_layout_row_push(ctx, 0.2);
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, cwhite, "%"PRIu16, song_pos);

									nk_layout_row_push(ctx, 0.1);
									_empty(ctx);
								} break;
								case LV2_MIDI_MSG_SONG_SELECT:
								{
									nk_layout_row_push(ctx, 0.1);
									_empty(ctx);

									nk_layout_row_push(ctx, 0.2);
									nk_labelf_colored(ctx, NK_TEXT_RIGHT, cwhite, "%"PRIu8, msg[1]);

									nk_layout_row_push(ctx, 0.1);
									_empty(ctx);
								} break;
								case LV2_MIDI_MSG_SYSTEM_EXCLUSIVE:
									// fall-throuh
								case LV2_MIDI_MSG_TUNE_REQUEST:
									// fall-throuh
								case LV2_MIDI_MSG_CLOCK:
									// fall-throuh
								case LV2_MIDI_MSG_START:
									// fall-throuh
								case LV2_MIDI_MSG_CONTINUE:
									// fall-throuh
								case LV2_MIDI_MSG_STOP:
									// fall-throuh
								case LV2_MIDI_MSG_ACTIVE_SENSE:
									// fall-throuh
								case LV2_MIDI_MSG_RESET:
								{
									nk_layout_row_push(ctx, 0.1);
									_empty(ctx);

									nk_layout_row_push(ctx, 0.2);
									_empty(ctx);

									nk_layout_row_push(ctx, 0.1);
									_empty(ctx);
								} break;
							}

							nk_layout_row_push(ctx, 0.1);
							nk_labelf_colored(ctx, NK_TEXT_RIGHT, blue, "%"PRIu32, body->size);
						}
						nk_layout_row_end(ctx);

						for(unsigned j=4; j<body->size; j+=4)
						{
							nk_layout_row_begin(ctx, NK_DYNAMIC, widget_h, 7);
							{
								nk_layout_row_push(ctx, 0.1);
								_shadow(ctx, &handle->shadow);
								_empty(ctx);

								nk_layout_row_push(ctx, 0.2);
								const unsigned rem = body->size - j;
								const unsigned to = rem >= 4 ? 4 : rem % 4;
								for(unsigned i=0, ptr=0; i<to; i++, ptr+=3)
									sprintf(&tmp[ptr], "%02"PRIX8" ", msg[j+i]);
								tmp[to*3 - 1] = '\0';
								nk_label_colored(ctx, tmp, NK_TEXT_LEFT, cwhite);

								nk_layout_row_push(ctx, 0.2);
								_empty(ctx);

								nk_layout_row_push(ctx, 0.1);
								_empty(ctx);

								nk_layout_row_push(ctx, 0.2);
								_empty(ctx);

								nk_layout_row_push(ctx, 0.1);
								_empty(ctx);

								nk_layout_row_push(ctx, 0.1);
								_empty(ctx);
							}
						}
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
