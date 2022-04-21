#pragma once

#include <se/type/object.h>
#include <stdint.h>
#include <stddef.h>

// 1. se_stack_t用alloc.h提供的方法进行内存分配，
// 务必确保se_stack_t的创建、使用与销毁在同一分配器环境下
// 2. 注册alloc.h的se_alloc_cleanup以避免不必要的内存泄漏

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