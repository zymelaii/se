#pragma once

#include <se/type/object.h>
#include <se/type/number.h>
#include <se/type/function.h>
#include <se/type/array.h>

#ifdef __cplusplus
extern "C" {
#endif

se_object_t wrap2obj(void *data, int type);
se_object_t objclone(se_object_t obj);
const char* obj2str(se_object_t obj, char *buffer, int len);

#ifdef __cplusplus
}
#endif