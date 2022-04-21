#pragma once

#include <se/type/object.h>
#include <se/type/number.h>
#include <se/type/function.h>
#include <se/type/array.h>

#ifdef __cplusplus
extern "C" {
#endif

// 约定：对象必然是值或引用的其中一个，且引用不为引用的引用
// 值被一层se_object_t包裹，data指向目标数据
// 引用被两层se_object_t包括，第一层指向引用的se_object_t，第二层指向目标数据
// 未被引用的值在语句结束后应被清除
// 引用被置nil时，被引用对象引用计数递减
// 引用的id总是0，未被引用的值的id也是零，值在被引用后，它的id应该成为任意唯一的正数
// *附加：以上任何与ref.h描述有冲突的部分，以ref.h为准

se_object_t wrap2obj(void *data, int type);
se_object_t objclone(se_object_t obj);
const char* obj2str(se_object_t obj, char *buffer, int len);

#ifdef __cplusplus
}
#endif