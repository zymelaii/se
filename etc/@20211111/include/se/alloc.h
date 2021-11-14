#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void* se_alloc(size_t size);
void* se_realloc(void *pb, size_t size);
void  se_free(void *pb);

void* se_memdup(void *pb);

#ifdef __cplusplus
}
#endif