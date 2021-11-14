#include <se/alloc.h>
#include <malloc.h>
#include <string.h>
#include <assert.h>

#define MEM_UNIT_SIZE 8 // 内存单元大小（字节）
#define MEM_INIT_SIZE 32 // 初始内存池大小（内存单元）

typedef struct memblock_s
{
	size_t  size;            // 内存池大小（单元数量）
	size_t  used;            // 已分配的内存段数量（内存段清零时当前指针指向初位置）
	uint8_t *cur;            // 内存块当前位置
	uint8_t *end;            // 内存块末尾
	struct memblock_s *next; // 下一个内存块
} memblock_t;

typedef struct mempool_s
{
	memblock_t *head;       // 初始内存块
	memblock_t *current;    // 当前内存块
	size_t id;              // 内存池编号
	struct mempool_s *next; // 下一个内存池
} mempool_t;

static int g_current_allocator      = 0;  // 使用标准库malloc/free
static mempool_t *g_mempool_root    = 0L; // 内存池链表
static mempool_t *g_mempool_current = 0L; // 当前内存池指针

static size_t se_msize_by_allocator(void *mptr)
{
	assert(g_current_allocator != 0);
	assert(g_mempool_current != 0L);

	memblock_t *mp = g_mempool_current->head;
	assert(mp != 0L);
	assert(mp->size != 0);

	uint8_t *p = (uint8_t*)mptr;

	while (mp != 0L)
	{
		const size_t bytelen = mp->size * MEM_UNIT_SIZE;
		if (mp->end - p + 1 <= bytelen - sizeof(size_t))
		{
			return *((size_t*)p - 1);
		}
		mp = mp->next;
	}

	return 0;
}

// 一次性分配超大块内存可能导致程序崩溃
static void* se_alloc_by_allocator(size_t size)
{
	assert(g_current_allocator != 0);
	assert(g_mempool_current != 0L);

	memblock_t *mp = g_mempool_current->current;
	assert(mp != 0L);
	assert(mp->size != 0);

	while (1)
	{
		if (mp->end - mp->cur >= size + sizeof(size_t))
		{	// 当前内存块大小足够
			++mp->used;
			*(size_t*)mp->cur = size;
			void *mem = mp->cur + sizeof(size_t);
			mp->cur += sizeof(size_t) + size;
			return mem;
		}

		if (mp->next == 0L) break;
		mp = mp->next;
	}

	// 内存分配失败，拓展内存池大小
	size_t next_size = mp->size * 2;
	while (next_size * MEM_UNIT_SIZE < size + sizeof(size_t))
	{
		next_size *= 2;
	}

	mp->next = (memblock_t*)malloc(sizeof(memblock_t));
	assert(mp->next != 0L);

	mp = mp->next;
	mp->size = next_size;
	mp->used = 0;
	mp->next = 0L;

	mp->cur = (uint8_t*)malloc(mp->size * MEM_UNIT_SIZE);
	mp->end = mp->cur + mp->size * MEM_UNIT_SIZE - 1;
	assert(mp->cur != 0L);

	g_mempool_current->current = mp;

	++mp->used;
	*(size_t*)mp->cur = size;
	void *mem = mp->cur + sizeof(size_t);
	mp->cur += sizeof(size_t) + size;
	return mem;
}

static int se_free_by_allocator(void *mptr)
{
	assert(g_current_allocator != 0);
	assert(g_mempool_current != 0L);

	memblock_t *mp = g_mempool_current->head;
	assert(mp != 0L);
	assert(mp->size != 0);

	uint8_t *p = (uint8_t*)mptr;

	while (mp != 0L)
	{
		const size_t bytelen = mp->size * MEM_UNIT_SIZE;
		if (mp->end - p + 1 <= bytelen - sizeof(size_t))
		{
			assert(mp->used > 0);
			*((size_t*)p - 1) = 0;
			--mp->used;
			if (mp->used == 0)
			{	// 重置该内存块
				mp->cur = mp->end - bytelen;
				g_mempool_current->current = mp;
			}
			return 0; // 释放成功
		}
		mp = mp->next;
	}

	return 1;
}

static void* se_realloc_by_allocator(void *mptr, size_t size)
{
	assert(g_current_allocator != 0);
	assert(g_mempool_current != 0L);
	assert(mptr != 0L);
	assert(size != 0);

	void *p = se_alloc_by_allocator(size);
	assert(p != 0L);

	memcpy(p, mptr, size);

	size_t oldsize = *((size_t*)mptr - 1);

	int state = se_free_by_allocator(mptr);
	assert(state == 0);

	if (size > oldsize)
	{
		memset((uint8_t*)p + oldsize, 0, size - oldsize);
	}

	return p;
}

