#pragma once

#include <se/type.h>
#include <se/priority.h>
#include <se/parser.h>
#include <se/exception.h>
#include <stddef.h>

#define ECTX_UNLOAD  0 // seus指令未加载
#define ECTX_UNBUILD 1 // seus指令未构建
#define ECTX_DONE    2 // seus执行完毕
#define ECTX_ERROR   3 // seus语句错误
#define ECTX_WAIT    4 // seus待执行

typedef struct se_context_s
{
	seus_t seus; // current se unit stream
	int   state; // seus state

	token_t *raw_tokens;
	size_t   ntokens;

	void *symbols; // HashMap { Symbol<string>: id<int> }
	void *memory;  // runtime storage

	const char *next_statement;
} se_context_t;

#ifdef __cplusplus
extern "C" {
#endif

// 以下函数，成功返回0，否则返回非零值
int se_ctx_create  (se_context_t *ctx); // 创建环境
int se_ctx_destroy (se_context_t *ctx); // 销毁环境
int se_ctx_load    (se_context_t *ctx, const char *script); // 载入SE代码（若代码已经存在，则向后连接）
int se_ctx_complete(se_context_t *ctx); // 判断代码是否全部执行完毕
int se_ctx_forward (se_context_t *ctx); // 读取下一个语句
int se_ctx_parse   (se_context_t *ctx); // 解析当前语句并构建SEUS
int se_ctx_compile (se_context_t *ctx); // 编译全部代码（未实现）
int se_ctx_onestep (se_context_t *ctx, unit_t *unit); // 单步执行
int se_ctx_execute (se_context_t *ctx); // 执行SEUS
int se_ctx_savetmp (se_context_t *ctx, void *data, int type, void **pp); // 保存临时值
int se_ctx_bind    (se_context_t *ctx, void *data, int type, const char *symbol); // 将数据绑定到对象
int se_ctx_unbind  (se_context_t *ctx, const char *symbol); // 对象解绑定
int se_ctx_sweep   (se_context_t *ctx); // 清理内存（未实现）

void* se_ctx_request(se_context_t *ctx, size_t size); // 请求一块内存
void  se_ctx_release(se_context_t *ctx, void *ptr);   // 释放从se_ctx_request请求的内存
const se_object_t* se_ctx_get_last_ret(se_context_t *ctx); // 获取上次的运行结果
se_object_t* se_ctx_find_by_id(se_context_t *ctx, uint16_t id); // 从id获取对象
se_object_t* se_ctx_find_by_symbol(se_context_t *ctx, const char *symbol); // 从符号获取对象

#ifdef __cplusplus
}
#endif