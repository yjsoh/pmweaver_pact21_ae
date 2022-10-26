#include <stdlib.h>
#include <string.h>

void *agr_pmem_memset(void *s, int c, size_t n);
void *agr_pmem_memcpy(void *dest, const void *src, size_t n);
void *pmem_alloc(size_t size);
void *agr_memalign(size_t align, size_t size);