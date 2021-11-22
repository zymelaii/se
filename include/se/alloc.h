#include <stddef.h>
#include <stdint.h>

// 1. 下列内存管理函数是针对se特化的内存池版本
// 2. 以下函数是线程不安全的
// 3. 推荐将se_alloc_cleanup注册给atexit
// 4. 以下函数是内存不安全的，错误的函数使用方法将导致无法预料的错误

#ifdef __cplusplus
extern "C" {
#endif

void se_alloc_cleanup(); // 清理内存环境（应当在程序结尾调用）

int se_allocator_create(int wanted_id); // 创建新的内存分配器，返回编号（尽可能按wantd_id分配）
int se_current_allocator(); // 返回当前使用的内存分配器编号
void se_allocator_restore(); // 重置为内置内存分配器

// 下列函数，返回0表示执行成功
int se_allocator_destroy(int allocator_id); // 销毁内存分配器，若销毁者恰为当前，则拒绝销毁
int se_allocator_set(int allocator_id); // 使用自定义的内存分配器

// 下列函数在当前内存分配器下生效
void* se_alloc(size_t size); // 申请内存
void* se_realloc(void *pb, size_t size); // 调整内存大小
void  se_free(void *pb); // 释放内存
size_t se_msize(void *pb); // 获取内存大小

#ifdef __cplusplus
}
#endif