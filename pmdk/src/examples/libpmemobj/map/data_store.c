 /*
 * Copyright 2015-2018, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * data_store.c -- tree_map example usage
 */

#include <ex_common.h>
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "map.h"
#include "map_ctree.h"
#include "map_btree.h"
#include "map_rbtree.h"
#include "map_hashmap_atomic.h"
#include "map_hashmap_tx.h"
#include "map_hashmap_rp.h"
#include "map_skiplist.h"

//#include "/home/smahar/git/transparent_txopt/helper.h"
#include <sys/time.h>

#ifdef GEM5
	#include "gem5/m5ops.h"
#endif

POBJ_LAYOUT_BEGIN(data_store);
POBJ_LAYOUT_ROOT(data_store, struct store_root);
POBJ_LAYOUT_TOID(data_store, struct store_item);
POBJ_LAYOUT_END(data_store);

#define MAX_INSERTS 5000

static uint64_t nkeys;
static uint64_t keys[MAX_INSERTS];

struct store_item {
	uint64_t item_data;
};

struct store_root {
	TOID(struct map) map;
};

/*
 * new_store_item -- transactionally creates and initializes new item
 */
static TOID(struct store_item)
new_store_item(void)
{
	TOID(struct store_item) item = TX_NEW(struct store_item);
	D_RW(item)->item_data = rand();

	return item;
}

/*
 * get_keys -- inserts the keys of the items by key order (sorted, descending)
 */
static int
get_keys(uint64_t key, PMEMoid value, void *arg)
{
	keys[nkeys++] = key;

	return 0;
}

/*
 * dec_keys -- decrements the keys count for every item
 */
static int
dec_keys(uint64_t key, PMEMoid value, void *arg)
{
	nkeys--;
	return 0;
}

/*
 * parse_map_type -- parse type of map
 */
static const struct map_ops *
parse_map_type(const char *type)
{
	if (strcmp(type, "ctree") == 0)
		return MAP_CTREE;
	else if (strcmp(type, "btree") == 0)
		return MAP_BTREE;
	else if (strcmp(type, "rbtree") == 0)
		return MAP_RBTREE;
	else if (strcmp(type, "hashmap_atomic") == 0)
		return MAP_HASHMAP_ATOMIC;
	else if (strcmp(type, "hashmap_tx") == 0)
		return MAP_HASHMAP_TX;
	else if (strcmp(type, "hashmap_rp") == 0)
		return MAP_HASHMAP_RP;
	else if (strcmp(type, "skiplist") == 0)
		return MAP_SKIPLIST;
	return NULL;

}

int main(int argc, const char *argv[]) {
        printf("Starting something\n");
	if (argc < 3) {
		printf("usage: %s "
			"<ctree|btree|rbtree|hashmap_atomic|hashmap_rp|"
			"hashmap_tx|skiplist> file-name [nops]\n", argv[0]);
		return 1;
	}

	const char *type = argv[1];
	const char *path = argv[2];
	const struct map_ops *map_ops = parse_map_type(type);
	if (!map_ops) {
		fprintf(stderr, "invalid container type -- '%s'\n", type);
		return 1;
	}

	int nops = MAX_INSERTS;

	if (argc > 3) {
		nops = atoi(argv[3]);
		if (nops <= 0 || nops > MAX_INSERTS) {
			fprintf(stderr, "number of operations must be "
				"in range 1..%d\n", MAX_INSERTS);
			return 1;
		}
	}

	PMEMobjpool *pop;
	srand(0);

	if (file_exists(path) != 0) {
		if ((pop = pmemobj_create(path, POBJ_LAYOUT_NAME(data_store),
			PMEMOBJ_MIN_POOL, 0666)) == NULL) {
			perror("failed to create pool\n");
			return 1;
		}
	} else {
		if ((pop = pmemobj_open(path,
				POBJ_LAYOUT_NAME(data_store))) == NULL) {
			perror("failed to open pool\n");
			return 1;
		}
	}

	TOID(struct store_root) root = POBJ_ROOT(pop, struct store_root);

	struct map_ctx *mapc = map_ctx_init(map_ops, pop);
	if (!mapc) {
		perror("cannot allocate map context\n");
		return 1;
	}
	#ifdef GEM5
		printf("Begining work for workid %d\n", atoi(argv[4]));
		m5_work_begin(atoi(argv[4]),0);
	#endif
//	TraceBegin();
	/* insert random items in a transaction */
	int aborted = 0;
		map_create(mapc, &D_RW(root)->map, NULL);
		syscall(316, 0);
		for (int i = 0; i < nops; ++i) {
		    TX_BEGIN(pop) {
			/* new_store_item is transactional! */
			int val = rand();

			map_insert(mapc, D_RW(root)->map, val,
					new_store_item().oid);
		    } TX_ONABORT {
			perror("transaction aborted\n");
			map_ctx_free(mapc);
			aborted = 1;
		    } TX_END
 
		}
//		TraceEnd();
//	TraceEnd();
	#ifdef GEM5
		printf("Done for workid %d\n", atoi(argv[4]));
		m5_work_end(atoi(argv[4]),0);
//		m5_dump_stats(atoi(argv[4]), 0);
		return 0;
	#endif

	//! Exit

	if (aborted)
		return -1;

	/* count the items */
	map_foreach(mapc, D_RW(root)->map, get_keys, NULL);

	/* remove the items without outer transaction */
	for (int i = 0; i < nkeys; ++i) {
		PMEMoid item = map_remove(mapc, D_RW(root)->map, keys[i]);

		assert(!OID_IS_NULL(item));
		assert(OID_INSTANCEOF(item, struct store_item));
	}

	uint64_t old_nkeys = nkeys;

	/* tree should be empty */
	map_foreach(mapc, D_RW(root)->map, dec_keys, NULL);
	assert(old_nkeys == nkeys);

	map_ctx_free(mapc);
	pmemobj_close(pop);

	return 0;
}
