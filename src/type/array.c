#include <se/type.h>
#include <se/alloc.h>
#include <se/stack.h>
#include <string.h>

se_array_t stack2array(se_stack_t *ps, int reverse)
{
	se_array_t ret = { 0 };

	if (ps == 0L) return ret;
	if (ps->size == 0) return ret;

	ret.data = (se_object_t*)se_alloc(ps->size * sizeof(se_object_t));
	ret.size = ps->size;

	if (!!reverse)
	{
		memcpy(ret.data, ps->stack, ret.size * sizeof(se_object_t));
	} else
	{
		int i = 0, j = ret.size - 1;
		while (j >= 0)
		{
			ret.data[i++] = ps->stack[j--];
		}
	}

	return ret;
}