#include <se/alloc.h>
#include <se/exception.h>
#include <se/context.h>
#include <stdlib.h>
#include <iostream>
#include <string>

#include "ee.c"

int main(int argc, char *argv[])
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

			char buf[256];
			const se_object_t *ret = se_ctx_get_last_ret(&ctx);
			if (ret != nullptr)
			{
				if (ret->type == EO_OBJ)
				{
					printf("{ id: %d, refs: %d } => %s\n",
						ret->id, ret->refs,
						obj2str(*(se_object_t*)ret->data, buf, 256));
				} else
				{
					std::cout << obj2str(*ret, buf, 256) << std::endl;
				}
			} else
			{
				std::cout << "error: uncaught result" << std::endl;
			}

			se_ctx_sweep(&ctx);
			handle_exception();
		}
	}

	se_ctx_destroy(&ctx);

	return 0;
}