// 释放所有内存池
void se_alloc_cleanup()
{
	mempool_t *ppool = g_mempool_root;
	memblock_t *pblock;

	while (ppool != 0L)
	{
		pblock = ppool->head;
		while (pblock != 0L)
		{
			void *p = pblock->end - pblock->size * MEM_UNIT_SIZE + 1;
			free(p);
			memblock_t *tmp = pblock->next;
			free(pblock);
			pblock = tmp;
		}
		ppool = ppool->next;
	}

	g_mempool_current = 0L;
	g_mempool_root = 0L;
}

int se_allocator_create(int wanted_id)
{
	mempool_t dummy, *ppool = &dummy;
	dummy.next = g_mempool_root;
	dummy.id = 0;

	while (ppool->next != 0)
	{
		if (wanted_id == ppool->id)
		{
			wanted_id = 0;
		}
		ppool = ppool->next;
	}

	int id = wanted_id == 0 ? ppool->id + 1 : wanted_id;
	assert(id != 0);

	ppool->next = (mempool_t*)malloc(sizeof(mempool_t));
	ppool = ppool->next;
	assert(ppool != 0L);

	memblock_t *pblock = (memblock_t*)malloc(sizeof(memblock_t));
	assert(pblock != 0L);

	pblock->size = MEM_INIT_SIZE;
	pblock->used = 0;
	pblock->cur  = (uint8_t*)malloc(pblock->size * MEM_UNIT_SIZE);
	pblock->end  = pblock->cur + pblock->size * MEM_UNIT_SIZE - 1;
	pblock->next = 0L;
	assert(pblock->cur != 0);

	ppool->id      = id;
	ppool->next    = 0L;
	ppool->head    = pblock;
	ppool->current = ppool->head;

	if (g_mempool_root == 0L)
	{
		g_mempool_root = ppool;
	}

	return id;
}

int se_allocator_destroy(int allocator_id)
{
	// 拒绝销毁默认内存分配器malloc/free
	if (allocator_id == 0) return 1;

	// 拒绝销毁当前内存分配器
	if (g_current_allocator == allocator_id) return 1;

	mempool_t dummy, *ppool = &dummy;
	dummy.next = g_mempool_root;

	while (ppool->next != 0L)
	{
		if (ppool->next->id == allocator_id) break;
		ppool = ppool->next;
	}

	if (ppool->next->id == allocator_id)
	{
		memblock_t *pblock = ppool->next->head;

		while (pblock != 0L)
		{
			void *p = pblock->end - pblock->size * MEM_UNIT_SIZE + 1;
			free(p);
			memblock_t *tmp = pblock->next;
			free(pblock);
			pblock = tmp;
		}

		mempool_t *tmp = ppool->next->next;
		free(ppool->next);

		if (ppool->next == g_mempool_root)
		{
			g_mempool_root = tmp;
		}

		ppool->next = tmp;

		return 0;
	}

	return 1;
}

int se_allocator_set(int allocator_id)
{
	if (allocator_id == g_current_allocator)
	{
		return 0;
	}

	if (allocator_id == 0)
	{
		se_allocator_restore();
		return 0;
	}

	mempool_t *ppool = g_mempool_root;

	while (ppool != 0L)
	{
		if (ppool->id == allocator_id)
		{
			g_mempool_current = ppool;
			g_current_allocator = allocator_id;
			return 0;
		}
		ppool = ppool->next;
	}

	return 1;
}

int se_current_allocator()
{
	return g_current_allocator;
}

void se_allocator_restore()
{
	g_current_allocator = 0;
}

void* se_alloc(size_t size)
{
	return g_current_allocator == 0
		? malloc(size) : se_alloc_by_allocator(size);
}

void* se_realloc(void *pb, size_t size)
{
	return g_current_allocator == 0
		? realloc(pb, size) : se_realloc_by_allocator(pb, size);
}

void se_free(void *pb)
{
	g_current_allocator == 0 ? free(pb) : se_free_by_allocator(pb);
}

size_t se_msize(void *pb)
{
	return g_current_allocator == 0
		? _msize(pb) : se_msize_by_allocator(pb);
}