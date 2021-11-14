#include <se/alloc.h>
#include <malloc.h>
#include <string.h>

// to be achieved

void* se_alloc(size_t size)
{
	return malloc(size);
}

void* se_realloc(void *pb, size_t size)
{
	return realloc(pb, size);
}

void  se_free(void *pb)
{
	return free(pb);
}

void* se_memdup(void *pb)
{
	size_t size = _msize(pb);
	if (size == 0) return 0L;

	void *p = se_alloc(size);
	memcpy(p, pb, size);

	return p;
}