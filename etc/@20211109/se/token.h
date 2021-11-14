#pragma once

#include <stdint.h>

#define T_INVALID  0
#define T_NULL     1 // '\0'
#define T_INDENT   2 // '\r','\n','\t',' '
#define T_NUMBER   3
#define T_SYMBOL   4
#define T_OPERATOR 5

#define T_INV_UNKNOWN 1 // unsupport visiable ascii characters
#define T_INV_UNICODE 2 // unicode characters

#define T_NUL_END     1 // reach '\0'
#define T_NUL_GHOST   2 // reach an unreal null, not the end actually

#define T_IDT_BLANK   1 // ' '
#define T_IDT_TAB     2 // '\t'
#define T_IDT_NEWLINE 3 // '\n','\r',"\r\n"

#define T_OP_COMMA     1 // ,
#define T_OP_SEMICOLON 2 // ;
#define T_OP_BRACKET0L 3 // (
#define T_OP_BRACKET0R 4 // )
#define T_OP_BRACKET1L 5 // [
#define T_OP_BRACKET1R 6 // ]
#define T_OP_BRACKET2L 7 // {
#define T_OP_BRACKET2R 8 // }

#define T_OP_ADD       9 // +
#define T_OP_SUB      10 // -
#define T_OP_MOD      11 // %
#define T_OP_MUL      12 // *
#define T_OP_DIV      13 // /
#define T_OP_ADD_ASS  14 // +=
#define T_OP_SUB_ASS  15 // -=
#define T_OP_MOD_ASS  16 // %=
#define T_OP_MUL_ASS  17 // *=
#define T_OP_DIV_ASS  18 // /=

#define T_OP_AND      19 // &
#define T_OP_OR       20 // |
#define T_OP_XOR      21 // ^
#define T_OP_LSH      22 // <<
#define T_OP_RSH      23 // >>
#define T_OP_NOT      24 // !
#define T_OP_NOR      25 // ~
#define T_OP_AND_ASS  26 // &=
#define T_OP_OR_ASS   27 // |=
#define T_OP_XOR_ASS  28 // ^=
#define T_OP_LSH_ASS  29 // <<=
#define T_OP_RSH_ASS  30 // >>=

#define T_OP_GTR      31 // >
#define T_OP_LSS      32 // <
#define T_OP_LEQ      33 // <=
#define T_OP_GEQ      34 // >=
#define T_OP_EQU      35 // ==
#define T_OP_NEQ      36 // !=
#define T_OP_CMP_AND  37 // &&
#define T_OP_CMP_OR   38 // ||

#define T_OP_ASS      39 // =

#define T_NUM_BIN  1 // binary
#define T_NUM_OCT  2 // octal
#define T_NUM_DEC  3 // decimal
#define T_NUM_HEX  4 // hexadecimal
#define T_NUM_FLT  5 // float
#define T_NUM_EEEE 6 // scientific notation of float

typedef struct token_s
{
	const char *p; // first position of token, to be null of type T_NULL/T_INVALID
	const char *q; // last position of token, to be null of type T_NULL/T_INVALID
	const char *r; // pointer to end of search, to be null defaultly
	int type;      // major type of token
	int sub_type;  // subtype of token
} token_t;

typedef size_t (*fn_getter)(const char*, token_t*);

#ifdef __cplusplus
extern "C" {
#endif

void reset_token(token_t *pt);
int  predict(const char *s);

size_t get_invalid (const char *s, token_t *pt);
size_t get_null    (const char *s, token_t *pt);
size_t get_indent  (const char *s, token_t *pt);
size_t get_number  (const char *s, token_t *pt);
size_t get_symbol  (const char *s, token_t *pt);
size_t get_operator(const char *s, token_t *pt);

fn_getter se_getter(int type);

#ifdef __cplusplus
}
#endif