#pragma once

#include <se/type.h>
#include <se/priority.h>

// 说明：
// 1. se的内存分配包括字面量与对象
// 2. 字面量为非引用量，对象为引用量
// 3. 字面量独占一块内存，引用量维护引用并共享被引用者的内存
// 4. 字面量占用的内存是临时的，一个未被引用的字面量在生命周期结束时可能被清理
// 5. 字面量被引用时将进入引用计数，当引用计数清零后其可能被清理
// 6. 字面量包括空值（EO_NIL），数字（EO_NUM），函数（EO_FUNC）
// 7. 对象包括数组（EO_ARRAY）
// 8. 空值（nil）是不可见内置常量，不占用内存
// 9. 引用量包括只读引用量与读写引用量
// 10. 从赋值符（OP_ASS）创建的对象为只读引用量，从对象成员访问获得的对象为读写引用量
// 11. 创建引用的引用将被转换为只读引用量
// 12. 无引用字面量的结构 se_object_t { data, id = 0, type != EO_OBJ, refs = 0, is_nil }
// 13. 被引用字面量的结构 se_object_t { data, id != 0, type != EO_OBJ, refs > 0, is_nil }
// 14. 只读引用量的结构 se_object_t { data != null, id != 0, type = EO_OBJ, refs = 0, is_nil = false }
// 15. 读写引用量的结构 se_object_t
//	{
//		data = se_object_t { 只读引用量 },
//		id != 0,
//		type = EO_OBJ,
//		refs = 1,
//		is_nil = false
//	}
// 16. 若目标呈只读引用量结构，但id=0，则其为无引用字面量的包装，
//	该量不应直接作为以下任意函数的参数，应当去除包装后再执行相关操作

typedef struct refreq_s
{	// 创建引用所需要的信息
	uint16_t    reqsize;  // 请求的内存大小
	uint16_t    writable; // 引用是否可写
	void        *reqmem;  // 请求的内存，手动分配并存入
	se_object_t *placer;  // 对象存储的地址，必须包含有效id，若为null，则需手动分配
} refreq_t;

static inline int is_writable(se_object_t *obj)
{
	if (obj->type != EO_OBJ || obj->refs != 1) return 0;
	obj = (se_object_t*)obj->data;
	return obj->type == EO_OBJ && obj->refs == 0;
}

static inline int is_readonly(se_object_t *obj)
{
	if (obj->type != EO_OBJ || obj->refs != 0) return 0;
	obj = (se_object_t*)obj->data;
	return obj->type != EO_OBJ && obj->refs > 0;
}

static inline int is_reftarget(se_object_t *obj)
{
	return obj->type != EO_OBJ;
}

#ifdef __cplusplus
extern "C" {
#endif

refreq_t se_ref_request(se_object_t *obj, int writable); // 获取引用请求信息
se_object_t se_refer(se_object_t *obj, refreq_t *req); // 创建引用

#ifdef __cplusplus
}
#endif