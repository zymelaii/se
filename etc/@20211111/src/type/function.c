#include <se/type.h>

se_object_t se_call(se_function_t func, se_stack_t *ps, int *state)
{
	se_object_t ret =
	{
		.data   = 0L,
		.id     = -1,
		.type   = EO_NIL,
		.is_ref = 0,
		.is_nil = 1
	};

	if (state == 0L) return ret;

	if (func.fn == 0L)
	{
		*state = EF_CALL;
		return ret;
	}

	if (ps == 0L)
	{
		*state = EF_ARGS;
		return ret;
	}

	if (ps->size != func.argc)
	{
		*state = EF_ARGS;
		return ret;
	}

	if (0) // to be achieved
	{
		*state = EF_TYPE;
		return ret;
	}

	ret = func.fn(ps);
	*state = EF_DONE;

	return ret;
}