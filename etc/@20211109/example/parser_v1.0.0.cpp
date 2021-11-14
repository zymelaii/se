#include <se/token.h>
#include <se/alloc.h>
#include <se/stack.h>
#include <se/type.h>
#include <se/priority.h>
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
	token_t token;
	const char *p = str.c_str();
	std::vector<token_t> stack;
	int row = 1, col = 1, error = 0;
	while (true)
	{
		se_getter(predict(p))(p, &token);
		if (token.type != T_INVALID && token.type != T_NULL && token.type != T_INDENT
			&& !(token.type == T_OPERATOR && token.sub_type == T_OP_SEMICOLON))
		{
			stack.push_back(token);
		}
		if (token.type == T_OPERATOR && token.sub_type == T_OP_SEMICOLON
			|| token.type == T_NULL && token.sub_type == T_NUL_END) break;
		p = token.r;
	}

	typedef struct { std::string str; int depth; } ele_t;

	std::vector<token_t> rpstack;
	std::vector<ele_t> rpn;

	int depth = 0;
	for (int i = 0; i < stack.size(); ++i)
	{
		if (stack[i].type != T_OPERATOR)
		{
			rpn.push_back({ std::string(stack[i].p, stack[i].r), depth });
		} else
		{
			int opid = tok2op(stack[i].sub_type);
			if (opid != 0)
			{
				int priority = get_priority(opid);
				while (true)
				{
					if (rpstack.size() == 0)
					{
						rpstack.push_back(stack[i]);
						break;
					}
					token_t tok = rpstack.back();
					int op = tok2op(tok.sub_type);
					if (op != 0 && (get_priority(op) < priority
						|| get_priority(op) == priority
						&& priority != 2 && priority != 14))
					{
						rpn.push_back({ std::string(tok.p, tok.r), depth });
						rpstack.pop_back();
					} else
					{
						rpstack.push_back(stack[i]);
						break;
					}
				}
			} else switch (stack[i].sub_type)
			{
				case T_OP_BRACKET0L:
				case T_OP_BRACKET1L:
				case T_OP_BRACKET2L:
				{
					rpstack.push_back(stack[i]);
					if (stack[i].sub_type == T_OP_BRACKET0L) ++depth;
					break;
				}
				case T_OP_BRACKET0R:
				case T_OP_BRACKET1R:
				case T_OP_BRACKET2R:
				{
					const char *s = stack[i].sub_type == T_OP_BRACKET0R ? "()"
						: stack[i].sub_type == T_OP_BRACKET1R ? "[]" : "{}";
					while (true)
					{
						if (rpstack.size() == 0)
						{	// error
							rpn.push_back({ std::string(s), depth });
							break;
						}
						token_t tok = rpstack.back();
						rpstack.pop_back();
						if (tok.sub_type + 1 == stack[i].sub_type)
						{
							rpn.push_back({ std::string(s), depth - (tok.sub_type == T_OP_BRACKET0L) });
							break;
						} else if (tok.sub_type == T_OP_BRACKET0L
							|| tok.sub_type == T_OP_BRACKET1L
							|| tok.sub_type == T_OP_BRACKET2L)
						{	// unexpected cross bracket
							printf("error: unexpected cross bracket\n");
							break;
						} else
						{
							rpn.push_back({ std::string(tok.p, tok.r), depth });
						}
					}
					if (stack[i].sub_type == T_OP_BRACKET0R) --depth;
					break;
				}
			}
		}
	}

	while (rpstack.size() != 0)
	{
		token_t tok = rpstack.back();
		rpstack.pop_back();
		rpn.push_back({ std::string(tok.p, tok.r), depth });
	}

	std::cout << "RPN:";
	for (auto ele : rpn)
	{
		std::cout<< " " << ele.str << "#" << ele.depth;
	}

	std::cout << std::endl;
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
		std::cout << "SE parser v1.0.0" << std::endl;
		return 2;
	} else if (str == ".version")
	{   //! print full version info
		std::cout <<
			"SE parser version 1.0.0\n"
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