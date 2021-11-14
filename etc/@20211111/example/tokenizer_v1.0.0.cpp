#include <se/token.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <cstdlib>

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

	const char *p = token.p;
	while (p <= token.r - 1)
	{
		os << *p++;
	}

	os << "\"";

	return os;
}

void tokenize(const std::string &str)
{
	token_t token;

	const char *p = str.c_str();

	while (true)
	{
		int pattern = predict(p);
		if (pattern == T_INVALID)
		{
			token.type = T_INVALID;
			int code = (unsigned char)p[0];
			std::cout << token << " - error: ";
			if (code < 32 || code >= 128)
			{
				std::cout << "stray \'\\"
					<< std::oct << std::setw(3) << std::setfill('0')
					<< code << "\' in program";
			} else
			{
				std::cout << "illegal character \'" << p[0] << "\' in program";
			}
			std::cout << std::endl;
			break;
		} else if (pattern == T_NULL)
		{
			token.type = T_NULL;
			std::cout << token << std::endl;
			break;
		} else if (pattern == T_INDENT)
		{
			get_indent(p, &token);
			std::cout << token << std::endl;
			p = token.r;
		} else if (pattern == T_NUMBER)
		{
			get_number(p, &token);
			std::cout << token << (token.q != nullptr ? ""
				: " - error: illegal number literal") << std::endl;
			p = token.r;
		} else if (pattern == T_SYMBOL)
		{
			get_symbol(p, &token);
			std::cout << token << std::endl;
			p = token.r;
		} else if (pattern == T_OPERATOR)
		{
			get_operator(p, &token);
			std::cout << token << std::endl;
			p = token.r;
		} else
		{
			std::cout << "@Undefined - "
				"error: uncaught token type ???" << std::endl;
			break;
		}
	}
}

int rout(const std::string &str)
{
	if (str == "")
	{	//! empty line -> switch to new line
		return 2;
	} else if (str == ".exit" || str == ".quit" || str == ".q")
	{	//! quit tokenizer
		return 0;
	} else if (str == ".clear" || str == ".cls" || str[0] == '\014'/*Ctrl-L*/)
	{	//! clear screen
#ifdef _WIN32
		system("cls");
#elif __linux__
		system("clear");
#endif
		return 2;
	} else if (str == ".v")
	{	//! print zipped version info
		std::cout << "SE tokenizer v1.0.0" << std::endl;
		return 2;
	} else if (str == ".version")
	{	//! print full version info
		std::cout <<
			"SE tokenizer version 1.0.0\n"
			"Copyright (C) 2021 ZYMelaii, Inc.\n"
			"This is an example program built for se (Simple Eval REPL) project."
		<< std::endl;
		return 2;
	}
	return 1;
}

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
		else if (state == 1) tokenize(str);
		else if (state == 2) continue;
	}

	return 0;
}