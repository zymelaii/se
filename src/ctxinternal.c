#ifndef SE_CONTEXT_BUILD
#error ctxinternal.c is only available in context.c
#endif

// 分配id
static int se_ctx_allocid(se_context_t *ctx, uint16_t *id)
{
	assert(ctx != 0L);
	assert(id != 0L);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	if (ctxmem->idlist != 0L)
	{
		*id = ctxmem->idlist->ids[--ctxmem->idlist->size];
		idlist_t *next = ctxmem->idlist->next;
		if (ctxmem->idlist->size == 0 && next != 0L)
		{
			se_ctx_release(ctx, ctxmem->idlist);
			ctxmem->idlist = next;
		}
		return 0;
	} else if (ctxmem->prev_available_id < 0xffff)
	{
		*id = ++ctxmem->prev_available_id;
		return 0;
	} else
	{	// id溢出
		return 1;
	}
}

// 归还id
static int se_ctx_releaseid(se_context_t *ctx, uint16_t id)
{
	assert(ctx != 0L);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	if (id == 0 || id > ctxmem->prev_available_id)
	{
		return 1;
	}

	if (id == ctxmem->prev_available_id)
	{
		--ctxmem->prev_available_id;
		return 0;
	} else if (ctxmem->idlist->size == 8)
	{	// 当前空闲id表无多余位置
		idlist_t *root = se_ctx_request(ctx, sizeof(idlist_t));
		root->next = ctxmem->idlist;
		root->size = 0;
		ctxmem->idlist = root;
	}

	ctxmem->idlist->ids[ctxmem->idlist->size++] = id;

	return 0;
}

#include <stdio.h>
// 标记对象生命周期结束
static int se_ctx_mov2blc(se_context_t *ctx, se_object_t *obj)
{
	assert(ctx != 0L);
	assert(obj != 0L);

	ctxmemory_t *ctxmem = (ctxmemory_t*)ctx->memory;
	assert(ctxmem != 0L);

	if (ctxmem->blcstorage_size == ctxmem->blcstorage_capacity)
	{
		const size_t size = sizeof(se_object_t) * ctxmem->blcstorage_capacity;
		void *p = se_ctx_request(ctx, size * 2);
		if (p == 0L)
		{	// 移动失败
			return 1;
		}
		memcpy(p, ctxmem->blcstorage, size);
		ctxmem->blcstorage_capacity *= 2;
		se_ctx_release(ctx, ctxmem->blcstorage);
		ctxmem->blcstorage = (se_object_t*)p;
	}

	ctxmem->blcstorage[ctxmem->blcstorage_size++] = *obj;

	return 0;
}