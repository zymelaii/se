#include <se/stack.h>
#include <se/alloc.h>

se_stack_t se_stack_create(size_t capacity)
{
	se_stack_t stack = { 0 };
	if (capacity <= 0)
	{
		capacity = 16;
	}

	stack.stack = (se_object_t*)se_alloc(capacity * sizeof(se_object_t));
	if (stack.stack != 0L)
	{
		stack.capacity = capacity;
	}

	return stack;
}

void se_stack_free(se_stack_t *ps)
{
	if (ps != 0L)
	{
		if (ps->stack != 0L)
		{
			se_free(ps->stack);
		}
		ps->stack    = 0L;
		ps->size     = 0;
		ps->capacity = 0;
	}
}

se_object_t se_stack_top(se_stack_t *ps)
{
	return ps->stack[ps->size - 1];
}

void se_stack_push(se_stack_t *ps, se_object_t obj)
{
	if (ps->size == ps->capacity)
	{
		ps->stack = (se_object_t*)se_realloc(
			ps->stack, ps->capacity * 2 * sizeof(se_object_t));
	}
	ps->stack[ps->size++] = obj;
}

se_object_t se_stack_pop(se_stack_t *ps)
{
	return ps->stack[--ps->size];
}