#include <se/context.h>
#include <se/type.h>
#include <se/alloc.h>
#include <se/exception.h>
#include <se/ref.h>
#include <time.h>
#include <assert.h>
#include <string.h>

#define N_HSAHMAP_SIZE 12
static const size_t g_hashmap_size[N_HSAHMAP_SIZE] =
{	// 推荐的哈希表大小
	53, 97, 193, 389,
	769, 1543, 3079, 6151,
	12289, 24593, 49157, 98317,
};

typedef struct s2inode_s
{
	const char *str; // key
	uint16_t id;     // value
	struct s2inode_s *next;
} s2inode_t;

// 挂链哈希表：符号→ID
// 当哈希表密度过大时将尝试拓展表大小进行重散列
typedef struct hashmap_s
{
	size_t size_id; // 哈希表大小
	size_t used; // 已使用表格
	s2inode_t *table;
} hashmap_t;

// id分配结构
typedef struct idlist_s
{
	int16_t ids[8];
	int16_t size;
	struct idlist_s *next;
} idlist_t;

// se_context_t.momery 结构
typedef struct ctxmemory_s
{
///-------- mempool id --------
	int mempool_id;             // 参考alloc.h
///-------- symbol map  --------
	hashmap_t symmap;           // 符号对id映射表
	s2inode_t **nilsym_pairs;   // 无效符号列表
	size_t nilsym_size;         // 无效符号列表长度
	size_t nilsym_capacity;     // 无效符号列表容量
///-------- script origin --------
	char *start_of_statement;   // 语句起始地址
///-------- id allocator --------
	uint16_t prev_available_id; // 递增id值
	idlist_t *idlist;           // 可用id值
///-------- reference storage --------
	se_object_t *idstorage;     // 持续对象储存空间（以id为下标访问，存活的唯一标准是对象id与下标一致）
	size_t idstorage_capacity;  // 持续空间容量
///-------- literal storage --------
	se_object_t *blcstorage;    // 过期对象储存空间（beyond life-cycle）
	size_t blcstorage_size;     // 对象数
	size_t blcstorage_capacity; // 储存容量
///-------- runtime --------
	int ssp;                    // 括号域状态下标指针
	se_stack_t efs;             // 元素帧栈
	se_stack_t vfs;             // 移动帧栈
	se_object_t result;         // 上一次的执行结果（is_nil=1即结果不存在）
} ctxmemory_t;

#define SE_CONTEXT_BUILD
#include "ctxinternal.c"
#include "hashmap.c"
#include "action.c"

int se_ctx_create(se_context_t *ctx)
{
	if (ctx == 0L)
	{
		return 1;
	}

	memset(ctx, 0, sizeof(se_context_t));

	int old_mempool_id = se_current_allocator();
	int id = se_allocator_create(time(0L));
	se_allocator_set(id);

	ctxmemory_t *ctxmem = (ctxmemory_t*)se_alloc(sizeof(ctxmemory_t));
	assert(ctxmem != 0L);
	memset(ctxmem, 0, sizeof(ctxmemory_t));

	ctxmem->mempool_id = id;
	ctxmem->symmap.size_id = 0;
	ctxmem->symmap.used = 0;

	const int size = g_hashmap_size[ctxmem->symmap.size_id];
	s2inode_t *table = (s2inode_t*)se_alloc(size * sizeof(s2inode_t));
	assert(table != 0L);

	memset(table, 0, size * sizeof(s2inode_t));
	ctxmem->symmap.table = table;
	ctxmem->nilsym_capacity = 32;
	ctxmem->nilsym_pairs = (s2inode_t**)se_alloc(
		sizeof(s2inode_t*) * ctxmem->nilsym_capacity);
	assert(ctxmem->nilsym_pairs != 0L);

	ctxmem->start_of_statement = 0L;

	ctxmem->prev_available_id = 0;
	ctxmem->idlist = 0L;

	ctxmem->idstorage_capacity = 32;
	ctxmem->idstorage = (se_object_t*)se_alloc(
		sizeof(se_object_t) * ctxmem->idstorage_capacity);
	assert(ctxmem->idstorage != 0L);
	memset(ctxmem->idstorage, 0,
		sizeof(se_object_t) * ctxmem->idstorage_capacity);

	ctxmem->blcstorage_capacity = 32;
	ctxmem->blcstorage = (se_object_t*)se_alloc(
		sizeof(se_object_t) * ctxmem->blcstorage_capacity);
	assert(ctxmem->blcstorage != 0L);

	ctx->symbols = &ctxmem->symmap;

	ctx->memory = ctxmem;

	se_allocator_set(old_mempool_id);

	return 0;
}

