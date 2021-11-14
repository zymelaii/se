#include <se/token.h>
#include <string.h>

void reset_token(token_t *pt)
{
	if (pt != 0L)
	{
		memset(pt, 0, sizeof(token_t));
		pt->type = T_NULL;
	}
}

int predict(const char *s)
{
	if (s == 0L) return T_INVALID;

	const char c = s[0];

	if (c == '\0')
	{
		return T_NULL;
	}

	if (c == '\r' || c == '\n' || c == '\t' || c == ' ')
	{
		return T_INDENT;
	}

	if (c >= '0' && c <= '9' || c == '.')
	{
		return T_NUMBER;
	}

	if (c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_')
	{
		return T_SYMBOL;
	}

	const char *op_charset = ",;()[]{}+-%*/&|^~><=!", *p = op_charset;
	do {
		if (c == *p) return T_OPERATOR;
	} while (*++p != '\0');

	return T_INVALID;
}

size_t get_invalid(const char *s, token_t *pt)
{
	int code = (unsigned char)s[0];

	pt->p        = s;
	pt->q        = s;
	pt->r        = s + 1;
	pt->type     = T_INVALID;
	pt->sub_type = code > 128 ? T_INV_UNICODE : T_INV_UNKNOWN;

	return 1;
}

size_t get_null(const char *s, token_t *pt)
{
	int isghost = s[0] != '\0';

	pt->p        = s;
	pt->q        = s;
	pt->r        = isghost ? s + 1 : s;
	pt->type     = T_NULL;
	pt->sub_type = isghost ? T_NUL_GHOST : T_NUL_END;

	return isghost ? 1 : 0;
}

size_t get_indent(const char *s, token_t *pt)
{
#define T1(c) ((c) == '\r' || (c) == '\n')
#define T2(c) ((c) == '\t')
#define T3(c) ((c) == ' ')
#define T4(c) (T1(c) || T2(c) || T3(c))

	const char *p = s, *q = p;
	if (!T4(*p))
	{
		reset_token(pt);
		return 0;
	}

	size_t len = 0;

	switch (*q)
	{
		case '\r': 
		case '\n': do { ++len; } while ((++q, T1(*q)));
		           pt->sub_type = T_IDT_NEWLINE; break;
		case '\t': do { ++len; } while ((++q, T2(*q)));
		           pt->sub_type = T_IDT_TAB; break;
		case ' ':  do { ++len; } while ((++q, T3(*q)));
		           pt->sub_type = T_IDT_BLANK; break;
	}

	pt->p    = p;
	pt->q    = q - 1;
	pt->r    = q;
	pt->type = T_INDENT;

	return len;

#undef T1
#undef T2
#undef T3
#undef T4
}

size_t get_number(const char *s, token_t *pt)
{
#define T1(c) ((c) == '0' || (c) >= '1' && (c) <= '9' || (c) == '.') // prefix
#define T2(c) ((c) == '0' || (c) == '1')                             // binary
#define T3(c) ((c) >= '0' && (c) <= '7')                             // octal
#define T4(c) ((c) >= '0' && (c) <= '9')                             // decimal
#define T5(c) ((c) >= 'a' && (c) <= 'f' || (c) >= 'A' && (c) <= 'F') // hexadecimal (part)
#define T6(c) (T4(c) || T5(c))                                       // hexadecimal
#define T7(c) ((c) == 'e' || (c) == 'E')                             // EEEE mark
#define T8(c) ((c) == '+' || (c) == '-')                             // EEEE sign

	const char *p = s, *q = p;

	if (!T1(*p))
	{
		reset_token(pt);
		return 0;
	}

	size_t len = 0;

_entry:
	if (*q == '0') goto _state_prefix_0;
	if (*q >= '1' && *q <= '9') goto _state_prefix_1to9;
	if (*q == '.') goto _state_prefix_float_dot;
	goto _error;

_state_prefix_0:
	++len, ++q;
	if (*q == '.') goto _state_infix_suffix_float_dot;
	if (*q == 'b') goto _state_binary_infix;
	if (*q == 'x') goto _state_hexadecimal_infix;
	if (T3(*q)) goto _state_oct_number_body;
	if (T7(*q)) goto _state_eeee_mark;

	pt->sub_type = T_NUM_DEC;
	goto _done;

_state_prefix_1to9:
	++len, ++q;
	if (*q == '.') goto _state_infix_suffix_float_dot;
	if (T4(*q)) goto _state_dec_number_body;
	if (T7(*q)) goto _state_eeee_mark;

	pt->sub_type = T_NUM_DEC;
	goto _done;

_state_prefix_float_dot:
	++len, ++q;
	if (T4(*q)) goto _state_float_body;
	if (T7(*q)) goto _state_eeee_mark;
	goto _error;

_state_infix_suffix_float_dot:
	++len, ++q;
	if (T4(*q)) goto _state_float_body;
	if (T7(*q)) goto _state_eeee_mark;

	pt->sub_type = T_NUM_FLT;
	goto _done;

_state_binary_infix:
	++len, ++q;
	if (!T2(*q)) goto _error;
	goto _state_bin_number_body;

_state_hexadecimal_infix:
	++len, ++q;
	if (!T6(*q)) goto _error;
	goto _state_hex_number_body;

_state_bin_number_body:
	++len;
	while ((++q, T2(*q)))
	{
		++len;
	}

	pt->sub_type = T_NUM_BIN;
	goto _done;

_state_oct_number_body:
	++len;
	while ((++q, T3(*q)))
	{
		++len;
	}

	pt->sub_type = T_NUM_OCT;
	goto _done;

_state_dec_number_body:
	++len;
	while ((++q, T4(*q)))
	{
		++len;
	}
	if (*q == '.') goto _state_infix_suffix_float_dot;
	if (T7(*q)) goto _state_eeee_mark;

	pt->sub_type = T_NUM_DEC;
	goto _done;

_state_hex_number_body:
	++len;
	while ((++q, T6(*q)))
	{
		++len;
	}

	pt->sub_type = T_NUM_HEX;
	goto _done;

_state_float_body:
	++len;
	while ((++q, T4(*q)))
	{
		++len;
	}
	if (T7(*q)) goto _state_eeee_mark;

	pt->sub_type = T_NUM_FLT;
	goto _done;

_state_eeee_mark:
	++len, ++q;
	if (T8(*q)) goto _state_eeee_sign;
	if (T4(*q)) goto _state_eeee_body;
	goto _error;

_state_eeee_sign:
	++len, ++q;
	if (T4(*q)) goto _state_eeee_body;
	goto _error;

_state_eeee_body:
	++len;
	while ((++q, T4(*q)))
	{
		++len;
	}

	pt->sub_type = T_NUM_EEEE;
	goto _done;

_error:
	reset_token(pt);
	pt->p    = p;
	pt->r    = q;
	pt->type = T_NUMBER;
	return 0;

_done:
	pt->p    = p;
	pt->q    = q - 1;
	pt->r    = q;
	pt->type = T_NUMBER;

	return len;

#undef T1
#undef T2
#undef T3
#undef T4
#undef T5
#undef T6
#undef T7
#undef T8
}

size_t get_symbol(const char *s, token_t *pt)
{
#define T1(c) ((c) >= 'a' && (c) <= 'z' || (c) >= 'A' && (c) <= 'Z' || (c) == '_')
#define T2(c) (T1(c) || (c) >= '0' && (c) <= '9')

	const char *p = s, *q = p;

	if (!T1(*p))
	{
		reset_token(pt);
		return 0;
	}

	size_t len = 1;
	while ((++q, T2(*q)))
	{
		++len;
	}

	pt->p        = p;
	pt->q        = q - 1;
	pt->r        = q;
	pt->type     = T_SYMBOL;
	pt->sub_type = 0;

	return len;

#undef T1
#undef T2
}

size_t get_operator(const char *s, token_t *pt)
{
#define T1(c) ((c) == '(' || (c) == '[' || (c) == '{')
#define T2(c) ((c) == ')' || (c) == ']' || (c) == '}')
#define T3(c) (T1(c) || T2(c))
#define T4(c) ((c) == '+' || (c) == '-' || (c) == '%' || (c) == '*' || (c) == '/')
#define T5(c) ((c) == '=' || (c) == '>' || (c) == '<' || (c) == '!')
#define T6(c) ((c) == '&' || (c) == '|' || (c) == '^' || (c) == '~')
#define T7(c) ((c) == ',' || (c) == ';' || T3(c) || T4(c) || T5(c) || T6(c))

	const char *p = s, *q = p;

	if (!T7(*p))
	{
		reset_token(pt);
		return 0;
	}

	size_t len = 1;

_entry:
	switch (*q++)
	{
		case ',': pt->sub_type = T_OP_COMMA; break;
		case ';': pt->sub_type = T_OP_SEMICOLON; break;
		case '(': pt->sub_type = T_OP_BRACKET0L; break;
		case ')': pt->sub_type = T_OP_BRACKET0R; break;
		case '[': pt->sub_type = T_OP_BRACKET1L; break;
		case ']': pt->sub_type = T_OP_BRACKET1R; break;
		case '{': pt->sub_type = T_OP_BRACKET2L; break;
		case '}': pt->sub_type = T_OP_BRACKET2R; break;
		case '+': if (*q == '=') goto _state_compound_eq;
		          pt->sub_type = T_OP_ADD; break;
		case '-': if (*q == '=') goto _state_compound_eq;
		          pt->sub_type = T_OP_SUB; break;
		case '%': if (*q == '=') goto _state_compound_eq;
		          pt->sub_type = T_OP_MOD; break;
		case '*': if (*q == '=') goto _state_compound_eq;
                  pt->sub_type = T_OP_MUL; break;
		case '/': if (*q == '=') goto _state_compound_eq;
		          pt->sub_type = T_OP_DIV; break;
		case '=': if (*q == '=') goto _state_compound_eq;
		          pt->sub_type = T_OP_ASS; break;
		case '>': if (*q == '=') goto _state_compound_eq;
		          if (*q == '>') goto _state_repeat_op;
		          pt->sub_type = T_OP_GTR; break;
		case '<': if (*q == '=') goto _state_compound_eq;
		          if (*q == '<') goto _state_repeat_op;
		          pt->sub_type = T_OP_LSS; break;
		case '!': if (*q == '=') goto _state_compound_eq;
		          pt->sub_type = T_OP_NOT; break;
		case '&': if (*q == '=') goto _state_compound_eq;
                  if (*q == '&') goto _state_repeat_op;
		          pt->sub_type = T_OP_AND; break;
		case '|': if (*q == '=') goto _state_compound_eq;
		          if (*q == '|') goto _state_repeat_op;
		          pt->sub_type = T_OP_OR; break;
		case '^': if (*q == '=') goto _state_compound_eq;
		          pt->sub_type = T_OP_XOR; break;
		case '~': pt->sub_type = T_OP_NOR; break;
	}
	goto _done;

_state_compound_eq:
	++len, ++q;
	switch (*(q - 2))
	{
		case '+': pt->sub_type = T_OP_ADD_ASS; break;
		case '-': pt->sub_type = T_OP_SUB_ASS; break;
		case '%': pt->sub_type = T_OP_MOD_ASS; break;
		case '*': pt->sub_type = T_OP_MUL_ASS; break;
		case '/': pt->sub_type = T_OP_DIV_ASS; break;
		case '&': pt->sub_type = T_OP_AND_ASS; break;
		case '|': pt->sub_type = T_OP_OR_ASS; break;
		case '^': pt->sub_type = T_OP_XOR_ASS; break;
		case '>': pt->sub_type = T_OP_GEQ; break;
		case '<': pt->sub_type = T_OP_LEQ; break;
		case '!': pt->sub_type = T_OP_NEQ; break;
		case '=': pt->sub_type = T_OP_EQU; break;
	}
	goto _done;

_state_repeat_op:
	++len;
	switch (*q++)
	{
		case '&': pt->sub_type = T_OP_CMP_AND; break;
		case '|': pt->sub_type = T_OP_CMP_OR; break;
		case '>': if (*q == '=') { ++len, ++q; pt->sub_type = T_OP_RSH_ASS; }
		          else pt->sub_type = T_OP_RSH; break;
		case '<': if (*q == '=') { ++len, ++q; pt->sub_type = T_OP_LSH_ASS; }
		          else pt->sub_type = T_OP_LSH; break;
	}
	goto _done;

_done:
	pt->p    = p;
	pt->q    = q - 1;
	pt->r    = q;
	pt->type = T_OPERATOR;

	return len;

#undef T1
#undef T2
#undef T3
#undef T4
#undef T5
#undef T6
#undef T7
}

fn_getter se_getter(int type)
{
	switch (type)
	{
		case T_INVALID:  return get_invalid;
		case T_NULL:     return get_null;
		case T_INDENT:   return get_indent;
		case T_NUMBER:   return get_number;
		case T_SYMBOL:   return get_symbol;
		case T_OPERATOR: return get_operator;
		default: return 0L;
	}
}