#pragma once

#include <se/type/object.h>
#include <stdint.h>

typedef struct stack_s
{
	se_object_t *stack;
	size_t size;
	size_t capacity;
} se_stack_t;

#ifdef __cplusplus
extern "C" {
#endif

se_stack_t  se_stack_create(size_t capacity); // defaultly 16
void        se_stack_free  (se_stack_t *ps);
se_object_t se_stack_top   (se_stack_t *ps);
void        se_stack_push  (se_stack_t *ps, se_object_t obj);
se_object_t se_stack_pop   (se_stack_t *ps);

#ifdef __cplusplus
}
#endif