int se_ctx_destroy(se_context_t *ctx)
{
	if (ctx == 0L)
	{
		return 1;
	}

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	if (ctxmem == 0L)
	{
		return 1;
	}

	if (se_current_allocator() == ctxmem->mempool_id)
	{
		se_allocator_restore();
	}

	int state = se_allocator_destroy(ctxmem->mempool_id);
	assert(state == 0);

	return 0;
}

int se_ctx_load(se_context_t *ctx, const char *script)
{
	assert(ctx != 0L);
	assert(ctx->memory != 0L);

	if (script == 0L)
	{
		return 1;
	}

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	int offset = 0;
	if (ctx->next_statement != 0L)
	{
		offset = ctxmem->start_of_statement - ctx->next_statement;
	}

	int old_mempool_id = se_current_allocator();
	se_allocator_set(ctxmem->mempool_id);

	const size_t len2 = strlen(script);
	if (ctx->next_statement == 0L)
	{
		if (ctxmem->start_of_statement != 0L)
		{
			se_free(ctxmem->start_of_statement);
		}
		ctxmem->start_of_statement = (char*)se_alloc(len2 + 1);
		assert(ctxmem->start_of_statement != 0L);
		strcpy(ctxmem->start_of_statement, script);
		ctx->next_statement = ctxmem->start_of_statement;
	} else
	{
		const size_t len1 = strlen(ctxmem->start_of_statement);
		ctxmem->start_of_statement = (char*)se_realloc(ctxmem->start_of_statement, len1 + len2 + 2);
		ctx->next_statement = ctxmem->start_of_statement + offset;
		char *p = ctxmem->start_of_statement;
		p[len1] = ';';
		memcpy(p + len1 + 1, script, len2 + 1);
	}

	se_allocator_set(old_mempool_id);

	return 0;
}

int se_ctx_complete(se_context_t *ctx)
{
	assert(ctx != 0L);
	assert(ctx->memory != 0L);

	if (ctx->state == ECTX_WAIT) return 1;

	if (ctx->next_statement == 0L) return 0;

	return 1;
}

int se_ctx_forward(se_context_t *ctx)
{
	assert(ctx != 0L);

	if (ctx->state == ECTX_WAIT || ctx->state == ECTX_UNBUILD)
	{
		return 1;
	}

	ctx->state = ECTX_UNLOAD;

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);
	int old_mempool_id = se_current_allocator();
	se_allocator_set(ctxmem->mempool_id);

	while (ctx->next_statement != 0L)
	{
		ctx->next_statement = str2tokens(
			ctx->next_statement, &ctx->raw_tokens, (int*)&ctx->ntokens);

		if (!se_caught())
		{
			se_allocator_set(old_mempool_id);
			return 1;
		}

		if (ctx->ntokens != 0)
		{
			ctx->state = ECTX_UNBUILD;
			se_allocator_set(old_mempool_id);
			return 0;
		}
	}

	se_allocator_set(old_mempool_id);
	return 1;
}

