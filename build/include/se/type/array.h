#pragma once

#include <se/type/object.h>
#include <stdint.h>
#include <stddef.h>

typedef struct array_s
{
	se_object_t *data;
	size_t       size;
} se_array_t;

#ifdef __cplusplus
extern "C" {
#endif

se_array_t stack2array(se_stack_t *ps, int reverse);

#ifdef __cplusplus
}
#endif