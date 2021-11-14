#pragma once

#include <se/type/object.h>
#include <se/stack.h>
#include <stdint.h>

#define EF_DONE 0 // sucess
#define EF_CALL 1 // invalid function call
#define EF_ARGS 2 // invalid arguments stack
#define EF_ARGC 3 // given arguments more or less
#define EF_TYPE 4 // type of params is wrong

typedef se_object_t(*se_fncall_t)(se_stack_t*);

typedef struct function_s
{
	se_fncall_t fn;
	const char *symbol;
	size_t      argc;
} se_function_t;

#ifdef __cplusplus
extern "C" {
#endif

se_object_t se_call(se_function_t func, se_stack_t *ps, int *state);

#ifdef __cplusplus
}
#endif