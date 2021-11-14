#include <se/parser.h>
#include <se/token.h>
#include <se/alloc.h>
#include <string.h>
#include <assert.h>

void reset_tokstate(tokstate_t *state)
{
	if (state != 0L)
	{
		memset(state, 0, sizeof(tokstate_t));
		state->status         = ETS_NULL;
		state->token.type     = T_NULL;
		state->token.sub_type = T_NUL_GHOST;
		state->prev.type      = T_NULL;
		state->prev.sub_type  = T_NUL_GHOST;
		state->row            = 1;
		state->col            = 1;
	}
}

const char* next_token(const char *str, token_t *token, tokstate_t *state)
{
	assert(str   != 0L);
	assert(token != 0L);
	assert(state != 0L);

	int toktype = predict(str);
	se_getter(toktype)(str, token);

	if (token->type == T_NULL)
	{
		state->status = ETS_ALLDONE;
		state->prev   = state->token;
		state->token  = *token;

		return 0L;
	}

	if (token->type == T_INVALID)
	{
		state->status = ETS_BADCH;
		state->prev   = state->token;
		reset_token(&state->token);

		++state->col;

		return token->r;
	}

	int len = token->r - token->p;

	if (token->type == T_INDENT)
	{
		state->status = ETS_IGNORE;
		if (state->token.type != T_NULL)
		{
			state->prev = state->token;
		}
		reset_token(&state->token);

		switch (token->sub_type)
		{
			case T_IDT_BLANK:
			{
				state->col += len;
			}
			break;
			case T_IDT_TAB:
			{
				const int tab_width = 4;
				state->col += len * tab_width;
			}
			break;
			case T_IDT_NEWLINE:
			{
				if (token->p[0] == '\r' && token->p[1] == '\n')
				{
					len >>= 1;
				}
				state->row += len;
				state->col  = 1;
			}
			break;
		}

		return token->r;
	}

	if (token->type == T_NUMBER)
	{
		state->col += len;
		if (token->q == 0L)
		{
			state->status = ETS_NAN;
		} else
		{
			if (state->token.type == T_NUMBER
				|| state->token.type == T_SYMBOL
				|| state->token.type == T_NULL
				&& (state->prev.type == T_NUMBER
				|| state->prev.type == T_SYMBOL))
			{
				state->status = ETS_NOSEP;
			} else
			{
				state->status = ETS_TOKEN;
			}
		}
	}

	if (token->type == T_SYMBOL)
	{
		state->col += len;
		const int max_symlen = 32;
		if (len > max_symlen)
		{
			state->status = ETS_SYMLEN;
		} else
		{
			if (state->token.type == T_NUMBER
				|| state->token.type == T_SYMBOL
				|| state->token.type == T_NULL
				&& (state->prev.type == T_NUMBER
				|| state->prev.type == T_SYMBOL))
			{
				state->status = ETS_NOSEP;
			} else
			{
				state->status = ETS_TOKEN;
			}
		}
	}

	if (token->type == T_OPERATOR)
	{
		state->col += len;
		if (token->sub_type == T_OP_SEMICOLON)
		{
			state->status = ETS_STATEMENT;
		} else
		{
			state->status = ETS_TOKEN;
		}
	}

	if (state->token.type != T_NULL)
	{
		state->prev = state->token;
	}
	state->token = *token;

	return token->r;
}

const char* str2tokens(const char *str, token_t **ptoks, int *psize)
{
	assert(str   != 0L);
	assert(ptoks != 0L);
	assert(psize != 0L);

	int n = 0, i = 0, errors = 0;
	const char *p = str;

	while (*p != ';' && *p != '\0')
	{
		++n;
		++p;
	}

	token_t *tokens = (token_t*)se_alloc(n * sizeof(token_t));
	assert(tokens != 0L);

	token_t    token;
	tokstate_t state;
	reset_tokstate(&state);

	p = str;

	while ((p = next_token(p, &token, &state)) != 0L)
	{
		if (state.status == ETS_STATEMENT) break;
		if (state.status == ETS_IGNORE) continue;

		if (state.status & 0x40)
		{
			++errors;
			continue;
		}

		if (state.status & 0x80 && errors == 0)
		{
			if (token.type == T_OPERATOR)
			{
				int op = tok2op(token.sub_type);
				int unary = state.prev.type == T_NULL
					|| state.prev.type == T_OPERATOR
					&& state.prev.sub_type != T_OP_BRACKET0R;
				switch (token.sub_type)
				{
					case T_OP_ADD: op = unary ? OP_PL  : op; break;
					case T_OP_SUB: op = unary ? OP_NL  : op; break;
					case T_OP_MUL: op = unary ? OP_EPA : op; break;
				}
				token.sub_type = op;
			}
			tokens[i++] = token;
		}
	}

	if (errors > 0 || i == 0)
	{
		*psize = 0;
		*ptoks = 0L;
	} else
	{
		*psize = i;
		*ptoks = (token_t*)se_alloc(i * sizeof(token_t));
		memcpy(*ptoks, tokens, i * sizeof(token_t));
	}

	se_free(tokens);

	return p;
}

