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

void parse(const std::string &str);
int  rout(const std::string &str);
std::ostream& print_tokens(token_t *toks, int n);
std::ostream& print_rpn(unit_t *rp, int n);
std::ostream& print_stack(se_stack_t *stack);

// dead: f({x})=*{f(*{f(x)}}

int main(int argc, char const *argv[])
{
	const char *PROMPT = ">>> ";
	std::string str;

	while (true)
	{
		std::cout << PROMPT;

		std::getline(std::cin, str);
		if (std::cin.eof()) break;

		int state = rout(str);

		if (state == 0) break;
		else if (state == 1) parse(str);
		else if (state == 2) continue;
	}

	return 0;
}

void parse(const std::string &str)
{
	unit_t  *rp     = nullptr;
	token_t *tokens = nullptr;
	int      ntok   = 0;
	int      nrp    = 0;
	int      err    = 0;

	const char *p = str.c_str();
	while (p != nullptr)
	{
		p = str2tokens(p, &tokens, &ntok);
		if (ntok == 0) continue;
		std::cout << "progress: str2tokens done" << std::endl;
		std::cout << "raw tokens: ";
		print_tokens(tokens, ntok) << std::endl;

		rp = toks2rpn(tokens, ntok, &nrp, &err);
		if (err == ECE_DONE)
		{
			std::cout << "progress: toks2rpn done" << std::endl;
			seus_t seus = rpn2seus(rp, nrp, &err);
			if (seus.ss != nullptr)
			{
				std::cout << "progress: rpn2seus done" << std::endl;
				std::cout << "rp statement: ";
				print_rpn(rp, nrp) << std::endl;
			}
			free_seus(&seus);
		} switch (err)
		{
			case ECE_BRCROSS:
			std::cout <<
				"SyntaxError: "
				"caught cross brackets"
			<< std::endl; break;
			case ECE_BRMISSL:
			std::cout <<
				"SyntaxError: "
				"missing left bracket"
			<< std::endl; break;
			case ECE_BRMISSR:
			std::cout <<
				"SyntaxError: "
				"missing right bracket"
			<< std::endl; break;
		}

		se_free(tokens);
	}
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
		std::cout << "SE parser v1.3.0" << std::endl;
		return 2;
	} else if (str == ".version")
	{   //! print full version info
		std::cout <<
			"SE parser version 1.3.0\n"
			"Copyright (C) 2021 ZYMelaii, Inc.\n"
			"This is an example program built for se (Simple Eval REPL) project."
		<< std::endl;
		return 2;
	}
	return 1;
}

std::ostream& print_tokens(token_t *toks, int n)
{
	std::cout << "(";
	for (int i = 0; i < n; ++i)
	{
		std::cout << " ";
		if (toks[i].type != T_OPERATOR)
		{
			std::cout << std::string(toks[i].p, toks[i].r);
		} else switch (toks[i].sub_type)
		{
			case OP_BRE_E: std::cout << "()";    break;
			case OP_BRE_S: std::cout << "()#";   break;
			case OP_ARG_E: std::cout << "fn()";  break;
			case OP_ARG_S: std::cout << "fn()#"; break;
			case OP_IDX_E: std::cout << "[]";    break;
			case OP_IDX_S: std::cout << "[]#";   break;
			case OP_ARR_E: std::cout << "{}";    break;
			case OP_ARR_S: std::cout << "{}#";   break;
			case OP_EPA:   std::cout << "*epa";  break;
			case OP_PL:    std::cout << "+pl";   break;
			case OP_NL:    std::cout << "-nl";   break;
			default: std::cout << std::string(toks[i].p, toks[i].r);
		}
	}
	std::cout << " )";
	return std::cout;
}

std::ostream& print_rpn(unit_t *rp, int n)
{
	std::cout << "(";
	for (int i = 0; i < n; ++i)
	{
		std::cout << " ";
		if ((rp[i].type >> 8 & 0xf) != T_OPERATOR)
		{
			std::cout << std::string(rp[i].tok, rp[i].tok + rp[i].len);
		} else switch (rp[i].type & 0xff)
		{
			case OP_BRE:   std::cout << "()";    break;
			case OP_BRE_S: std::cout << "()#";   break;
			case OP_ARG:   std::cout << "fn()";  break;
			case OP_ARG_S: std::cout << "fn()#"; break;
			case OP_IDX:   std::cout << "[]";    break;
			case OP_IDX_S: std::cout << "[]#";   break;
			case OP_ARR:   std::cout << "{}";    break;
			case OP_ARR_S: std::cout << "{}#";   break;
			case OP_EPA:   std::cout << "*epa";  break;
			case OP_PL:    std::cout << "+pl";   break;
			case OP_NL:    std::cout << "-nl";   break;
			default: std::cout << std::string(rp[i].tok, rp[i].tok + rp[i].len);
		}
	}
	std::cout << " )";
	return std::cout;
}

std::ostream& print_stack(se_stack_t *stack)
{
	if (stack == nullptr) return std::cout;
	if (stack->size == 0)
	{
		std::cout << "(null)";
		return std::cout;
	}

	char buffer[64];
	for (int i = stack->size - 1; i >= 0; --i)
	{
		std::cout << "[" << i + 1 << "] "
			<< obj2str(stack->stack[i], buffer, 64);
		if (i > 0) std::cout << std::endl;
	}

	return std::cout;
}