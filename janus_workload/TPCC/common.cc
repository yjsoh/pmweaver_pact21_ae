#include "common.h"
#include "stdio.h"
#include <libpmem.h>
#include <assert.h>
#include <stdio.h>

#define PMEMFILE "/mnt/ramdisk/tpcc"

void *pmem_base_addr = NULL;

void *pmalloc(size_t length)
{
	if (pmem_base_addr == NULL)
	{
		size_t filesize = (1UL << 34);
		pmem_base_addr = pmem_map_file(PMEMFILE, filesize, PMEM_FILE_CREATE, 0x666, 0, 0);
		if (pmem_base_addr == NULL)
		{
			fprintf(stderr, "[%s] Error %s, %lu", __func__, PMEMFILE, filesize);
			perror("pmem_map_file");
		}
		assert(pmem_base_addr != nullptr);
	}
	void *toRet = pmem_base_addr;
	pmem_base_addr = ((char *)pmem_base_addr) + length;
	return toRet;
}

void cache_flush(void *addr, size_t size)
{
#if PMEM_NO_FLUSH == 0
	uint64_t startAddr = (((uint64_t)addr) >> 6) * 64UL;
	uint64_t endAddr = (((uint64_t)addr + (uint64_t)size + 63UL) >> 6) * 64UL;

	for (uint64_t curAddr = startAddr; curAddr < endAddr; curAddr += 64UL)
	{
		_mm_clwb((void *)curAddr);
	}
#endif
}

void flush_caches(void *addr, size_t size)
{
#if PMEM_NO_FLUSH == 0
	cache_flush(addr, size);
#endif
}

void s_fence(void)
{
	_mm_sfence();
}
