#include <se/alloc.h>
#include <se/exception.h>
#include <se/context.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <string>

bool handle_exception();

int main()
{
	atexit(se_alloc_cleanup);

	se_context_t ctx;
	se_exception_t e;

	se_ctx_create(&ctx);

	const char *PROMPT = ">>> ";
	std::string str;

	while (true)
	{
		std::cout << PROMPT;

		std::getline(std::cin, str);
		if (std::cin.eof()) break;
		if (str.empty()) continue;

		se_ctx_load(&ctx, str.c_str());

		while (se_ctx_complete(&ctx) != 0)
		{
			se_ctx_forward(&ctx);
			if (handle_exception()) continue;

			se_ctx_parse(&ctx);
			if (handle_exception()) continue;

			se_ctx_execute(&ctx);
			if (handle_exception()) continue;

			char buf[64];
			const se_object_t *ret = se_ctx_get_last_ret(&ctx);
			if (ret != nullptr)
			{
				while (ret->type == EO_OBJ)
				{
					ret = (se_object_t*)ret->data;
				}
				std::cout << obj2str(*ret, buf, 64) << std::endl;
			} else
			{
				std::cout << "error: uncaught result" << std::endl;
			}
		}
	}

	se_ctx_destroy(&ctx);

	return 0;
}

bool handle_exception()
{
	if (se_caught()) return false;

	se_exception_t e;
	if (se_catch(&e, UnknownError))
	{
		const char *serror[] = { "ArgumentErrorInCSrc" };
		std::cout << "UnknownError: " << serror[e.error - 1] << std::endl;
	} else if (se_catch(&e, SyntaxError))
	{
		const char *serror[] = {
			"InvalidSyntax", "UndefinedIdentifier", "UnicodeChar", "InvalidNumberLiteral",
			"ExpectSeperator", "SymbolTooLong", "NoLeftBracket", "NoRightBracket",
			"CrossedBrackets", "MissingComma", "TooManyCommas", "MissingOperand",
			"BeyondCharset" };
		std::cout << "SyntaxError: " << serror[e.error - 1] << std::endl;
	} else if (se_catch(&e, TypeError))
	{
		const char *serror[] = {
			"NonCallableObject", "NonIndexableObject", "NonExpandableObject", "MathOperationAmongNonNumbers",
			"ModuloWithFloat", "BitwiseOpWithFloat" };
		std::cout << "TypeError: " << serror[e.error - 1] << std::endl;
	} else if (se_catch(&e, IndexError))
	{
		const char *serror[] = {
			"NoIndex", "MissingArray", "ExpectNonNegativeIntegerIndex", "IndexOutOfRange" };
		std::cout << "IndexError: " << serror[e.error - 1] << std::endl;
	} else if (se_catch(&e, RuntimeError))
	{
		const char *serror[] = {
			"ExpectFunction", "BadFunctionCallArgs", "BadFunctionCallArgc", "BadFunctionCallArgType",
			"ExpandEmptyArray", "AssignLeftValue", "MathOperationWithNaNOrInf", "IntDivOrModByZero",
			"NoAvailableID", "BadAlloc", "BadSymbolInsertion" };
		std::cout << "RuntimeError: " << serror[e.error - 1] << std::endl;
	} else if (se_catch_any(&e))
	{
		printf("UncaughtException: CustomError(%d): extra=0x%016x reserved=0x%16x",
			e.error - CustomError, e.extra, e.reserved);
	}
	return true;
}