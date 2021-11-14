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

		se_throw(SyntaxError, BeyondCharset,
			(uint64_t)state->row << 32 | (uint32_t)state->col, 0);

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
		if (token->q == 0L)
		{
			state->status = ETS_NAN;
			se_throw(SyntaxError, InvalidNumberLiteral,
				(uint64_t)state->row << 32 | (uint32_t)state->col, 0);
		} else
		{
			if (state->token.type == T_NUMBER
				|| state->token.type == T_SYMBOL
				|| state->token.type == T_NULL
				&& (state->prev.type == T_NUMBER
				|| state->prev.type == T_SYMBOL))
			{
				state->status = ETS_NOSEP;
				se_throw(SyntaxError, ExpectSeperator,
					(uint64_t)state->row << 32 | (uint32_t)state->col, 0);
			} else
			{
				state->status = ETS_TOKEN;
			}
		}
		state->col += len;
	}

	if (token->type == T_SYMBOL)
	{
		const int max_symlen = 32;
		if (len > max_symlen)
		{
			state->status = ETS_SYMLEN;
			se_throw(SyntaxError, SymbolTooLong,
				(uint64_t)state->row << 32 | (uint32_t)state->col, 0);
		} else
		{
			if (state->token.type == T_NUMBER
				|| state->token.type == T_SYMBOL
				|| state->token.type == T_NULL
				&& (state->prev.type == T_NUMBER
				|| state->prev.type == T_SYMBOL))
			{
				state->status = ETS_NOSEP;
				se_throw(SyntaxError, ExpectSeperator,
					(uint64_t)state->row << 32 | (uint32_t)state->col, 0);
			} else
			{
				state->status = ETS_TOKEN;
			}
		}
		state->col += len;
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
			se_exception_t exception;
			if (se_catch_any(&exception))
			{
				se_throw(exception.etype, exception.error,
					exception.extra, errors);
			}
			continue;
		}

		if (state.status & 0x80 && errors == 0)
		{
			if (token.type == T_OPERATOR)
			{
				int op = tok2op(token.sub_type);
				int unary = state.prev.type == T_NULL
					|| state.prev.type == T_OPERATOR
					&& state.prev.sub_type != T_OP_BRACKET0R
					&& state.prev.sub_type != T_OP_BRACKET1R;
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

// 将TOKENS转换为逆波兰表达式
unit_t *toks2rpn(token_t *tokens, int ntok, int *psize)
{
	assert(psize != 0L);

	if (tokens == 0L || ntok <= 0)
	{
		se_throw(UnknownError, ArgumentErrorInCSrc, 0, 0);
		*psize = 0;
		return 0L;
	}

	se_exception_t last_exception = { 0 };
	se_catch_any(&last_exception);

	int size = ntok, i = 0;
	unit_t *rp = se_alloc(size * sizeof(unit_t));
	assert(rp != 0L);

	int p = -1;   // pointer to RPN
	int q = size; // pointer to stack

	for (i = 0; i < ntok; ++i)
	{
		token_t tok = tokens[i];
		if (tok.type != T_OPERATOR)
		{	// 为数字或符号，直接输出
			rp[++p] = tok2unit(tok);
		} else if (tok.sub_type & 0x40)
		{	// 为左括号，符号输出并入栈
			rp[++p] = rp[--q] = tok2unit(tok);
			if (tok.sub_type == OP_BRE_S && i > 0)
			{	// 推断是否为参数列表操作符
				int type     = tokens[i - 1].type;
				int sub_type = tokens[i - 1].sub_type;
				if (type == T_SYMBOL || type == T_OPERATOR
					&& (sub_type == OP_BRE_E || sub_type == OP_IDX_E))
				{	// 圆括号前为符号，推断变易
					// 圆括号前为圆括号或方括号，推断变易
					rp[p].type = rp[q].type &= 0xff00;
					rp[p].type = rp[q].type |= OP_ARG_S;
				}
			}
		} else if (tok.sub_type & 0x80)
		{	// 为右括号，出栈所有符号直到匹配左括号
			token_t tok3  = { 0 };
			tok3.type     = T_OPERATOR;
			tok3.sub_type = tok.sub_type &~ 0x80;
			if (q == size)
			{	// 栈中无内容，左括号不存在，终止解析
				se_throw(SyntaxError, NoLeftBracket, tok3.sub_type, 0);
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
					// RPN指针与符号栈指针重叠，此次不位移
					if (++p != q)
					{
						rp[p] = tok2unit(tok3);
					}
					break;
				} else if (tok2.sub_type & 0x40)
				{	// 为其他的左括号，括号交叉，为非法表达式，终止解析
					se_throw(SyntaxError, CrossedBrackets,
						(uint64_t)(tok2.sub_type &~ 0x40) << 32 | (uint32_t)tok3.sub_type, 0);
					break;
				} else
				{	// 为其他操作符，出栈输出
					if (++p != q)
					{
						rp[p] = tok2unit(tok2);
					}
				}
				if (q == size)
				{	// 左括号不存在，为非法表达式，终止解析
					se_throw(SyntaxError, NoLeftBracket, tok3.sub_type, 0);
					break;
				}
			} while (1);
		} else while (1)
		{	// 其他操作符，按优先级与结合性输出栈中符号
			if (q == size)
			{	// 栈中为空，操作符入栈
				rp[--q] = tok2unit(tok);
				break;
			}
			token_t tok2  = unit2tok(rp[q]);
			int priority  = get_priority(tok.sub_type);
			int priority2 = get_priority(tok2.sub_type);
			if (tok2.sub_type & 0x40)
			{	// 遇到左括号，操作符入栈
				// 栈中符号不可能为右括号
				rp[--q] = tok2unit(tok);
				break;
			} else if (priority > priority2)
			{	// 高优先级操作符出栈输出
				if (++p != ++q)
				{
					rp[p] = tok2unit(tok2);
				}
			} else if (priority == priority2 && get_associativity(tok2.sub_type))
			{	// 同等优先级按结合性出栈
				// 结合性为自左向右，操作符出栈输出
				if (++p != ++q)
				{
					rp[p] = tok2unit(tok2);
				}
			} else
			{	// 其余情况符号入栈
				rp[--q] = tok2unit(tok);
				break;
			}
		}
		if (!se_caught()) break;
	}

	if (se_caught()) 
	{
		while (q != size)
		{
			token_t tok = unit2tok(rp[q++]);
			if (tok.sub_type & 0x40)
			{	// 未出栈的符号中包含左括号，为非法表达式
				se_throw(SyntaxError, NoRightBracket, tok.sub_type &~ 0x40, 0);
				break;
			}
			if (++p < q)
			{
				rp[p] = tok2unit(tok);
			}
		}
	}

	if (!se_caught())
	{
		se_free(rp);
		rp     = 0L;
		*psize = 0;
	} else
	{
		*psize = size;
		se_throw(last_exception.etype, last_exception.error,
			last_exception.extra, last_exception.reserved);
	}

	return rp;
}

// rpn2seus进一步处理从tok2rpn得到的结果，保证括号是成对出现的
// 在该步骤中，检查模拟语句执行中的栈情况
seus_t rpn2seus(unit_t *units, int n)
{
	seus_t seus = { 0 };

	se_exception_t last_exception = { 0 };
	se_catch_any(&last_exception);

	if (units == 0L || n <= 0)
	{
		goto _complete;
	}

	int ef = -1, nef = ef; // 入栈的元素帧 element frame
	int vf = -1, nvf = vf; // 弹出元素的值帧 moved value frame

	scopestate_t *ss; // 括号域状态
	int p = -1, nss = 1;

	int i = 0;
	do {
		if ((units[i].type >> 8 & 0xf) == T_OPERATOR)
		switch (units[i].type & 0xff)
		{
			case OP_BRE:
			case OP_ARG:
			case OP_IDX:
			case OP_ARR: ++nss;
		}
	} while (++i < n);

	ss = (scopestate_t*)se_alloc(nss * sizeof(scopestate_t));
	assert(ss != 0L);

	ss[++p] = (scopestate_t){ ef + 1, 0 };
	nss = 1; // 用于确定ss的真实最大长度	

	for (i = 0; i < n; ++i)
	{
		unit_t *e = units + i;
		if ((e->type >> 8 & 0xf) == T_OPERATOR)
		switch (e->type & 0xff)
		{
			case OP_BRE_S:
			case OP_ARG_S:
			case OP_IDX_S:
			case OP_ARR_S:
			{	// 压入新的括号域
				ss[++p] = (scopestate_t){ ef + 1, 0 };
				if (p >= nss)
				{
					nss = p + 1;
				}
			}
			continue;
			case OP_BRE:
			case OP_ARG:
			case OP_IDX:
			case OP_ARR:
			{
				int nele;

				if (ef > ss[p].sframe)
				{
					se_throw(SyntaxError, MissingComma, i, 0);
					goto _error;
				} else if (ef < ss[p].sframe)
				{
					if (ss[p].accept > 0)
					{
						se_throw(SyntaxError, TooManyCommas, i, 0);
						goto _error;
					} else
					{
						nele = 0;
					}
				} else if (ss[p].accept > 0)
				{
					nele = ss[p].accept + 1;
				} else
				{
					nele = 1;
				}

				int op = e->type & 0xff;

				if (nele == 0)
				{
					if (op == OP_IDX)
					{
						se_throw(IndexError, NoIndex, i, 0);
						goto _error;
					} else
					{
						++ef;
						if (ef > nef)
						{
							nef = ef;
						}
					}
				} else
				{
					vf -= ss[p].accept;
					if (vf < -1)
					{
						se_throw(SyntaxError, InvalidSyntax, i, 0);
						goto _error;
					}
				}

				if (op == OP_IDX)
				{
					--ef;
					if (ef < 0)
					{
						se_throw(IndexError, MissingArray, i, 0);
						goto _error;
					}
				}

				if (op == OP_ARG)
				{
					--ef;
					if (ef < 0)
					{
						se_throw(RuntimeError, ExpectFunction, i, 0);
						goto _error;
					}
				}

				--p;

				if (p < 0)
				{
					se_throw(SyntaxError, InvalidSyntax, i, 0);
					goto _error;
				}
			}
			continue;
			case OP_CME:
			{
				if (ef >= ss[p].sframe)
				{
					++ss[p].accept;
					--ef;
					++vf;
					if (vf > nvf)
					{
						nvf = vf;
					}
				} else
				{
					se_throw(SyntaxError, TooManyCommas, i, 0);
					goto _error;
				}
			}
			continue;
			case OP_PL:
			case OP_NL:
			case OP_EPA: // uncertain
			case OP_NOT:
			case OP_LNOT:
			{
				if (ef < ss[p].sframe)
				{
					se_throw(SyntaxError, MissingOperand, i, 0);
					goto _error;
				}
			}
			continue;
			default:
			{
				--ef;
				if (ef < ss[p].sframe)
				{
					se_throw(SyntaxError, MissingOperand, i, 0);
					goto _error;
				}
			}
			continue;
		}

		++ef;
		if (ef > nef)
		{
			nef = ef;
		}
	}

	if (ef != 0)
	{
		se_throw(SyntaxError, InvalidSyntax, n, ef);
		goto _error;
	}

_done:
	seus = (seus_t){
		.nef = nef > 0 ? nef : 0,
		.nvf = nvf > 0 ? nvf : 0,
		.nss = nss,
		.nus = n,
		.ss  = (scopestate_t*)se_alloc(nss * sizeof(scopestate_t)),
		.us  = units,
	};
	assert(seus.ss != 0L);
	goto _clean;

_error:
	seus.nus = n;
	seus.us  = units;
	goto _clean;

_clean:
	se_free(ss);
	goto _complete;

_complete:
	if (se_caught())
	{
		se_throw(last_exception.etype, last_exception.error,
			last_exception.extra, last_exception.reserved);
	}
	return seus;
}

void free_seus(seus_t *seus)
{
	if (seus != 0L)
	{
		if (seus->us != 0L)
		{
			se_free(seus->us);
		}
		if (seus->ss != 0L)
		{
			se_free(seus->ss);
		}
		memset(seus, 0, sizeof(seus_t));
	}
}