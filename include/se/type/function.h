#pragma once

#include <se/stack.h>
#include <se/type/object.h>
#include <se/stack.h>
#include <se/exception.h>
#include <stdint.h>
#include <stddef.h>

typedef se_object_t(*se_fncall_t)(se_stack_t*);

typedef struct function_s
{
	se_fncall_t fn;
	const char *symbol;
	int argc; // -1时表示容许可变参数
} se_function_t;

#ifdef __cplusplus
extern "C" {
#endif

se_object_t se_call(se_function_t func, se_stack_t *ps);

#ifdef __cplusplus
}
#endif