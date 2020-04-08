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

#ifndef ENCODER_H
#define ENCODER_H

#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#define NK_PUGL_API
#include "nk_pugl/nk_pugl.h"

extern const struct nk_color cwhite;
extern const struct nk_color gray;
extern const struct nk_color yellow;
extern const struct nk_color magenta;
extern const struct nk_color green;
extern const struct nk_color blue;
extern const struct nk_color orange;
extern const struct nk_color violet;
extern const struct nk_color red;

struct nk_token *
ttl_lex(void *data, const char *utf8, int len);

#endif
