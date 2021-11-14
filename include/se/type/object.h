#pragma once

#include <stdint.h>

// eObject
#define EO_NIL   0 // 空值（nil）
#define EO_OBJ   1
#define EO_NUM   2
#define EO_FUNC  3
#define EO_ARRAY 4

typedef struct object_s
{
	void    *data;
	uint16_t id;
	uint16_t type;
	uint16_t refs;
	uint16_t is_nil; // 是否为空对象（即对象无效，不等同于空值）
} se_object_t;