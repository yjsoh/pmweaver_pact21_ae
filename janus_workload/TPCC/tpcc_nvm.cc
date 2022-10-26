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

/*
 *   File: barrier.h
 *   Author: Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>,
 *  	     Tudor David <tudor.david@epfl.ch>
 *   Description:
 *   barrier.h is part of ASCYLIB
 *
 * Copyright (c) 2014 Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>,
 * 	     	      Tudor David <tudor.david@epfl.ch>
 *	      	      Distributed Programming Lab (LPD), EPFL
 *
 * ASCYLIB is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef BARRIER_H
#define BARRIER_H

/* ################################################################### *
 * BARRIER
 * ################################################################### */

typedef struct barrier
{
	pthread_cond_t complete;
	pthread_mutex_t mutex;
	int count;
	int crossing;
} barrier_t;

static inline void barrier_init(barrier_t *b, int n)
{
	pthread_cond_init(&b->complete, NULL);
	pthread_mutex_init(&b->mutex, NULL);
	b->count = n;
	b->crossing = 0;
}

static inline void barrier_cross(barrier_t *b)
{
	pthread_mutex_lock(&b->mutex);
	/* One more thread through */
	b->crossing++;
	/* If not all here, wait */
	if (b->crossing < b->count)
	{
		pthread_cond_wait(&b->complete, &b->mutex);
	}
	else
	{
		pthread_cond_broadcast(&b->complete);
		/* Reset for next time */
		b->crossing = 0;
	}
	pthread_mutex_unlock(&b->mutex);
}

#endif /* BARRIER_H */

#define NUM_ORDERS 100 // 10000000
#define NUM_THREADS 1

#define NUM_WAREHOUSES 1
#define NUM_ITEMS 100 // 10000
#define NUM_LOCKS NUM_WAREHOUSES * 10 + NUM_WAREHOUSES *NUM_ITEMS
TPCC_DB **tpcc_db; // array of TPCC_DB, array length == nthreads

void init_db(uint64_t nthreads, uint64_t nwarehouse)
{
	*tpcc_db = (TPCC_DB *)aligned_malloc(64, nthread * sizeof(TPCC_DB));
	for (uint64_t tid = 0; tid < nthreads; tid++)
	{
		tpcc_db[tid] = new TPCC_DB();
		tpcc_db[tid]->initialize(nthreads, nwarehouse);
	}
}

void deinit_db()
{
	for (uint64_t tid = 0; tid < nthreads; tid++)
	{
		tpcc_db[tid]->deinitialize();
		delete tpcc_db[tid];
	}
	free(tpcc_db);
}

std::atomic<bool> stop;
uint64_t new_orders(uint64_t tid)
{
	uint64_t ops = 0;
	fprintf(stderr, "Running received\n");
	while (!stop)
	{
		int w_id = 1;

		int d_id = tpcc_db[tid]->get_random(tid, 1, 10);
		int c_id = tpcc_db[tid]->get_random(tid, 1, 3000);

		tpcc_db[tid]->new_order_tx(tid, w_id, d_id, c_id);

		ops++;
	}
	fprintf(stderr, "Stop received. Returning: %lu\n", ops);

	return ops;
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

	init_db(nthreads, nwarehouse);

	pthread_t threads[nthreads];
	int id[nthreads];

	for (int i = 0; i < nthreads; i++)
	{
		id[i] = i;
		new_orders((void *)&id[i]);
	}

	deinit_db();

	return 0;
}
