/*
 Author: Vaibhav Gogte <vgogte@umich.edu>
		 Aasheesh Kolli <akolli@umich.edu>


This file models the TPCC benchmark.
*/

// Korakit
// remove MT stuffs
//#include <pthread.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include <sys/time.h>
#include <string>
#include <fstream>
#include <assert.h>
#include "common.h"
#include "tpcc_db.h"

std::atomic<bool> stop;
uint64_t new_orders(uint64_t tid);

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


/************************************************************/
// Following from Mirror work
barrier_t barrier_global;
barrier_t init_barrier;

static uint8_t cores[] = {
	1, 3, 5, 7, 9, 11, 13, 15,
	0, 2, 4, 6, 8, 10, 12, 14};

static inline void set_cpu(int cpu)
{
	assert(cpu > -1);
	int n_cpus = sysconf(_SC_NPROCESSORS_ONLN);
	if (cpu < n_cpus)
	{
		int cpu_use = cores[cpu];
		cpu_set_t mask;
		CPU_ZERO(&mask);
		CPU_SET(cpu_use, &mask);
		pthread_t thread = pthread_self();
		if (pthread_setaffinity_np(thread, sizeof(cpu_set_t), &mask) != 0)
		{
			fprintf(stderr, "Error setting thread affinity\n");
		}
	}
}


struct thread_data
{
	uint64_t tid;
	uint64_t ops;
};

/* An operation every thread runs during the execution */
void *threadRun(void *arg)
{
	struct thread_data *tData = (struct thread_data *)arg;
	uint64_t tid = tData->tid;
	uint64_t ops = tData->ops;

	// Set CPU affinity
	set_cpu(tid);

#ifdef _ENABLE_AGR
	while (!((bool)my_context_void()))
	{
	}
	// fprintf(stdout, "threadRun (ID:%lu)\n", (uint64_t)my_context_void());
#endif

	barrier_cross(&init_barrier);
	barrier_cross(&barrier_global);

	tData->ops = new_orders(tid);

	return NULL;
}

void run(char *argv[], uint64_t nwarehouse, uint64_t nitems, uint64_t nthreads, uint64_t duration)
{
	pthread_t threads[nthreads];
	thread_data allThreadsData[nthreads];
	struct timespec tv_start, tv_end;
	std::ofstream fexec;

	/* Init */
	fexec.open("tpcc.csv", std::ios_base::app);
	barrier_init(&barrier_global, nthreads + 1);
	barrier_init(&init_barrier, nthreads);
	for(uint64_t i = 0; i < nthreads; i++)
	{
		allThreadsData[i].tid = i;
		allThreadsData[i].ops = 0;
	}
	stop = (false);

	/* Run */
	for (uint64_t i = 0; i < nthreads; i++)
	{
		pthread_create(&threads[i], NULL, threadRun, (void *)&allThreadsData[i]);
	}

	barrier_cross(&barrier_global);

	sleep(duration);

	stop = (true);
	for (uint64_t i = 0; i < nthreads; i++)
	{
		pthread_join(threads[i], NULL);
	}

	/* Collect */
	uint64_t totalOps = 0;
	for(uint64_t i = 0; i < nthreads; i++)
	{
		totalOps += allThreadsData[i].ops;
	}

	uint64_t tput = (uint64_t)((double)totalOps) / ((double)duration);
	double mtput = (double)tput / (1000000UL);

	uint64_t precision = 4;
	fexec << argv[0] << "," << std::to_string(nwarehouse) << "," << std::to_string(totalOps) << "," << std::to_string(nthreads) << "," << std::to_string(duration) << "," << std::to_string(tput) << std::endl;
	std::cout << argv[0] << "," << std::to_string(nwarehouse) << "," << std::to_string(totalOps) << "," << std::to_string(nthreads) << "," << std::to_string(duration) << "," << std::to_string(tput) << "," << std::setprecision(precision) << mtput << std::endl;
	std::cerr << argv[0] << "," << std::to_string(nwarehouse) << "," << std::to_string(totalOps) << "," << std::to_string(nthreads) << "," << std::to_string(duration) << "," << std::to_string(tput) << "," << std::setprecision(precision) << mtput << std::endl;

	fexec.close();
}

#define NUM_ORDERS 100 // 10000000
#define NUM_THREADS 1

#define NUM_WAREHOUSES 1
#define NUM_ITEMS 100 // 10000
#define NUM_LOCKS NUM_WAREHOUSES * 10 + NUM_WAREHOUSES *NUM_ITEMS
TPCC_DB **tpcc_db; // array of TPCC_DB, array length == nthreads

void init_db(uint64_t nthreads, uint64_t nwarehouse, uint64_t nitems)
{
	*tpcc_db = (TPCC_DB *)aligned_malloc(64, nthreads * sizeof(TPCC_DB));
	for (uint64_t tid = 0; tid < nthreads; tid++)
	{
		tpcc_db[tid] = new TPCC_DB();
		tpcc_db[tid]->initialize(nthreads, nwarehouse, nitems);
	}
}

void deinit_db(uint64_t nthreads)
{
	for (uint64_t tid = 0; tid < nthreads; tid++)
	{
		tpcc_db[tid]->deinitialize();
		delete tpcc_db[tid];
	}
	free(tpcc_db);
}

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
	if (argc != 5)
	{
		fprintf(stderr, "%s N_WAREHOUSE N_ITEMS NTHREADS DURATION\n", argv[0]);
		return -1;
	}

	uint64_t nwarehouse = atoi(argv[1]);
	uint64_t nitems = atoi(argv[2]);
	uint64_t nthreads = atoi(argv[3]);
	uint64_t duration = atoi(argv[4]);

	uint64_t nlocks = nwarehouse * 10 + nwarehouse * nitems;

	init_db(nthreads, nwarehouse, nitems);

	run(argv, nwarehouse, nitems, nthreads, duration);

	deinit_db(nthreads);

	return 0;
}