int se_ctx_parse(se_context_t *ctx)
{
	assert(ctx != 0L);

	if (ctx->state != ECTX_UNBUILD)
	{
		return 1;
	}

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	int old_mempool_id = se_current_allocator();
	se_allocator_set(ctxmem->mempool_id);

	se_free(&ctx->seus);

	unit_t *rpn = 0L;
	int     nrp = 0;

	rpn = toks2rpn(ctx->raw_tokens, ctx->ntokens, &nrp);

	if (!se_caught())
	{
		ctx->state = ECTX_ERROR;
		se_allocator_set(old_mempool_id);
		return 0;
	}

	ctx->seus = rpn2seus(rpn, nrp);

	if (!se_caught())
	{
		ctx->state = ECTX_ERROR;
		se_allocator_set(old_mempool_id);
		return 0;
	}

	ctx->state = ECTX_WAIT;
	se_allocator_set(old_mempool_id);
	return 0;
}

int se_ctx_compile(se_context_t *ctx); // 编译全部代码

int se_ctx_onestep(se_context_t *ctx, unit_t *unit)
{	// 单步执行，映射动作
	assert(ctx != 0L);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	if (ctx->state != ECTX_WAIT)
	{
		return 1;
	}

	assert(unit != 0L);
	assert(ctxmem->efs.stack != 0L);
	assert(ctxmem->vfs.stack != 0L);

	int old_mempool_id = se_current_allocator();
	se_allocator_set(ctxmem->mempool_id);

	int type = SE_UNIT_TYPE(*unit);
	int subtype = SE_UNIT_SUBTYPE(*unit);

	if (type == T_SYMBOL)
		se_ctx_action_assign_symbol(ctx, unit);
	else if (type == T_NUMBER)
		se_ctx_action_assign_number(ctx, unit);
	else switch (subtype)
	{
		case OP_BRE_S:
		case OP_ARG_S:
		case OP_IDX_S:
		case OP_ARR_S:  ctx->seus.ss[++ctxmem->ssp] = (scopestate_t){ (int)ctxmem->efs.size, 0 }; break;
		case OP_BRE:    se_ctx_action_bracketval(ctx, unit);   break;
		case OP_ARG:    se_ctx_action_fncall(ctx, unit);       break;
		case OP_IDX:    se_ctx_action_index(ctx, unit);        break;
		case OP_ARR:    se_ctx_action_makearray(ctx, unit);    break;
		case OP_ASS:    se_ctx_action_assign (ctx, unit);      break;
		case OP_CME:    se_ctx_action_mov2vfs(ctx, unit);      break;
		case OP_EPA:    se_ctx_action_exparray(ctx, unit);     break;
		case OP_PL:
		case OP_NL:     se_ctx_action_sign(ctx, unit);         break;
		case OP_ADD:
		case OP_SUB:
		case OP_MOD:
		case OP_MUL:
		case OP_DIV:    se_ctx_action_basecalc(ctx, unit);     break;
		case OP_GTR:
		case OP_GEQ:
		case OP_LSS:
		case OP_LEQ:
		case OP_EQU:
		case OP_NEQ:
		case OP_LAND:
		case OP_LOR:    se_ctx_action_compare(ctx, unit);      break;
		case OP_LNOT:   se_ctx_action_logical_not(ctx, unit);  break;
		case OP_NOT:    se_ctx_action_bitwise_not(ctx, unit);  break;
		case OP_LSH:
		case OP_RSH:
		case OP_AND:
		case OP_XOR:
		case OP_OR:     se_ctx_action_binary_bitop(ctx, unit); break;
		case OP_ADD_ASS:
		case OP_SUB_ASS:
		case OP_MOD_ASS:
		case OP_MUL_ASS:
		case OP_DIV_ASS:
		case OP_LSH_ASS:
		case OP_RSH_ASS:
		case OP_AND_ASS:
		case OP_XOR_ASS:
		case OP_OR_ASS: se_ctx_action_calc_and_ass(ctx, unit); break;
	}

	se_allocator_set(old_mempool_id);

	return !se_caught();
}

