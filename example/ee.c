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