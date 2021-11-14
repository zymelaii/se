#include <se/type.h>
#include <se/exception.h>

se_object_t se_call(se_function_t func, se_stack_t *ps)
{
	se_object_t ret =
	{
		.data   = 0L,
		.id     = -1,
		.type   = EO_NIL,
		.refs   = 0,
		.is_nil = 1
	};

	if (func.fn == 0L)
	{
		se_throw(RuntimeError, ExpectFunction, 0, 0);
		return ret;
	}

	if (ps == 0L)
	{
		se_throw(RuntimeError, BadFunctionCallArgs, 0, 0);
		return ret;
	}

	if (func.argc != -1 && ps->size != func.argc)
	{
		se_throw(RuntimeError, BadFunctionCallArgc, 0, 0);
		return ret;
	}

	ret = func.fn(ps);

	return ret;
}