int se_ctx_execute(se_context_t *ctx)
{
	assert(ctx != 0L);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	if (ctx->state != ECTX_WAIT)
	{
		return 1;
	}

	int old_mempool_id = se_current_allocator();
	se_allocator_set(ctxmem->mempool_id);

	se_stack_free(&ctxmem->efs);
	se_stack_free(&ctxmem->vfs);

	ctxmem->efs = se_stack_create(ctx->seus.nef);
	ctxmem->vfs = se_stack_create(ctx->seus.nvf);

	ctxmem->ssp = -1;
	ctx->seus.ss[++ctxmem->ssp] = (scopestate_t){ 0, 0 };

	int i = 0;
	for (; i < ctx->seus.nus; ++i)
	{
		if (se_ctx_onestep(ctx, ctx->seus.us + i) != 0)
		{
			se_allocator_set(old_mempool_id);
			ctx->state = ECTX_ERROR;
			return 1;
		}
	}

	ctxmem->result = se_stack_pop(&ctxmem->efs);
	ctx->state = ECTX_DONE;

	se_allocator_set(old_mempool_id);

	return 0;
}

///-------- api --------
int se_ctx_savetmp(se_context_t *ctx, void *data, int type, void **pp)
{
	assert(ctx != 0L);
	assert(pp != 0L);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	if (data == 0L || type == EO_NIL)
	{
		return 1;
	}

	size_t size;
	switch (type)
	{
		case EO_OBJ  : size = sizeof(se_object_t);   break;
		case EO_NUM  : size = sizeof(se_number_t);   break;
		case EO_FUNC : size = sizeof(se_function_t); break;
		case EO_ARRAY: size = sizeof(se_array_t);    break;
		default: assert(0);
	}

	void *p = se_ctx_request(ctx, size);
	if (p == 0L)
	{
		se_throw(RuntimeError, BadAlloc, size, 0);
		return 1;
	}
	memcpy(p, data, size);

	*pp = p;

	return 0;
}

int se_ctx_bind(se_context_t *ctx, void *data, int type, const char *symbol); // 将数据绑定到对象
int se_ctx_unbind(se_context_t *ctx, const char *symbol); // 对象解绑定

int se_ctx_sweep(se_context_t *ctx)
{
	
}

void* se_ctx_request(se_context_t *ctx, size_t size)
{
	assert(ctx != 0L);

	if (size == 0) return 0L;

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	int old_mempool_id = se_current_allocator();
	se_allocator_set(ctxmem->mempool_id);

	void *ret = se_alloc(size);
	assert(ret != 0L);

	se_allocator_set(old_mempool_id);

	return ret;
}

void se_ctx_release(se_context_t *ctx, void *ptr)
{
	assert(ctx != 0L);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	int old_mempool_id = se_current_allocator();
	se_allocator_set(ctxmem->mempool_id);

	se_free(ptr);

	se_allocator_set(old_mempool_id);
}

const se_object_t* se_ctx_get_last_ret(se_context_t *ctx)
{
	assert(ctx != 0L);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	return ctxmem->result.is_nil == 1 ? 0L : &ctxmem->result;
}

se_object_t* se_ctx_find_by_id(se_context_t *ctx, uint16_t id)
{
	assert(ctx != 0L);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	assert(ctxmem->idstorage != 0L);

	if (id > ctxmem->idstorage_capacity) return 0L;

	se_object_t *obj = &ctxmem->idstorage[id - 1];

	return obj->id == id ? 0L : obj;
}

se_object_t* se_ctx_find_by_symbol(se_context_t *ctx, const char *symbol)
{
	assert(ctx != 0L);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	s2inode_t *result = hashmap_find_by_key(&ctxmem->symmap, symbol);

	if (result == 0L) return 0L;

	return se_ctx_find_by_id(ctx, result->id);
}