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

%{
#include <stdio.h>
#include <string.h>

#include <encoder.h>

#define ful 0xff
#define one 0xbb
#define two 0x66
#define non 0x0
const struct nk_color cwhite	= {.r = one, .g = one, .b = one, .a = ful};
const struct nk_color gray		= {.r = two, .g = two, .b = two, .a = ful};
const struct nk_color yellow	= {.r = one, .g = one, .b = non, .a = ful};
const struct nk_color magenta	= {.r = one, .g = two, .b = one, .a = ful};
const struct nk_color green		= {.r = non, .g = one, .b = non, .a = ful};
const struct nk_color blue		= {.r = non, .g = one, .b = one, .a = ful};
const struct nk_color orange	= {.r = one, .g = two, .b = non, .a = ful};
const struct nk_color violet	= {.r = two, .g = two, .b = one, .a = ful};
const struct nk_color red			= {.r = one, .g = non, .b = non, .a = ful};

enum {
	TK_NONE,
	TK_PREFIX,
	TK_SUBJECT,
	TK_PREDICATE,
	TK_BOOL,
	TK_NUMBER,
	TK_URI_IN,
	TK_URI_OUT,
	TK_URI_ERR,
	TK_STRING_IN,
	TK_STRING_OUT,
	TK_STRING_ERR,
	TK_LONG_STRING_IN,
	TK_LONG_STRING_OUT,
	TK_WHITESPACE,
	TK_RAW,
	TK_EOL
};

%}

%option reentrant noyywrap

w              [ \v\a\t]+
name           [_a-zA-Z@][_a-zA-Z0-9\.]*
n              [0-9]+
exp            [Ee][+-]?{n}
number         ({n}|{n}[.]{n}){exp}?
eol            [\n\r]

%x XSTRING
%x XLONG_STRING
%x XURI

%%

{w}            return TK_WHITESPACE;
"<"            BEGIN(XURI); return TK_URI_IN;
\"\"\"         BEGIN(XLONG_STRING); return TK_LONG_STRING_IN;
\"             BEGIN(XSTRING); return TK_STRING_IN;
{name}:        return TK_SUBJECT;
"@prefix"      return TK_PREFIX;
"a"            return TK_PREFIX;
true           return TK_BOOL;
false          return TK_BOOL;
{name}         return TK_PREDICATE;
{number}       return TK_NUMBER;
{eol}          return TK_RAW;
.              return TK_RAW;

<XURI>
{
   ">"         BEGIN(0); return TK_URI_OUT;
   {eol}	     BEGIN(0); return TK_URI_ERR;
   .           return TK_RAW;
}

<XLONG_STRING>
{
   \"\"\"      BEGIN(0); return TK_LONG_STRING_OUT;
   {eol}	     return TK_EOL;
   .           return TK_RAW;
}

<XSTRING>
{
   \\\"	       return TK_RAW;
   \"          BEGIN(0); return TK_STRING_OUT;
   {eol}	     BEGIN(0); return TK_STRING_ERR;
   .           return TK_RAW;
}

%%

struct nk_token *
ttl_lex(void *data, const char *utf8, int len)
{
	yyscan_t scanner;
	YY_BUFFER_STATE buf;

	enclex_init(&scanner);
	if(utf8)
	{
		buf = enc_scan_bytes(utf8, len, scanner);
	}
	else
	{
		enclex_destroy(scanner);
		return NULL;
	}

	struct nk_token *tokens = NULL;
	int n_tokens = 0;
				
	const char *base = encget_text(scanner);
	int offset0 = 0;
	struct nk_color col0 = {0xff, 0xff, 0xff, 0xff};

	for(int tok=enclex(scanner); tok; tok=enclex(scanner))
	{
		const char *txt = encget_text(scanner);
		const int offset1 = txt - base;
		struct nk_color col1 = col0;

		switch(tok)
		{
			case TK_PREFIX:
				col1 = blue;
				break;
			case TK_SUBJECT:
				col1 = magenta;
				break;
			case TK_PREDICATE:
				col1 = orange;
				break;
			case TK_BOOL:
				col1 = violet;
				break;
			case TK_NUMBER:
				col1 = green;
				break;
			case TK_URI_IN:
			case TK_URI_OUT:
			case TK_URI_ERR:
				col1 = yellow;
				break;

			case TK_STRING_IN:
			case TK_STRING_OUT:
			case TK_STRING_ERR:
			case TK_LONG_STRING_IN:
			case TK_LONG_STRING_OUT:
				col1 = red;
				break;

			case TK_NONE:
			case TK_WHITESPACE:
				col1 = cwhite;
				break;

			case TK_EOL:
				continue;

			case TK_RAW:
			default:
				// skip over
				break;
		}

		if(offset1)
		{
			tokens = realloc(tokens, (n_tokens + 1) * sizeof(struct nk_token));
			tokens[n_tokens].offset = offset1;
			tokens[n_tokens++].color = col0;
		}

		offset0 = offset1;
		col0 = col1;
	}

	tokens = realloc(tokens, (n_tokens + 1) * sizeof(struct nk_token));
	tokens[n_tokens].offset = len;
	tokens[n_tokens++].color = (struct nk_color){0xff, 0xff, 0xff, 0xff};

	enc_delete_buffer(buf, scanner);
	enclex_destroy(scanner);

	// to please compiler
	(void)offset0;
	(void)input;
	(void)yyunput;

	return tokens;
}
