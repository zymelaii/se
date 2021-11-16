#include <se/alloc.h>
#include <se/exception.h>
#include <se/context.h>
#include <stdlib.h>
#include <iostream>
#include <string>

#include "ee.c"
#include "fnlib.c"
#include "phead.c"
#include "presult.c"
#include "rout.cpp"

int main(int argc, char *argv[])
{
	atexit(se_alloc_cleanup);

	se_context_t ctx;

	se_ctx_create(&ctx);

	show_repl_info();

	const char *PROMPT = ">>> ";
	std::string str;

	import_all(&ctx);

	while (true)
	{
		std::cout << PROMPT;

		std::getline(std::cin, str);
		if (std::cin.eof()) break;
		if (str.empty()) continue;

		int state = rout(&ctx, str);
		if      (state == 0) ;
		else if (state == 1) break;
		else if (state == 2) continue;

		se_ctx_load(&ctx, str.c_str());

		while (se_ctx_complete(&ctx) != 0)
		{
			se_ctx_forward(&ctx);
			if (handle_exception()) continue;

			se_ctx_parse(&ctx);
			if (handle_exception()) continue;

			se_ctx_execute(&ctx);
			if (handle_exception()) continue;

			se_ctx_print_result(&ctx);

			se_ctx_sweep(&ctx);
			handle_exception();
		}
	}

	se_ctx_destroy(&ctx);

	return 0;
}