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
#include <map>

void eval(const std::string &str);
int  rout(const std::string &str);
void exec_rpn(unit_t *rp, int n);
std::ostream& print_rpn(unit_t *rp, int n);
std::ostream& print_stack(se_stack_t *stack);

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
		else if (state == 1) eval(str);
		else if (state == 2) continue;
	}

	return 0;
}

void eval(const std::string &str)
{
	unit_t  *rp     = nullptr;
	token_t *tokens = nullptr;
	int      ntok   = 0;
	int      nrp    = 0;

	const char *p = str.c_str();
	while (p != nullptr)
	{
		p = str2tokens(p, &tokens, &ntok);
		if (ntok == 0) continue;

		rp = build_rpn(tokens, ntok, &nrp);
		if (nrp != 0)
		{
			std::cout << "statement: ";
			print_rpn(rp, nrp) << std::endl;

			exec_rpn(rp, nrp);
			se_free(rp);
		}

		se_free(tokens);
	}
}

void exec_rpn(unit_t *rp, int n)
{
	int bound_eles = 1;
	int this_id = 0;

	std::map<std::string, int> sym_table; // 符号表
	std::map<int, se_object_t*> id_table; // ID表

	se_stack_t stack = se_stack_create(0);
	std::vector<int> depth_stack;
	std::vector<void*> mempool;

	#define se_push_ele(type, val, etype, depth)   \
	do {                                           \
		void *p = se_alloc(sizeof(type));          \
		*(type*)p = (val);                         \
		se_stack_push(&stack, wrap2obj(p, etype)); \
		depth_stack.push_back(depth);              \
		mempool.push_back(p);                      \
	} while (0);

	int i = 0;
	while (i < n)
	{
		token_t tok = unit2tok(rp[i]);
		if (tok.type == T_NUMBER)
		{	// number
			se_push_ele(se_number_t, parse_number(&tok), EO_NUM, rp[i].depth);
		} else if (tok.type == T_SYMBOL)
		{	// symbol
			se_object_t obj;
			std::string symbol(tok.p, tok.r);
			obj = wrap2obj(0L, EO_NIL);
			if (sym_table.find(symbol) == sym_table.end())
			{
				obj.id            = this_id;
				sym_table[symbol] = this_id;
				id_table[this_id] = nullptr;
				++this_id;
			} else
			{
				obj.id = sym_table[symbol];
				if (id_table[obj.id] != nullptr)
				{
					obj = *id_table[obj.id];
				}
			}
			se_stack_push(&stack, obj);
			depth_stack.push_back(rp[i].depth);
		} else switch (tok.sub_type)
		{	// operator
			case OP_BRE:
			{
				int expect_depth = rp[i].depth + 1;

				se_object_t ret = wrap2obj(0L, EO_NIL);

				if (stack.size > 0 && bound_eles > 0)
				{
					if (depth_stack.back() == expect_depth)
					{
						ret = se_stack_top(&stack);
					}
				}

				while (stack.size > 0 && bound_eles > 0)
				{
					int next_depth = depth_stack.back();
					if (next_depth != expect_depth) break;

					se_stack_pop(&stack);
					depth_stack.pop_back();
					--bound_eles;
				}

				if (bound_eles > 1)
				{
					std::cout <<
						"RuntimeError: "
						"require more elements in () pop action (unexpected)"
					<< std::endl;
					goto clean;
				}

				se_stack_push(&stack, ret);
				depth_stack.push_back(rp[i].depth);
			}
			break;
			case OP_ARG:
			{
				int expect_depth = rp[i].depth + 1;
				se_stack_t argstack = se_stack_create(bound_eles);
				while (stack.size > 0 && bound_eles > 0)
				{
					int next_depth = depth_stack.back();
					if (next_depth != expect_depth) break;

					se_stack_push(&argstack, se_stack_pop(&stack));
					depth_stack.pop_back();
					--bound_eles;
				}

				if (bound_eles > 1)
				{	// 表达式错误
					std::cout <<
						"ArgumentError: "
						"failed matching arguments when apply ()"
					<< std::endl;
					goto clean;
				} else if (stack.size == 0)
				{
					std::cout <<
						"FunctionError: "
						"calling without function name"
					<< std::endl;
					goto clean;
				}

				se_object_t obj = se_stack_pop(&stack);
				if (obj.type != EO_FUNC)
				{
					std::cout <<
						"FunctionError: "
						"object is not callable"
					<< std::endl;
					goto clean;
				}

				int state;
				se_object_t ret = se_call(*(se_function_t*)obj.data, &argstack, &state);
				switch (state)
				{
					case EF_CALL:
					std::cout <<
						"RuntimeError: "
						"calling an invalid function"
					<< std::endl;
					goto clean;
					case EF_ARGS:
					std::cout <<
						"RuntimeError: "
						"missing arguments in function call"
					<< std::endl;
					goto clean;
					case EF_ARGC:
					std::cout <<
						"RuntimeError: "
						"not enough arguments for function call"
					<< std::endl;
					goto clean;
					case EF_TYPE:
					std::cout <<
						"RuntimeError: "
						"got argument of wrong type in function call"
					<< std::endl;
					goto clean;
				}

				// 函数执行成功，结果入栈
				se_stack_push(&stack, ret);
				depth_stack.push_back(rp[i].depth);
			}
			break;
			case OP_IDX:
			{
				if (stack.size == 0)
				{
					std::cout <<
						"IndexError: "
						"expect index number"
					<< std::endl;
					goto clean;
				}

				se_object_t obj = se_stack_pop(&stack);
				if (obj.type != EO_NUM)
				{
					std::cout <<
						"IndexError: "
						"index must be non-negative integer"
					<< std::endl;
					goto clean;
				}

				se_number_t *num = (se_number_t*)obj.data;
				if (num->type == EN_FLT || num->i < 0)
				{
					std::cout <<
						"IndexError: "
						"index must be non-negative integer"
					<< std::endl;
					goto clean;
				}

				if (stack.size == 0)
				{
					std::cout <<
						"IndexError: "
						"indexing without array object"
					<< std::endl;
					goto clean;
				}

				obj = se_stack_pop(&stack);

				if (obj.type != EO_ARRAY)
				{
					std::cout <<
						"IndexError: "
						"object is not indexable"
					<< std::endl;
					goto clean;
				}

				int index = num->i;
				se_array_t *array = (se_array_t*)obj.data;

				if (index >= array->size)
				{
					std::cout <<
						"IndexError: "
						"array index out of range"
					<< std::endl;
					goto clean;
				}

				// 获取索引对象，压栈
				obj = array->data[index];
				se_stack_push(&stack, wrap2obj(obj.data, obj.type));
			}
			break;
			case OP_ARR:
			{
				int expect_depth = rp[i].depth;
				se_stack_t array = se_stack_create(bound_eles);
				while (stack.size > 0 && bound_eles > 0)
				{
					int next_depth = depth_stack.back();
					if (next_depth != expect_depth) break;
					se_stack_push(&array, se_stack_pop(&stack));
					depth_stack.pop_back();
					--bound_eles;
				}

				if (bound_eles > 1)
				{
					std::cout <<
						"RuntimeError: "
						"require more elements in {} pop action (unexpected)"
					<< std::endl;
					goto clean;
				}

				se_push_ele(se_array_t, stack2array(&array, 0), EO_ARRAY, rp[i].depth);
			}
			break;
			case OP_CME:
			{
				++bound_eles;
			}
			break;
		}
		if (bound_eles < 1) bound_eles = 1;
		++i;
	}

	if (stack.size == 0 || bound_eles != stack.size)
	{
		std::cout <<
			"SyntaxError: "
			"invalid syntax "
		<< std::endl;
		goto clean;
	} else
	{
		se_object_t result = se_stack_top(&stack);
		result.is_ref = false;
		char buf[64];
		std::cout << obj2str(result, buf, 64) << std::endl;
	}

	for (auto it : sym_table)
	{
		if (id_table[it.second] == nullptr)
		{
			std::cout <<
				"info: "
				"unused symbol \""
				<< it.first << "\""
			<< std::endl;
		}
	}

clean:
	std::cout << "stack: ";
	if (stack.size != 0) std::cout << std::endl;
	print_stack(&stack) << std::endl;
	se_stack_free(&stack);
	for (auto mem : mempool)
	{
		se_free(mem);
	}
	mempool.clear();
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
		std::cout << "SE evaluator v1.0.0" << std::endl;
		return 2;
	} else if (str == ".version")
	{   //! print full version info
		std::cout <<
			"SE evaluator version 1.0.0\n"
			"Copyright (C) 2021 ZYMelaii, Inc.\n"
			"This is an example program built for se (Simple Eval REPL) project."
		<< std::endl;
		return 2;
	}
	return 1;
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
			case OP_BRE: std::cout << "()";   break;
			case OP_ARG: std::cout << "fn()"; break;
			case OP_IDX: std::cout << "[]";   break;
			case OP_ARR: std::cout << "{}";   break;
			case OP_EPA: std::cout << "*epa"; break;
			case OP_PL:  std::cout << "+pl";  break;
			case OP_NL:  std::cout << "-nl";  break;
			default: std::cout << std::string(rp[i].tok, rp[i].tok + rp[i].len);
		}
		std::cout << "@" << rp[i].depth;
		if (i != n - 1)
		{
			std::cout << ",";
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