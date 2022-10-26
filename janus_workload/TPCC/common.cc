#include "common.h"
#include "stdio.h"
#include <libpmem.h>
#include <assert.h>
void *pmem_base_addr = NULL;
uint64_t current_offset = 0UL;

#ifdef GEM5
#define PMEM_START_ADDR (NULL)
#else
#define PMEM_START_ADDR (0x10000000000UL)
#endif

#define PMEMFILE "/mnt/ramdisk/tatp"
#define PMEMSIZE (1UL << 34)

void *mmap_persistent(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
	// printf("Using syscall id %d\n", MMAP_PERSISTENT);
	// return (void*)syscall(MMAP_PERSISTENT, start, length, prot, flags, fd, offset);
	return pmem_map_file(PMEMFILE, PMEMSIZE, PMEM_FILE_CREATE, 0x666, 0, 0);
}

void *pmalloc(size_t length)
{
	// void *ptr = (void*)NULL;
	// ptr = mmap_persistent((void*)PMEM_START_ADDR, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	// printf("Allocation at %p for size %zu returned %p\n", (void*)PMEM_START_ADDR, length, (void*)ptr);

	void *toRet = pmem_base_addr;
	pmem_base_addr = ((char *)pmem_base_addr) + length;
	return toRet;
}

/*
// We need this to bypass buggy pmalloc.
void *pmalloc(size_t length) {
//  return malloc(length);
  void* alloc_pmem_base = (void*)((uint64_t)pmem_base_addr +current_offset);
  current_offset += length;
  return alloc_pmem_base;
}
*/

void init_pmalloc()
{
	// pmem_base_addr = mmap_persistent(NULL, 1024UL*1024UL*1024UL, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
	pmem_base_addr = pmem_map_file(PMEMFILE, PMEMSIZE, PMEM_FILE_CREATE, 0x666, 0, 0);
	assert(pmem_base_addr != nullptr);
}

void *aligned_malloc(size_t alignment, size_t size)
{
	return (void *)(((uint64_t)pmalloc(size + alignment) + alignment - 1UL) / alignment * alignment);
}

void cache_flush(void *addr, size_t size)
{
	// uint64_t startAddr = (((uint64_t)addr) >> 6) * 64UL;
	// uint64_t endAddr = (((uint64_t)addr + (uint64_t)size + 63UL) >> 6) * 64UL;

	// for (uint64_t curAddr = startAddr; curAddr < endAddr; curAddr += 64UL)
	// {
	// 	_mm_clwb((void *)curAddr);
	// }
}

void flush_caches(void *addr, size_t size)
{
	// cache_flush(addr, size);
}

void s_fence(void)
{
	_mm_sfence();
}
