#pragma once

#include <se/token.h>
#include <se/alloc.h>
#include <se/type.h>
#include <se/priority.h>
#include <se/exception.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>

// enum for tokstate_t.status
#define ETS_NULL      0x00 // initial status
#define ETS_TOKEN     0x81 // get one token
#define ETS_STATEMENT 0x82 // complete one statement
#define ETS_ALLDONE   0x83 // complete all
#define ETS_IGNORE    0x84 // catch blanks
#define ETS_BADCH     0x41 // catch bad character
#define ETS_NAN       0x42 // catch NaN number
#define ETS_NOSEP     0x43 // expect seperator between 2 tokens
#define ETS_SYMLEN    0x44 // length of a symbol token is too big (more than 32 chs)

typedef struct tokstate_s
{
	uint32_t status;
	token_t token;
	token_t prev;
	size_t row, col;
} tokstate_t;

typedef struct unit_s
{	// { extra: 4, major_type: 4, sub_type: 8 }
	uint16_t type;
	uint16_t len;
	const char *tok;
} unit_t;

typedef struct scopestate_s
{	// scope state of bracket for seus
	int sframe;
	int accept;
} scopestate_t;

typedef struct seus_s
{	// Simple Eval Unit Stream
	uint16_t     nef;
	uint16_t     nvf;
	uint16_t     nss;
	uint16_t     nus;
	scopestate_t *ss;
	unit_t       *us;
} seus_t;

#define SE_UNIT_TYPE(e) ((e).type >> 8 & 0xf)
#define SE_UNIT_SUBTYPE(e) ((e).type & 0xff)

static inline unit_t tok2unit(token_t token)
{
	unit_t ret = {
		.type = (uint16_t)((token.type & 0xf) << 8 | (token.sub_type & 0xff)),
		.len  = (uint16_t)(token.r - token.p),
		.tok  = token.p,
	};
	return ret;
}

static inline token_t unit2tok(unit_t unit)
{
	token_t ret = {
		.p        = unit.tok,
		.q        = unit.tok + unit.len - 1,
		.r        = unit.tok + unit.len,
		.type     = SE_UNIT_TYPE(unit),
		.sub_type = SE_UNIT_SUBTYPE(unit),
	};
	return ret;
}

#ifdef __cplusplus
extern "C" {
#endif

void reset_tokstate(tokstate_t *state);

const char* next_token(
	const char *str, token_t *token, tokstate_t *state
); // get one token each time

const char* str2tokens(
	const char *str, token_t **ptoks, int *psize
); // if available, get tokens of one statement

// step 1 compile, convert tokens to Reverse Polish notion
unit_t* toks2rpn(token_t *tokens, int ntok, int *psize);

// step 2 compile, convert Reverse Polish notion to Simple Eval Unit Stream
seus_t rpn2seus(unit_t *units, int n);
void free_seus(seus_t *seus);

#ifdef __cplusplus
}
#endif