unit_t* build_rpn(token_t *tokens, int ntok, int *psize)
{
	assert(psize != 0L);

	if (tokens == 0L || ntok <= 0) return 0L;

	int size = ntok, i = 0, error = 0;
	do {
		if (tokens[i].sub_type & 0x40)
		{
			--size;
		}
	} while (++i < ntok);

	unit_t *rp = se_alloc(size * sizeof(unit_t));
	assert(rp != 0L);

	int p     = -1;   // pointer to RPN
	int q     = size; // pointer to stack
	int depth = 0;

	for (i = 0; i < ntok; ++i)
	{
		token_t tok = tokens[i];
		if (tok.type != T_OPERATOR)
		{	// 为数字或符号，直接输出
			rp[++p] = tok2unit(tok, depth);
		} else if (tok.sub_type & 0x40)
		{	// 为左括号，符号入栈
			rp[--q] = tok2unit(tok, depth);
			// 是圆括号，深度递增
			if (tok.sub_type == OP_BRE_S)
			{
				++depth;
				if (i > 0)
				{	// 推断是否为参数列表操作符
					int type     = tokens[i - 1].type;
					int sub_type = tokens[i - 1].sub_type;
					if (type == T_SYMBOL || type == T_OPERATOR
						&& (sub_type == OP_BRE_E || sub_type == OP_IDX_E))
					{	// 圆括号前为符号，推断变易
						// 圆括号前为圆括号或方括号，推断变易
						rp[q].type &= 0xff00;
						rp[q].type |= OP_ARG_S;
					}
				}
			}
		} else if (tok.sub_type & 0x80)
		{	// 为右括号，出栈所有符号直到匹配左括号
			token_t tok3  = { 0 };
			tok3.type     = T_OPERATOR;
			tok3.sub_type = tok.sub_type &~ 0x80;
			if (q == size)
			{	// 栈中无内容
				// 左括号不存在，为非法表达式，终止解析
				error = 1;
				break;
			} else do
			{
				token_t tok2 = unit2tok(rp[q++]);
				if (tok2.sub_type == OP_ARG_S && tok.sub_type == OP_BRE_E)
				{	// 为匹配的圆括号，向前括号合并为参数列表操作符
					tok3.sub_type = OP_ARG;
				}
				if (tok2.sub_type == (tok3.sub_type | 0x40))
				{	// 为相匹配的左括号，出栈输出
					// 圆括号的深度为左括号的深度
					rp[++p] = tok2unit(tok3, depth);
					if (tok.sub_type == OP_BRE_E)
					{	// 为圆括号，深度返回上一级
						--rp[p].depth;
					}
					break;
				} else if (tok2.sub_type & 0x40)
				{	// 为其他的左括号，括号交叉，为非法表达式，终止解析
					error = 1;
					break;
				} else
				{	// 为其他操作符，出栈输出
					rp[++p] = tok2unit(tok2, depth);
				}
				if (q == size)
				{	// 左括号不存在，为非法表达式，终止解析
					error = 1;
					break;
				}
			} while (1);
			// 是圆括号，深度递减
			if (tok.sub_type == OP_BRE_E) --depth;
		} else while (1)
		{	// 其他操作符
			// 按优先级与结合性输出栈中符号
			if (q == size)
			{	// 栈中为空，操作符入栈
				rp[--q] = tok2unit(tok, depth);
				break;
			}
			token_t tok2  = unit2tok(rp[q]);
			int priority  = get_priority(tok.sub_type);
			int priority2 = get_priority(tok2.sub_type);
			if (tok2.sub_type & 0x40)
			{	// 遇到左括号，操作符入栈
				// 栈中符号不可能为右括号
				rp[--q] = tok2unit(tok, depth);
				break;
			} else if (priority > priority2)
			{	// 高优先级操作符出栈输出
				rp[++p] = tok2unit(tok2, depth);
				++q;
			} else if (priority == priority2 && get_associativity(tok2.sub_type))
			{	// 同等优先级按结合性出栈
				// 结合性为自左向右，操作符出栈输出
				rp[++p] = tok2unit(tok2, depth);
				++q;
			} else
			{	// 其余情况符号入栈
				rp[--q] = tok2unit(tok, depth);
				break;
			}
		}
		if (error) break;
	}

	if (!error) while (q != size)
	{
		token_t tok = unit2tok(rp[q++]);
		if (tok.sub_type & 0x40)
		{	// 未出栈的符号中包含左括号，为非法表达式
			error = 1;
			break;
		}
		rp[++p] = tok2unit(tok, depth);
	}

	if (error)
	{
		se_free(rp);
		rp     = 0L;
		*psize = 0;
	} else
	{
		*psize = size;
	}

	return rp;
}