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

	int id = 0;

	std::cout << "#----------------" << std::endl;
	#define RPNCAT(DEPTH, ARGS...) \
	rpn += std::string(ARGS) + "@" + std::to_string((DEPTH)) + " ";

	for (auto &tokens : statements)
	{
		stack.clear();
		std::string rpn = "";

		int depth = 0;
		for (auto &tok : tokens)
		{
			if (tok.type != T_OPERATOR)
			{
				RPNCAT(depth, tok.p, tok.r);
			} else
			{
				int op = tok2op(tok.sub_type);
				if (op != 0)
				{
					int priority = get_priority(op);
					while (true)
					{
						if (stack.empty())
						{
							stack.push_back(tok);
							break;
						}
						token_t tok2 = stack.back();
						int op2 = tok2op(tok2.sub_type);
						if (op2 != 0 && (get_priority(op2) < priority
							|| get_priority(op2) == priority
							&& priority != 2 && priority != 14))
						{
							RPNCAT(depth, tok.p, tok.r);
							stack.pop_back();
						} else
						{
							stack.push_back(tok);
							break;
						}
					}
				} else switch(tok.sub_type)
				{
					case T_OP_BRACKET0L:
					case T_OP_BRACKET1L:
					case T_OP_BRACKET2L:
					{
						stack.push_back(tok);
						if (tok.sub_type == T_OP_BRACKET0L) ++depth;
						break;
					}
					case T_OP_BRACKET0R:
					case T_OP_BRACKET1R:
					case T_OP_BRACKET2R:
					{
						const char *s = tok.sub_type == T_OP_BRACKET0R ? "()"
							: tok.sub_type == T_OP_BRACKET1R ? "[]" : "{}";
						while (true)
						{
							if (stack.empty())
							{
								RPNCAT(depth, s);
								break;
							}
							token_t tok2 = stack.back();
							stack.pop_back();
							if (tok2.sub_type + 1 == tok.sub_type)
							{
								RPNCAT(depth - (tok2.sub_type == T_OP_BRACKET0L), s);
								break;
							} else if (tok2.sub_type == T_OP_BRACKET0L
								|| tok2.sub_type == T_OP_BRACKET1L
								|| tok2.sub_type == T_OP_BRACKET2L)
							{
								printf("error: unexpected cross bracket\n");
								break;
							} else
							{
								RPNCAT(depth, tok.p, tok.r);
							}
						}
						if (tok.sub_type == T_OP_BRACKET0R) --depth;
						break;
					}
				}
			}
		}

		while (!stack.empty())
		{
			token_t tok2 = stack.back();
			stack.pop_back();
			RPNCAT(depth, tok2.p, tok2.r);
		}

		printf("[%02d] %s -> %s\n", id++,
			std::string(tokens.front().p, tokens.back().r).c_str(), rpn.c_str());
	}
	#undef RPNCAT
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
		std::cout << "SE parser v1.1.0" << std::endl;
		return 2;
	} else if (str == ".version")
	{   //! print full version info
		std::cout <<
			"SE parser version 1.1.0\n"
			"Copyright (C) 2021 ZYMelaii, Inc.\n"
			"This is an example program built for se (Simple Eval REPL) project."
		<< std::endl;
		return 2;
	}
	return 1;
}

std::ostream& print_token(token_t &token)
{
	const char *p = token.p;
	while (p <= token.r - 1)
	{
		std::cout << *p++;
	}
	return std::cout;
}

std::ostream& operator<<(std::ostream &os, token_t &token)
{
	switch (token.type)
	{
		case T_INVALID:  os << "@Invalid";  return os;
		case T_NULL:     os << "@Null";     return os;
		case T_INDENT:   os << "@Indent";   break;
		case T_NUMBER:   os << "@Number";   break;
		case T_SYMBOL:   os << "@Symbol";   break;
		case T_OPERATOR: os << "@Operator"; break;
	}

	if (token.type == T_INDENT)
	{
		const char *subtypes[] = { "blank", "tab", "newline" };
		os << " #" << subtypes[token.sub_type - 1];
		return os;
	}

	if (token.type == T_NUMBER && token.sub_type != 0)
	{
		const char *subtypes[] = { "bin", "oct", "dec", "hex", "float", "EEEE" };
		os << " #" << subtypes[token.sub_type - 1];
	}

	if (token.type == T_OPERATOR && token.sub_type != 0)
	{
		const char* subtypes[] = {
			"comma",     "semicolon", "bracket0l", "bracket0r",
			"bracket1l", "bracket1r", "bracket2l", "bracket2r",
			"add",       "sub",       "mod",       "mul",
			"div",       "add_ass",   "sub_ass",   "mod_ass",
			"mul_ass",   "div_ass",   "and",       "or",
			"xor",       "lsh",       "rsh",       "not",
			"nor",       "and_ass",   "or_ass",    "xor_ass",
			"lsh_ass",   "rsh_ass",   "gtr",       "lss",
			"leq",       "geq",       "equ",       "neq",
			"cmp_and",   "cmp_or",    "ass",
		};
		os << " #" << subtypes[token.sub_type - 1];
	}

	os << " \"";
	print_token(token);
	os << "\"";

	return os;
}
