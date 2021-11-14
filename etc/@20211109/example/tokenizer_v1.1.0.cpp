#include <se/token.h>
#include <se/stack.h>
#include <se/type.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>

std::ostream& print_tokenizer_result(token_t &token);
void tokenize(const std::string &str);
void lex(const std::string &str);
int  rout(const std::string &str);

#define M_TOKENIZER 1
#define M_LEXER     2
#define M_PARSER    3
#define M_EVALUATOR 4

int REPL_MODE = M_TOKENIZER;

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
		else if (state == 1)
		switch (REPL_MODE)
		{
			case M_TOKENIZER: tokenize(str); break;
			case M_LEXER: lex(str); break;
		}
		else if (state == 2) continue;
	}

	return 0;
}

std::ostream& print_tokenizer_result(token_t &token)
{
	switch (token.type)
	{
		case T_NULL:     printf("@Null");     return std::cout;
		case T_INVALID:  printf("@Invalid");  break;
		case T_INDENT:   printf("@Indent");   break;
		case T_NUMBER:   printf("@Number");   break;
		case T_SYMBOL:   printf("@Symbol");   break;
		case T_OPERATOR: printf("@Operator"); break;
	}

	if (token.type == T_INVALID)
	{
		printf(" \\%03o", (unsigned char)token.p[0]);
		return std::cout;
	}

	if (token.type == T_INDENT)
	{
		const char *subtypes[] = { "blank", "tab", "newline" };

		int count = token.r - token.p;
		if (token.sub_type == T_IDT_NEWLINE
			&& token.p[0] == '\r' && token.p[1] == '\n')
		{
			count >>= 1;
		}
		printf(" #%s%d", subtypes[token.sub_type - 1], count);
		return std::cout;
	}

	if (token.type == T_NUMBER && token.sub_type != 0)
	{
		const char *subtypes[] = { "bin", "oct", "dec", "hex", "float", "EEEE" };
		printf(" #%s", subtypes[token.sub_type - 1]);
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
		printf(" #%s", subtypes[token.sub_type - 1]);
	}

	printf(" \"");

	const char *p = token.p;
	while (p <= token.r - 1)
	{
		printf("%c", *p++);
	}

	printf("\"");

	if (token.type == T_NUMBER)
	{
		char buf[64];
		se_number_t num = parse_number(&token);
		printf(" %s", obj2str(wrap2obj(&num, EO_NUM), buf, 64));
	}

	return std::cout;
}

void tokenize(const std::string &str)
{
	token_t token;
	const char *p = str.c_str();

	while (true)
	{
		se_getter(predict(p))(p, &token);
		print_tokenizer_result(token) << std::endl;
		if (token.type == T_NULL && token.sub_type == T_NUL_END) break;
		p = token.r;
	}
}

void lex(const std::string &str)
{
	token_t token;
	const char *p = str.c_str();
	std::vector<token_t> stack;
	int row = 1, col = 1, error = 0;
	while (true)
	{
		se_getter(predict(p))(p, &token);
		stack.push_back(token);
		switch (token.type)
		{
			case T_INVALID:
			{
				if (token.sub_type == T_INV_UNKNOWN)
				{
					printf("#%d:%d " "error: "
						"unrecognized character '%c'\n",
						row, col, token.p[0]);
					++error;
				}
				if (token.sub_type == T_INV_UNICODE)
				{
					printf("#%d:%d " "error: "
						"stray \\%03o in program\n",
						row, col, (unsigned char)token.p[0]);
					++error;
				}
				col += 1;
			}
			break;
			case T_INDENT:
			{
				int count = token.r - token.p;
				if (token.sub_type == T_IDT_NEWLINE
					&& token.p[0] == '\r' && token.p[1] == '\n')
				{
					count >>= 1;
				}
				if (token.sub_type == T_IDT_BLANK) col += count;
				if (token.sub_type == T_IDT_TAB) col += count * 4;
				if (token.sub_type == T_IDT_NEWLINE) row += count, col = 1;
			}
			break;
			case T_NUMBER:
			{
				if (token.q == 0L)
				{
					printf("#%d:%d " "error: "
						"catch NaN \"",
						row, col, (unsigned char)token.p[0]);
					const char *p = token.p;
					while (p < token.r)
					{
						printf("%c", *p++);
					}
					printf("\"\n");
					++error;
				}
				col += token.r - token.p;
			}
			break;
			case T_SYMBOL:
			{
				col += token.r - token.p;
			}
			break;
			case T_OPERATOR:
			{
				col += token.r - token.p;
			}
			break;
		}
		if (token.type == T_NULL && token.sub_type == T_NUL_END) break;
		p = token.r;
	}

	if (error != 0)
	{
		printf("lexer: %d error%s in total\n", error, (error == 1 ? "" : "s"));
		return;
	}

// expect seperator before '%s';

	int i = stack[0].type != T_INDENT && stack[0].type != T_NULL ? 1 : 0;
	int j = i + 1;
	token_t front = stack[i];

	while (j < stack.size())
	{
		token = stack[j];

		if ((front.type == T_NUMBER || front.type == T_SYMBOL)
		 	&& (token.type == T_NUMBER || token.type == T_SYMBOL))
		{
			printf("error: expect seperator before \"");
			const char *p = token.p;
			while (p < token.r)
			{
				printf("%c", *p++);
			}
			printf("\"\n");
			return;
		}

		if (token.type == T_OPERATOR && token.sub_type == T_OP_BRACKET1L
			&& front.type != T_SYMBOL)
		{
			printf("error: array index expect an array before, "
				"got \"");
			const char *p = front.p;
			while (p < front.r)
			{
				printf("%c", *p++);
			}
			printf("\" instead\n");
			return;
		}

		if (token.type == T_INDENT)
		{
			++j;
		} else
		{
			front = token;
			if (front.type != T_NULL)
			{
				stack[i] = front;
			}
			++i, ++j;
		}
	}

	printf("lexer stack:\n");
	for (int j = i - 1; j >= 0; --j)
	{
		printf("    [%2d] ", j);
		print_tokenizer_result(stack[j]) << std::endl;
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
		std::cout << "SE tokenizer v1.1.0" << std::endl;
		return 2;
	} else if (str == ".version")
	{   //! print full version info
		std::cout <<
			"SE tokenizer version 1.1.0\n"
			"Copyright (C) 2021 ZYMelaii, Inc.\n"
			"This is an example program built for se (Simple Eval REPL) project."
		<< std::endl;
		return 2;
	} else if (str == ".mode")
	{
		const char *modes[] = { "#", "tokenizer", "lexer", "parser", "evaluator" };
		std::cout << modes[REPL_MODE] << std::endl;
		return 2;
	} else if (str == ".mode tokenizer")
	{
		REPL_MODE = M_TOKENIZER;
		std::cout << "mode switch to tokenizer" << std::endl;
		return 2;
	} else if (str == ".mode lexer")
	{
		REPL_MODE = M_LEXER;
		std::cout << "mode switch to lexer" << std::endl;
		return 2;
	}
	return 1;
}