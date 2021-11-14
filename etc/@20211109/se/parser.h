#pragma once

#include <se/token.h>
#include <se/alloc.h>
#include <se/type.h>
#include <se/priority.h>
#include <stdint.h>

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
{
	// { extra: 4, major_type: 4, sub_type: 8 }
	uint16_t type;
	int16_t  depth;
	union {
		uint32_t len;
		int32_t  id;
	};
	union {
		const char *tok;
		const char *symbol;
	};
} unit_t;

static inline unit_t tok2unit(token_t token, int depth)
{
	unit_t ret = {
		.type  = (uint16_t)((token.type & 0xf) << 8 | (token.sub_type & 0xff)),
		.depth = (int16_t)depth,
		.len   = (uint32_t)(token.r - token.p),
		.tok   = token.p,
	};
	return ret;
}

static inline token_t unit2tok(unit_t unit)
{
	token_t ret = {
		.p        = unit.tok,
		.q        = unit.tok + unit.len - 1,
		.r        = unit.tok + unit.len,
		.type     = (unit.type >> 8) & 0xf,
		.sub_type = unit.type & 0xff,
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

unit_t* build_rpn(token_t *tokens, int ntok, int *psize);

#ifdef __cplusplus
}
#endif