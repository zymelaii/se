#include <se/alloc.h>
#include <se/exception.h>
#include <se/context.h>
#include <stdlib.h>
#include <iostream>
#include <iomanip>

int main(int argc, char const *argv[])
{
	atexit(se_alloc_cleanup);

	se_context_t ctx;
	se_exception_t e;

	if (se_ctx_create(&ctx) != 0)
	{
		std::cout << "error: failed create se context" << std::endl;
		return 1;
	} else
	{
		std::cout << "info: create se context" << std::endl;
	}

	for (int i = 1; i < argc; ++i)
	{
		if (se_ctx_load(&ctx, argv[i]) != 0)
		{
			std::cout << "error: failed load se script" << std::endl;
			return 1;
		} else
		{
			std::cout << "info: load se script: \"" << ctx.next_statement << "\"" << std::endl;
		}
	}

	if (argc == 1)
	{
		const char *script = "a = { 1, 2, 3 }[1]";
		if (se_ctx_load(&ctx, script) != 0)
		{
			std::cout << "error: failed load se script" << std::endl;
			return 1;
		} else
		{
			std::cout << "info: load se script: \"" << ctx.next_statement << "\"" << std::endl;
		}
	}

	if (se_ctx_forward(&ctx) != 0)
	{
		std::cout << "error: failed turn forward in se script" << std::endl;
		if (se_catch_any(&e))
		{
			std::cout << "info: catch exception: se_exception_t {"
				<< " etype: " << e.etype << ","
				<< " error: " << e.error << ","
				<< " extra: " << e.extra << ","
				<< " reserved: " << e.reserved << " }"
			<< std::endl;
		}
		return 1;
	} else
	{
		std::cout << "info: tokenize se statement: count = " << ctx.ntokens << std::endl;
	}

	if (se_ctx_complete(&ctx) != 0)
	{
		std::cout << "info: next se statement: \"" << ctx.next_statement << "\"" << std::endl;
	}

	if (se_ctx_parse(&ctx) != 0)
	{
		std::cout << "error: failed parse se statement" << std::endl;
		return 1;
	} else
	{
		if (se_catch_any(&e))
		{
			std::cout << "info: catch exception: se_exception_t {"
				<< " etype: " << e.etype << ","
				<< " error: " << e.error << ","
				<< " extra: " << e.extra << ","
				<< " reserved: " << e.reserved << " }"
			<< std::endl;
			return 1;
		}
		std::cout << "info: parse se statement as seus_t {"
			<< " nef: " << ctx.seus.nef <<  ","
			<< " nvf: " << ctx.seus.nvf <<  ","
			<< " nss: " << ctx.seus.nss <<  ","
			<< " nus: " << ctx.seus.nus <<  ","
			<< " ss: "  << ctx.seus.ss  <<  ","
			<< " us: "  << ctx.seus.us  << " }"
		<< std::endl;
	}

	if (se_ctx_execute(&ctx) != 0)
	{
		std::cout << "error: failed execute seus" << std::endl;
		if (se_catch_any(&e))
		{
			std::cout << "info: catch exception: se_exception_t {"
				<< " etype: " << e.etype << ","
				<< " error: " << e.error << ","
				<< " extra: " << e.extra << ","
				<< " reserved: " << e.reserved << " }"
			<< std::endl;
		}
		return 1;
	} else
	{
		std::cout << "info: execute seus" << std::endl;
		const se_object_t *ret = se_ctx_get_last_ret(&ctx);
		if (ret != nullptr)
		{
			char buffer[64];
			std::cout << "info: seus statement result: " << obj2str(*ret, buffer, 64) << std::endl;
		}
	}

	if (se_ctx_destroy(&ctx) != 0)
	{
		std::cout << "error: failed destroy se context" << std::endl;
		return 2;
	} else
	{
		std::cout << "info: destroy se context" << std::endl;
	}

	return 0;
}