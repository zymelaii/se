#pragma once

#include <stdint.h>

// eObject
#define EO_NIL   0
#define EO_OBJ   1
#define EO_NUM   2
#define EO_FUNC  3
#define EO_ARRAY 4

typedef struct object_s
{
	void    *data;
	uint16_t id;
	uint16_t type;
	uint16_t is_ref;
	uint16_t is_nil;
} se_object_t;