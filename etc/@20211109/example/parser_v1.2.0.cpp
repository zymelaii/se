#include <se/token.h>
#include <se/alloc.h>
#include <se/stack.h>
#include <se/type.h>
#include <se/priority.h>
#include <se/parser.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

std::ostream& operator<<(std::ostream &os, token_t &token);
std::ostream& print_token(token_t &token);
std::ostream& print_unit(unit_t &unit);
void parse(const std::string &str);
int  rout(const std::string &str);

int main(int argc, char const *argv[])
{
	const char *PROMPT = ">>> ";
	std::string str;

	while (true)
	{
		std::cout << PROMPT;

		std::getline(std::cin, str);

		int state = rout(str);
		if (std::cin.eof()) break;

		if (state == 0) break;
		else if (state == 1) parse(str);
		else if (state == 2) continue;
	}

	return 0;
}

void parse(const std::string &str)
{
	token_t    token;
	tokstate_t state;
	reset_tokstate(&state);

	size_t row = state.row, col = state.col;

	const char *p = str.c_str(), *q = p;
	std::vector<std::vector<token_t>> statements;
	std::vector<token_t> stack;

	bool error = false;

	while ((p = next_token(p, &token, &state)) != nullptr)
	{
		size_t i = row, j = col;
		row = state.row;
		col = state.col;

		if (state.status == ETS_STATEMENT)
		{
			if (error)
			{
				error = false;
			} else if (!stack.empty())
			{
				std::cout << "$accept: " << std::string(q, p - 1) << std::endl;
				q = p;
				statements.push_back(std::vector<token_t>(stack.begin(), stack.end()));
				stack.clear();
			}
			continue;
		}

		if (state.status == ETS_IGNORE) continue;

		if (state.status & 0x40)
		{
			error = true;
			#define ERROR(CS, ARGS...) printf("#%d:%d error: " CS, i, j, ##ARGS)
			switch (state.status)
			{
				case ETS_BADCH:
				{
					if (token.sub_type == T_INV_UNKNOWN)
					{
						ERROR("unrecognized character '%c'\n", token.p[0]);
					} else if (token.sub_type == T_INV_UNICODE)
					{
						ERROR("stray \\%03o in program\n", (uint8_t)token.p[0]);
					}
				}
				break;
				case ETS_NAN:
				{
					ERROR("parse number-like token as NaN \"");
					print_token(token) << "\"" << std::endl;
				}
				break;
				case ETS_NOSEP:
				{
					ERROR("expect seperator before \"");
					print_token(token) << "\"" << std::endl;
				}
				break;
				case ETS_SYMLEN:
				{
					ERROR("symbol \"%8s...\" was too long "
						"since expected limit is 32\n", token.p);
				}
				break;
			}
			#undef ERROR
		}

		if (state.status & 0x80)
		{
			if (token.type == T_OPERATOR)
			{
				int opid = tok2op(token.sub_type);
				bool unary = state.prev.type == T_NULL
					|| state.prev.type == T_OPERATOR
					&& state.prev.sub_type != T_OP_BRACKET0R;
				switch (token.sub_type)
				{
					case T_OP_ADD: token.sub_type = unary ? OP_PL  : opid; break;
					case T_OP_SUB: token.sub_type = unary ? OP_NL  : opid; break;
					case T_OP_MUL: token.sub_type = unary ? OP_EPA : opid; break;
					default: token.sub_type = opid;
				}
			}
			stack.push_back(token);
		}
	}

	if (error)
	{
		error = false;
	} else if (!stack.empty())
	{
		std::cout << "$accept: " << std::string(q) << std::endl;
		statements.push_back(std::vector<token_t>(stack.begin(), stack.end()));
		stack.clear();
	}

	std::cout << statements.size() << " statements in total" << std::endl;

	if (statements.size() == 0) return;

	std::cout << "#----------------" << std::endl;
	int index = 0;

	for (auto &tokens : statements)
	{
		unit_t *rp = (unit_t*)calloc(tokens.size(), sizeof(unit_t));
		int pi = -1, qi = tokens.size();

		int  depth = 0;
		error = false;

		for (int i = 0; i < tokens.size(); ++i)
		{
			token_t tok = tokens[i];
			if (tok.type != T_OPERATOR)
			{	// 为数字或符号，直接输出
				rp[++pi] = tok2unit(tok, depth);
			} else if (tok.sub_type & 0x40)
			{	// 为左括号，符号入栈
				rp[--qi] = tok2unit(tok, depth);
				// 是圆括号，深度递增
				if (tok.sub_type == OP_BRE_S) ++depth;
			} else if (tok.sub_type & 0x80)
			{	// 为右括号，出栈所有符号直到匹配左括号
				token_t tok3  = { 0 };
				tok3.type     = T_OPERATOR;
				tok3.sub_type = tok.sub_type &~ 0x80;
				if (qi == tokens.size())
				{	// 栈中无内容
					// 左括号不存在，为非法表达式，终止解析
					std::cout << "error: missing bracket" << std::endl;
					error = true;
					break;
				} else do
				{
					token_t tok2 = unit2tok(rp[qi++]);
					if (tok2.sub_type == (tok3.sub_type | 0x40))
					{	// 为相匹配的左括号，出栈输出
						// 圆括号的深度为左括号的深度
						rp[++pi] = tok2unit(tok3, depth - (tok2.sub_type == OP_BRE_S));
						break;
					} else if (tok2.sub_type & 0x40)
					{	// 为其他的左括号，括号交叉，为非法表达式，终止解析
						std::cout << "error: match crossing brackets" << std::endl;
						error = true;
						break;
					} else
					{	// 为其他操作符，出栈输出
						rp[++pi] = tok2unit(tok2, depth);
					}
					if (qi == tokens.size())
					{	// 左括号不存在，为非法表达式，终止解析
						std::cout << "error: missing bracket" << std::endl;
						error = true;
						break;
					}
				} while (true);
				// 是圆括号，深度递减
				if (tok.sub_type == OP_BRE_E) --depth;
			} else while (true)
			{	// 其他操作符
				// 按优先级与结合性输出栈中符号
				if (qi == tokens.size())
				{	// 栈中为空，操作符入栈
					rp[--qi] = tok2unit(tok, depth);
					break;
				}
				token_t tok2  = unit2tok(rp[qi]);
				int priority  = get_priority(tok.sub_type);
				int priority2 = get_priority(tok2.sub_type);
				if (tok2.sub_type & 0x40)
				{	// 遇到左括号，操作符入栈
					// 栈中符号不可能为右括号
					rp[--qi] = tok2unit(tok, depth);
					break;
				} else if (priority > priority2)
				{	// 高优先级操作符出栈输出
					rp[++pi] = tok2unit(tok2, depth);
					++qi;
				} else if (priority == priority2 && get_associativity(tok2.sub_type))
				{	// 同等优先级按结合性出栈
					// 结合性为自左向右，操作符出栈输出
					rp[++pi] = tok2unit(tok2, depth);
					++qi;
				} else
				{	// 其余情况符号入栈
					rp[--qi] = tok2unit(tok, depth);
					break;
				}
			}
			if (error) break;
		}

		if (!error) while (qi != tokens.size())
		{	// 剩余符号出栈
			token_t tok = unit2tok(rp[qi++]);
			if (tok.sub_type & 0x40)
			{	// 未出栈的符号中包含左括号，为非法表达式
				std::cout << "error: missing bracket" << std::endl;
				error = true;
				break;
			}
			rp[++pi] = tok2unit(tok, depth);
		}

		if (error)
		{	// 语句存在错误，中断解析，跳转至下一语句
			std::cout << "info: statement parsing terminate" << std::endl;
			free(rp);
			continue;
		}

		printf("[%02d] %s ->", ++index, std::string(tokens[0].p, tokens.back().r).c_str());
		for (int i = 0; i <= pi; ++i)
		{
			std::cout << " ";
			if ((rp[i].type >> 8 & 0xf) != T_OPERATOR)
			{
				print_unit(rp[i]);
			} else switch (rp[i].type & 0xff)
			{
				case OP_BRE: std::cout << "()"; break;
				case OP_IDX: std::cout << "[]"; break;
				case OP_ARR: std::cout << "{}"; break;
				default: print_unit(rp[i]);
			}
			std::cout << "@" << rp[i].depth;
		}
		std::cout << std::endl;

		free(rp);
	}

	std::cout << "#----------------" << std::endl;
}

int rout(const std::string &str)
{
	if (str == "")
	{   //! empty line -> switch to new line
		return 2;
	} else if (str == ".exit" || str == ".quit" || str == ".q")
	{   //! quit tokenizer
		return 0;
	} else if (str == ".clear" || str == ".cls" || str[0] == '\014'/*Ctrl-L*/)
	{   //! clear screen
#ifdef _WIN32
		system("cls");
#elif __linux__
		system("clear");
#endif
		return 2;
	} else if (str == ".v")
	{   //! print zipped version info
		std::cout << "SE parser v1.2.0" << std::endl;
		return 2;
	} else if (str == ".version")
	{   //! print full version info
		std::cout <<
			"SE parsre version 1.2.0\n"
			"Copyright (C) 2021 ZYMelaii, Inc.\n"
			"This is an example program built for se (Simple Eval REPL) project."
		<< std::endl;
		return 2;
	}
	return 1;
}

std::ostream& print_token(token_t &token)
{
	std::cout << std::string(token.p, token.r);
	return std::cout;
}

std::ostream& print_unit(unit_t &unit)
{
	std::cout << std::string(unit.tok, unit.tok + unit.len);
	return std::cout;
}