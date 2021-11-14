#include <se/token.h>
#include <se/alloc.h>
#include <se/stack.h>
#include <se/type.h>
#include <se/priority.h>
#include <se/exception.h>
#include <se/parser.h>
#include <math.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>

bool gDisplayTokens = false;
bool gDisplayRPN = false;
bool gDisplaySEUEProgress = false;

std::map<std::string, int> symmap; // symbol -> id
std::map<int, se_object_t> memmap; // id -> object
int id = 0; // id of local value

void eval(const std::string &str);
int  rout(const std::string &str);
void exec_seus(seus_t *seus);
bool handle_exception();
void import_builtin_functions();
void cleanup();
std::string trim(std::string str);
std::ostream& print_tokens(token_t *toks, int n);
std::ostream& print_rpn(unit_t *rp, int n);
std::ostream& print_stack(se_stack_t *stack);

int main(int argc, char const *argv[])
{
	const char *PROMPT = ">>> ";
	std::string str;

	import_builtin_functions();

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

	cleanup();

	return 0;
}

void eval(const std::string &str)
{
	unit_t  *rp     = nullptr;
	token_t *tokens = nullptr;
	int      ntok   = 0;
	int      nrp    = 0;

	se_exception_t e = { 0 };

	const char *p = str.c_str();
	while (p != nullptr)
	{
		p = str2tokens(p, &tokens, &ntok);
		if (gDisplayTokens)
		{
			std::cout << "tokens: ";
			print_tokens(tokens, ntok) << std::endl;
		}
		if (handle_exception()) continue;
		if (ntok == 0) continue;

		rp = toks2rpn(tokens, ntok, &nrp);
		if (!handle_exception())
		{
			if (gDisplayRPN)
			{
				std::cout << "statement: ";
				print_rpn(rp, nrp) << std::endl;
			}

			seus_t seus = rpn2seus(rp, nrp);
			if (!handle_exception())
			{
				exec_seus(&seus);
				handle_exception();
			}

			free_seus(&seus);
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
		std::cout << "SE evaluator v1.2.0" << std::endl;
		return 2;
	} else if (str == ".version")
	{   //! print full version info
		std::cout <<
			"SE evaluator version 1.2.0\n"
			"Copyright (C) 2021 ZYMelaii, Inc.\n"
			"This is an example program built for se (Simple Eval REPL) project."
		<< std::endl;
		return 2;
	} else if (str == ".on tokens")
	{
		gDisplayTokens = true;
		return 2;
	} else if (str == ".on rpn")
	{
		gDisplayRPN = true;
		return 2;
	} else if (str == ".on seus progress")
	{
		gDisplaySEUEProgress = true;
		return 2;
	} else if (str == ".off tokens")
	{
		gDisplayTokens = false;
		return 2;
	} else if (str == ".off rpn")
	{
		gDisplayRPN = false;
		return 2;
	} else if (str == ".off seus progress")
	{
		gDisplaySEUEProgress = false;
		return 2;
	} else if (str == ".reset")
	{
		cleanup();
		import_builtin_functions();
		return 2;
	} else if (str.substr(0, 2) == ".p" || str.substr(0, 6) == ".print")
	{
		auto symbol = trim(str.substr(str[2] == 'r' ? 6 : 2));
		if (!symbol.empty())
		{
			auto it = symmap.find(symbol);
			if (it != symmap.end())
			{
				auto obj = memmap[it->second];
				char buf[64];
				printf("$%s := { id: %d, value: %s }\n",
					it->first.c_str(), obj.id, obj2str(obj, buf, 64));
			} else
			{
				std::cout << "$" << symbol << " is undefined" << std::endl;
			}
			return 2;
		}
	} else if (str == ".list" || str == ".l")
	{
		for (auto it : symmap)
		{
			std::cout << it.second << "# " << it.first << std::endl;
		}
		return 2;
	} else if (str == ".help" || str == ".h")
	{
		printf(
			"<.help|.h>: show this page"                   "\n"
			"<.version|.v>: show version info"             "\n"
			"<.exit|.quit|.q>: quit program"               "\n"
			"<.clear|.cls|^L>: clear screen"               "\n"
			"<.<on|off> <tokens|rpn|seus progress>>: "
			          "enable or disable info output"      "\n"
			"<.list|.l>: list current symbols"             "\n"
			"<.<print|p> <Symbol>>: print value of symbol" "\n"
			"<.reset>: swept all objects and symbols"      "\n"
		);
		return 2;
	}
	return 1;
}

bool handle_exception()
{
	if (se_caught()) return false;
	se_exception_t exception;
	if (se_catch(&exception, SyntaxError))
	{
		const char *serror[] = { "InvalidSyntax", "UndefinedIdentifier", "UnicodeChar", "InvalidNumberLiteral", "ExpectSeperator", "SymbolTooLong", "NoLeftBracket", "NoRightBracket", "CrossedBrackets", "MissingComma", "TooManyCommas", "MissingOperand" };
		std::cout << "SyntaxError: " << serror[exception.error - 1] << std::endl;
	} else if (se_catch(&exception, TypeError))
	{
		const char *serror[] = { "NonCallableObject", "NonIndexableObject", "NonExpandableObject", "MathOperationAmongNonNumbers", "ModuloWithFloat" };
		std::cout << "TypeError: " << serror[exception.error - 1] << std::endl;
	} else if (se_catch(&exception, IndexError))
	{
		const char *serror[] = { "NoIndex", "MissingArray", "ExpectNonNegativeIntegerIndex", "IndexOutOfRange" };
		std::cout << "IndexError: " << serror[exception.error - 1] << std::endl;
	} else if (se_catch(&exception, RuntimeError))
	{
		const char *serror[] = { "ExpectFunction", "BadFunctionCallArgs", "BadFunctionCallArgc", "BadFunctionCallArgType", "ExpandEmptyArray", "AssignLeftValue", "MathOperationWithNaNOrInf", "IntDivOrModByZero" };
		std::cout << "RuntimeError: " << serror[exception.error - 1] << std::endl;
	} else if (se_catch_any(&exception))
	{
		printf("UncaughtException: CustomError(%d): extra=0x%016x reserved=0x%16x",
			exception.error - CustomError, exception.extra, exception.reserved);
	}
	return true;
}

se_object_t __se_builtin_sin(se_stack_t *args)
{
	auto obj_x = se_stack_pop(args);
	if (obj_x.type != EO_NUM)
	{
		se_throw(RuntimeError, BadFunctionCallArgType, (uint64_t)EO_NUM << 32 | obj_x.type, 0);
	} else
	{
		auto x = (se_number_t*)obj_x.data;
		auto y = parse_flt_number(sin(x->type == EN_FLT ? x->f : x->i * 1.0));
		auto ret = objclone(wrap2obj(&y, EO_NUM));
		ret.is_ref = true;
		return ret;
	}
	return wrap2obj(0L, EO_NIL);
}

se_object_t __se_builtin_cos(se_stack_t *args)
{
	auto obj_x = se_stack_pop(args);
	if (obj_x.type != EO_NUM)
	{
		se_throw(RuntimeError, BadFunctionCallArgType, (uint64_t)EO_NUM << 32 | obj_x.type, 0);
	} else
	{
		auto x = (se_number_t*)obj_x.data;
		auto y = parse_flt_number(cos(x->type == EN_FLT ? x->f : x->i * 1.0));
		auto ret = objclone(wrap2obj(&y, EO_NUM));
		ret.is_ref = true;
		return ret;
	}
	return wrap2obj(0L, EO_NIL);
}

se_object_t __se_builtin_ln(se_stack_t *args)
{
	auto obj_x = se_stack_pop(args);
	if (obj_x.type != EO_NUM)
	{
		se_throw(RuntimeError, BadFunctionCallArgType, (uint64_t)EO_NUM << 32 | obj_x.type, 0);
	} else
	{
		auto x = (se_number_t*)obj_x.data;
		auto y = parse_flt_number(log(x->type == EN_FLT ? x->f : x->i * 1.0));
		auto ret = objclone(wrap2obj(&y, EO_NUM));
		ret.is_ref = true;
		return ret;
	}
	return wrap2obj(0L, EO_NIL);
}

void import_builtin_functions()
{
	{
		auto fn = (se_function_t){ __se_builtin_sin, "sin", 1 };
		symmap["sin"] = ++id;
		memmap[id] = objclone(wrap2obj(&fn, EO_FUNC));
	}

	{
		auto fn = (se_function_t){ __se_builtin_cos, "cos", 1 };
		symmap["cos"] = ++id;
		memmap[id] = objclone(wrap2obj(&fn, EO_FUNC));
	}

	{
		auto fn = (se_function_t){ __se_builtin_ln, "ln", 1 };
		symmap["ln"] = ++id;
		memmap[id] = objclone(wrap2obj(&fn, EO_FUNC));
	}
}

void cleanup()
{
	for (auto &it : memmap)
	{
		auto &obj = it.second;
		if (obj.data != nullptr)
		{
			se_free(obj.data);
			obj.data = nullptr;
		}
	}
	std::map<std::string, int>().swap(symmap);
	std::map<int, se_object_t>().swap(memmap);
	id = 0;
}

void exec_seus(seus_t *seus)
{
	se_exception_t last_exception;
	se_catch_any(&last_exception);

	int p  = -1; // pointer to seus->ss
	int i  =  0; // index of seus->us

	se_stack_t efs = se_stack_create(seus->nef); // element frame stack
	se_stack_t vfs = se_stack_create(seus->nvf); // moved value frame stack

	seus->ss[++p] = { 0, 0 };

	for (; i < seus->nus; ++i)
	{
		unit_t *e = seus->us + i;
		int type = e->type >> 8 & 0xf;
		int sub_type = e->type & 0xff;
		if (type == T_SYMBOL)
		{
			auto symbol = std::string(e->tok, e->tok + e->len);
			auto it = symmap.find(symbol);
			se_object_t ret;
			if (it != symmap.end())
			{
				ret = memmap[it->second];
			} else
			{
				symmap[symbol] = ++id;
				ret = wrap2obj(0L, EO_NIL);
				ret.id = id;
				memmap[id] = ret;
			}
			ret.is_ref = true;
			se_stack_push(&efs, ret);
		} else if (type == T_NUMBER)
		{
			auto num = (se_number_t*)se_alloc(sizeof(se_number_t));
			auto tok = unit2tok(*e);
			*num = parse_number(&tok);
			auto obj = wrap2obj(num, EO_NUM);
			memmap[++id] = obj;
			se_stack_push(&efs, obj);
		} else switch (sub_type)
		{
			case OP_BRE_S:
			case OP_ARG_S:
			case OP_IDX_S:
			case OP_ARR_S:
			{
				seus->ss[++p] = { (int)efs.size, 0 };
			}
			break;
			case OP_BRE:
			case OP_IDX:
			{
				auto state = &seus->ss[p--];

				if (efs.size <= state->sframe)
				{
					se_stack_push(&efs, wrap2obj(0L, EO_NIL));
					continue;
				} else for (int c = 0; c < state->accept; ++c)
				{
					se_stack_pop(&vfs);
				}

				if (sub_type == OP_IDX)
				{
					auto obj_index = se_stack_pop(&efs);
					auto obj_array = se_stack_pop(&efs);
					auto index = (se_number_t*)obj_index.data;
					auto array = (se_array_t*)obj_array.data;

					bool  is_index = obj_index.type == EO_NUM
						? index->type != EN_FLT && index->i >= 0
						? true : false : false;

					if (!is_index)
					{
						se_throw(IndexError, ExpectNonNegativeIntegerIndex, 0, 0);
						break;
					}

					if (obj_array.type != EO_ARRAY)
					{
						se_throw(TypeError, NonIndexableObject, 0, 0);
						break;
					}

					if (index->inf || index->i >= array->size)
					{
						se_throw(IndexError, IndexOutOfRange, 0, 0);
						break;
					}

					se_stack_push(&efs, array->data[index->i]);
				}
			}
			break;
			case OP_ARG:
			case OP_ARR:
			{
				auto state = &seus->ss[p--];

				int len = efs.size <= state->sframe ? 0 : state->accept + 1;
				se_array_t as = { 0 };

				se_object_t ret;

				if (len > 0)
				{
					as.size = len;
					as.data = (se_object_t*)se_alloc(as.size * sizeof(se_object_t));
					as.data[as.size - 1] = se_stack_pop(&efs);
					for (int c = state->accept; c > 0; --c)
					{
						as.data[c - 1] = se_stack_pop(&vfs);
					}
				}

				if (sub_type == OP_ARG)
				{
					auto obj_fn = se_stack_pop(&efs);
					se_function_t fn = { 0 };
					if (obj_fn.type == EO_FUNC)
					{
						fn = *(se_function_t*)obj_fn.data;
					}

					int fnstate;
					se_stack_t fns = { .stack = as.data, .size = as.size };
					ret = se_call(fn, &fns);

					if (!se_caught())
					{
						break;
					}
				}

				if (sub_type == OP_ARR)
				{
					ret = wrap2obj(&as, EO_ARRAY);
				}

				if (!ret.is_ref)
				{
					ret = objclone(ret);
					memmap[++id] = ret;
				}

				se_stack_push(&efs, ret);
			}
			break;
			case OP_EPA:
			{
				auto obj_array = se_stack_pop(&efs);
				auto array = (se_array_t*)obj_array.data;

				if (obj_array.type != EO_ARRAY)
				{
					se_throw(TypeError, NonExpandableObject, 0, 0);
					break;
				}

				if (array->size == 0)
				{
					se_throw(RuntimeError, ExpandEmptyArray, 0, 0);
					break;
				}

				auto state = &seus->ss[p];
				se_stack_push(&efs, array->data[array->size - 1]);
				for (int c = 0; c < array->size - 1; ++c)
				{
					se_stack_push(&vfs, array->data[c]);
				}
				state->accept += array->size - 1;
			}
			break;
			case OP_CME:
			{
				auto state = &seus->ss[p];
				se_stack_push(&vfs, efs.stack[state->sframe]);
				for (int c = state->sframe; c < efs.size - 1; ++c)
				{
					efs.stack[c] = efs.stack[c + 1];
				}
				--efs.size;
				++state->accept;
			}
			break;
			case OP_ASS:
			{
				auto rhs = se_stack_pop(&efs);
				auto lhs = se_stack_pop(&efs);

				if (lhs.id == 0)
				{
					se_throw(RuntimeError, AssignLeftValue, 0, 0);
					break;
				}

				if (lhs.id != rhs.id)
				{
					int this_id = lhs.id;
					lhs = objclone(rhs);
					lhs.id = this_id;
				} else
				{
					lhs = rhs;
				}
				memmap[lhs.id] = lhs;

				lhs.is_ref = false;
				se_stack_push(&efs, lhs);
			}
			break;
			case OP_PL:
			case OP_NL:
			{
				auto obj = objclone(se_stack_pop(&efs));
				auto num = (se_number_t*)obj.data;

				if (obj.type != EO_NUM)
				{
					se_throw(TypeError, MathOperationAmongNonNumbers, 0, 0);
					break;
				}

				if (num->inf || num->nan)
				{
					se_throw(RuntimeError, MathOperationWithNaNOrInf, 0, 0);
					break;
				}

				if (sub_type == OP_NL)
				{
					if (num->type == EN_FLT)
					{
						num->f = -num->f;
					} else
					{
						num->i = -num->i;
					}
				}

				memmap[++id] = obj;
				se_stack_push(&efs, obj);
			}
			break;
			case OP_ADD:
			case OP_SUB:
			case OP_MUL:
			case OP_DIV:
			case OP_MOD:
			{
				auto obj_rhs = se_stack_pop(&efs);
				auto obj_lhs = objclone(se_stack_pop(&efs));
				auto lhs = (se_number_t*)obj_lhs.data;
				auto rhs = (se_number_t*)obj_rhs.data;

				if (obj_lhs.type != EO_NUM || obj_rhs.type != EO_NUM)
				{
					se_throw(TypeError, MathOperationAmongNonNumbers, 0, 0);
					break;
				}

				if (lhs->inf || rhs->inf || lhs->nan || rhs->nan)
				{
					se_throw(RuntimeError, MathOperationWithNaNOrInf, 0, 0);
					break;
				}

				int32_t ix, iy;
				double  fx, fy;

				bool use_flt = lhs->type == EN_FLT || rhs->type == EN_FLT;

				if (use_flt && sub_type == OP_MOD)
				{
					se_throw(TypeError, ModuloWithFloat, 0, 0);
					break;
				}

				if (use_flt)
				{
					fx = lhs->type == EN_FLT ? lhs->f : 1.0 * lhs->i;
					fy = rhs->type == EN_FLT ? rhs->f : 1.0 * rhs->i;
				} else
				{
					ix = lhs->i;
					iy = rhs->i;
				}

				switch (sub_type)
				{
					case OP_MOD:
					{
						if (iy == 0)
						{
							se_throw(RuntimeError, IntDivOrModByZero, 0, 0);
							break;
						}
						lhs->i = ix % iy;
					}
					break;
					case OP_DIV:
					{
						if (use_flt)
						{
							lhs->f = fx / fy;
							lhs->inf = isinf(lhs->f);
							lhs->nan = isnan(lhs->f);
						} else if (iy == 0)
						{
							se_throw(RuntimeError, IntDivOrModByZero, 0, 0);
							break;
						} else
						{
							lhs->i = ix / iy;
						}
					}
					break;
					case OP_ADD:
					{
						if (use_flt)
						{
							lhs->f = fx + fy;
							lhs->inf = isinf(lhs->f);
							lhs->nan = isnan(lhs->f);
						} else
						{
							lhs->i = ix + iy;
							lhs->inf = ix > 0 && iy > 0 && lhs->i <= 0
								|| ix < 0 && iy < 0 && lhs->i >= 0;
						}
					}
					break;
					case OP_SUB:
					{
						if (use_flt)
						{
							lhs->f = fx - fy;
							lhs->inf = isinf(lhs->f);
							lhs->nan = isnan(lhs->f);
						} else
						{
							lhs->i = ix - iy;
							lhs->inf = ix > 0 && iy < 0 && lhs->i <= 0
								|| ix < 0 && iy > 0 && lhs->i >= 0;
						}
					}
					break;
					case OP_MUL:
					{
						if (use_flt)
						{
							lhs->f = fx * fy;
							lhs->inf = isinf(lhs->f);
							lhs->nan = isnan(lhs->f);
						} else
						{
							lhs->i = ix * iy;
							if (iy != 0)
							{
								lhs->inf = lhs->i / iy != ix;
							}
						}
					}
					break;
				}

				lhs->type = use_flt ? EN_FLT : lhs->type;
				memmap[++id] = obj_lhs;
				se_stack_push(&efs, obj_lhs);
			}
			break;
		}

		if (!se_caught()) break;

		if (gDisplaySEUEProgress)
		{
			char buf[64];

			std::cout << "status" << "#" << i << ":" << std::endl;

			std::cout << "[ss<" << p + 1 << ">]";
			if (p + 1 == 0)
			{
				std::cout << " (null)";
			} else for (int c = 0; c < p + 1; ++c)
			{
				printf(" (%d, %d)", seus->ss[c].sframe, seus->ss[c].accept);
			}
			std::cout << std::endl;

			std::cout << "[ef<" << efs.size << ">]";
			if (efs.size == 0)
			{
				std::cout << " (null)";
			} else for (int c = 0; c < efs.size; ++c)
			{
				printf(" %s", obj2str(efs.stack[c], buf, 64));
			}
			std::cout << std::endl;

			std::cout << "[vf<" << vfs.size << ">]";
			if (vfs.size == 0)
			{
				std::cout << " (null)";
			} else for (int c = 0; c < vfs.size; ++c)
			{
				printf(" %s", obj2str(vfs.stack[c], buf, 64));
			}
			std::cout << std::endl;
		}
	}

	if (efs.size == 1 && se_caught())
	{
		char buf[64];
		auto result = se_stack_top(&efs);
		result.is_ref = false;
		std::cout << obj2str(result, buf, 64) << std::endl;
	}

_clean:
	se_stack_free(&efs);
	se_stack_free(&vfs);

	if (se_caught())
	{
		se_throw(last_exception.etype, last_exception.error,
			last_exception.extra, last_exception.reserved);
	}
}

std::string trim(std::string str)
{
	if(!str.empty())
	{
		str.erase(0, str.find_first_not_of(" \n\r\t"));
		str.erase(str.find_last_not_of(" \n\r\t") + 1);
	}
	return str;
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
			case OP_BRE_S: std::cout << "()#";   break;
			case OP_BRE_E: std::cout << "()";    break;
			case OP_ARG_S: std::cout << "fn()#"; break;
			case OP_ARG_E: std::cout << "fn()";  break;
			case OP_IDX_S: std::cout << "[]#";   break;
			case OP_IDX_E: std::cout << "[]";    break;
			case OP_ARR_S: std::cout << "{}#";   break;
			case OP_ARR_E: std::cout << "{}";    break;
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