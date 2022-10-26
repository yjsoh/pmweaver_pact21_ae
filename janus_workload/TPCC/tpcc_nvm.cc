/*
 Author: Vaibhav Gogte <vgogte@umich.edu>
		 Aasheesh Kolli <akolli@umich.edu>


This file models the TPCC benchmark.
*/

// Korakit
// remove MT stuffs
//#include <pthread.h>
#include <iostream>
#include <vector>
#include <sys/time.h>
#include <string>
#include <fstream>
#include "common.h"
#include "tpcc_db.h"

#define NUM_ORDERS 100 // 10000000
#define NUM_THREADS 1

#define NUM_WAREHOUSES 1
#define NUM_ITEMS 100 // 10000
#define NUM_LOCKS NUM_WAREHOUSES * 10 + NUM_WAREHOUSES *NUM_ITEMS
TPCC_DB *tpcc_db[NUM_THREADS];

void initialize(int tid)
{
	tpcc_db[tid] = (TPCC_DB *)aligned_malloc(64, sizeof(TPCC_DB));
	new (tpcc_db[tid]) TPCC_DB();
	tpcc_db[tid]->initialize(NUM_WAREHOUSES, NUM_THREADS);
	// fprintf(stderr, "Created tpcc at %p\n", (void *)tpcc_db[tid]);
}

void *new_orders(void *arguments)
{
	int thread_id = *((int *)arguments);
	for (int i = 0; i < NUM_ORDERS / NUM_THREADS; i++)
	{
		int w_id = 1;

		int d_id = tpcc_db[thread_id]->get_random(thread_id, 1, 10);
		int c_id = tpcc_db[thread_id]->get_random(thread_id, 1, 3000);

		tpcc_db[thread_id]->new_order_tx(thread_id, w_id, d_id, c_id);
	}

	return NULL;
}

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		fprintf(stderr, "%s N_WAREHOUSE NTHREADS DURATION\n", argv[0]);
		return -1;
	}

	uint64_t nwarehouse = atoi(argv[1]);
	uint64_t nitems = atoi(argv[2]);
	uint64_t nthreads = atoi(argv[3]);
	uint64_t duration = atoi(argv[4]);

	uint64_t nlocks = nwarehouse * 10 + nwarehouse * nitems;

	for (int i = 0; i < nthreads; i++)
	{
		initialize(i);
	}

	pthread_t threads[nthreads];
	int id[nthreads];

	for (int i = 0; i < nthreads; i++)
	{
		id[i] = i;
		new_orders((void *)&id[i]);
	}

	return 0;
}
