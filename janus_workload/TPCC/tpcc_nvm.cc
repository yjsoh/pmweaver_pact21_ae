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
uint64_t new_orders(uint64_t tid, uint64_t nwarehouse, uint64_t num_items);
uint64_t new_orders_nops(uint64_t tid, uint64_t nwarehouse, uint64_t nops, uint64_t num_items);

#ifdef _ENABLE_LIBPMEMOBJ
pmem::obj::pool<TPCC_DB> pool;
pmem::obj::persistent_ptr<TPCC_DB> table;
#endif

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
	uint64_t nops; // if execute exactly nops
	uint64_t nwarehouse;
	uint64_t num_items;
};

/* An operation every thread runs during the execution */
void *threadTimedRun(void *arg)
{
	struct thread_data *tData = (struct thread_data *)arg;
	uint64_t tid = tData->tid;
	uint64_t nwarehouse = tData->nwarehouse;
	uint64_t num_items = tData->num_items;

	// Set CPU affinity
	set_cpu(tid);

	barrier_cross(&init_barrier);
	barrier_cross(&barrier_global);

	tData->ops = new_orders(tid, nwarehouse, num_items);

	return NULL;
}

void *threadOpRun(void *arg)
{
	struct thread_data *tData = (struct thread_data *)arg;
	uint64_t tid = tData->tid;
	uint64_t nops = tData->nops;
	uint64_t nwarehouse = tData->nwarehouse;
	uint64_t num_items = tData->num_items;

	// Set CPU affinity
	set_cpu(tid);

	barrier_cross(&init_barrier);
	barrier_cross(&barrier_global);

	tData->ops = new_orders_nops(tid, nops, nwarehouse, num_items);

	return NULL;
}

void run(char *argv[], uint64_t nwarehouse, uint64_t nitems, uint64_t nthreads, uint64_t duration, uint64_t nops)
{
	double exectime;
	uint64_t precision = 4;
	pthread_t threads[nthreads];
	thread_data allThreadsData[nthreads];
	struct timespec tv_start, tv_end;
	std::ofstream fexec;

	/* Init */
	fexec.open("tpcc.csv", std::ios_base::app);
	barrier_init(&barrier_global, nthreads + 1);
	barrier_init(&init_barrier, nthreads);
	for (uint64_t i = 0; i < nthreads; i++)
	{
		allThreadsData[i].tid = i;
		allThreadsData[i].ops = 0;
		allThreadsData[i].nops = nops;
		allThreadsData[i].nwarehouse = nwarehouse;
		allThreadsData[i].num_items = nitems;
	}
	stop = (false);

	/* Timed Run */
	if (duration != 0)
	{
		for (uint64_t i = 0; i < nthreads; i++)
		{
			pthread_create(&threads[i], NULL, threadTimedRun, (void *)&allThreadsData[i]);
		}

		barrier_cross(&barrier_global);

		// for(int i = 0; i < duration; i++) {
		// 	sleep(1);
		// 	uint64_t totalOps = 0;
		// 	for(uint64_t j = 0; j < nthreads; j++)
		// 	{
		// 		totalOps += allThreadsData[j].ops;
		// 	}
		// 	uint64_t tput = (uint64_t)((double)totalOps) / ((double)i);
		// 	double mtput = (double)tput / (1000000UL);
		// 	std::cout << argv[0] << "," << std::to_string(nwarehouse) << "," << std::to_string(totalOps) << "," << std::to_string(nthreads) << "," << std::to_string(duration) << "," << std::to_string(tput) << "," << std::setprecision(precision) << mtput << std::endl;
		// }

		sleep(duration);

		stop = (true);
		for (uint64_t i = 0; i < nthreads; i++)
		{
			pthread_join(threads[i], NULL);
		}
		exectime = duration;
	}
	else
	{
		for (uint64_t i = 0; i < nthreads; i++)
		{
			pthread_create(&threads[i], NULL, threadOpRun, (void *)&allThreadsData[i]);
		}

		clock_gettime(CLOCK_REALTIME, &tv_start);
		barrier_cross(&barrier_global);

		for (uint64_t i = 0; i < nthreads; i++)
		{
			pthread_join(threads[i], NULL);
		}
		clock_gettime(CLOCK_REALTIME, &tv_end);

		exectime = (tv_end.tv_sec - tv_start.tv_sec) * 1E9;
		exectime += tv_end.tv_nsec - tv_start.tv_nsec;
		exectime = (double)exectime / (double)1E9; // convert to second
	}

	/* Collect */
	uint64_t totalOps = 0;
	for (uint64_t i = 0; i < nthreads; i++)
	{
		totalOps += allThreadsData[i].ops;
	}

	uint64_t tput = (uint64_t)((double)totalOps) / ((double)exectime);
	double mtput = (double)tput / (1000000UL);

	// uint64_t precision = 4;
	long pos = fexec.tellp();
	if (pos == 0)
	{
		fexec << "binary,nwarehouse,nitems,nthread,duration,nops,total_ops,exec_time,throughput\n";
	}

	fexec << argv[0] << "," << std::to_string(nwarehouse) << "," << std::to_string(nitems) << "," << std::to_string(nthreads) << "," << std::to_string(duration) << "," << std::to_string(nops) << "," << std::to_string(totalOps) << "," << std::to_string(exectime) << "," << std::to_string(tput) << std::endl;
	std::cout << argv[0] << "," << std::to_string(nwarehouse) << "," << std::to_string(nitems) << "," << std::to_string(nthreads) << "," << std::to_string(duration) << "," << std::to_string(nops) << "," << std::to_string(totalOps) << "," << std::to_string(exectime) << "," << std::to_string(tput) << "," << std::setprecision(precision) << mtput << std::endl;
	std::cerr << argv[0] << "," << std::to_string(nwarehouse) << "," << std::to_string(nitems) << "," << std::to_string(nthreads) << "," << std::to_string(duration) << "," << std::to_string(nops) << "," << std::to_string(totalOps) << "," << std::to_string(exectime) << "," << std::to_string(tput) << "," << std::setprecision(precision) << mtput << std::endl;

	fexec.close();
}

#define NUM_ORDERS 100 // 10000000
#define NUM_THREADS 1

#define NUM_WAREHOUSES 1
#define NUM_ITEMS 100 // 10000
#define NUM_LOCKS NUM_WAREHOUSES * 10 + NUM_WAREHOUSES *NUM_ITEMS
TPCC_DB *tpcc_db; // array of TPCC_DB, array length == nthreads

void init_db(uint64_t nthreads, uint64_t nwarehouse, uint64_t nitems)
{
#ifdef _ENABLE_LIBPMEMOBJ
	try
	{
		if (access(PMDK_POOL_FILE, F_OK) != 0)
		{

			pool = pmem::obj::pool<TPCC_DB>::create(
				PMDK_POOL_FILE, "TPCC", PMDK_POOL_SIZE, CREATE_MODE_RW);
		}
		else
		{
			pool = pmem::obj::pool<TPCC_DB>::open(PMDK_POOL_FILE, "TPCC");
		}
		table = pool.root();
	}
	catch (const pmem::pool_error &e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
		return;
	}
	catch (const pmem::transaction_error &e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
		return;
	}
#else
#endif

	tpcc_db = new TPCC_DB(nwarehouse, nitems);

#ifdef _ENABLE_LIBPMEMOBJ
	table->initialize(nthreads, nwarehouse, nitems);
	table->populate_tables();
#else
	tpcc_db->initialize(nthreads, nwarehouse, nitems);
	tpcc_db->populate_tables();
#endif
}

void deinit_db(uint64_t nthreads)
{
#ifdef _ENABLE_LIBPMEMOBJ
#else
	tpcc_db->deinitialize();
	delete tpcc_db;
#ifdef _VOLATILE_TPCC_DB
	free(tpcc_db);
#endif
#endif
}

#define OL_MIN 5
#define OL_RANGE 10

uint64_t new_orders_nops(uint64_t tid, uint64_t nops, uint64_t nwarehouse, uint64_t num_items)
{
	long int rand_val;
	int w_id, d_id, c_id, ol_cnt;
	int item_ids[15];
	uint64_t ops = 0;
	fprintf(stderr, "Execution Started\n");
	int new_item_id;
	bool match;
	int i;
	for (uint64_t i = 0; i < nops; i++)
	{
		rand_val = random();
		w_id = rand_val % nwarehouse;					   // 4 bits
		d_id = (rand_val >> 4) % N_DISTRICT_PER_WAREHOUSE; // 4 bits
		c_id = (rand_val >> 8) % N_CUSTOMER_PER_DISTRICT;  // 4 bits

		// Create orderline
		// ol_cnt = get_random(tid, 5, 15);
		ol_cnt = (rand_val >> 12) % OL_RANGE + OL_MIN; // 4 bits
		for (i = 0; i < ol_cnt; i++)
		{
			do
			{
				match = false;
				new_item_id = random() % num_items;
				// new_item_id = (rand_val >> (16 + i) % num_items;
				for (int j = 0; j < i; j++)
				{
					if (new_item_id == item_ids[j])
					{
						match = true;
						break;
					}
				}
			} while (match);
			item_ids[i] = new_item_id;
		}

		// std::sort(item_ids, item_ids + ol_cnt);
#ifdef _ENABLE_LIBPMEMOBJ
try
		{
			// pmem::obj::transaction::run(pool, [&]
			// table->update_location(subId, vlr);
			// 							{ table->update_location(subId, vlr); });
			pmem::obj::transaction::run(pool, [&]
										{ table->new_order_tx(tid, w_id, d_id, c_id, item_ids, ol_cnt); });
		}
		catch (const std::runtime_error &e)
		{
			std::cerr << "Exception: " << e.what()
					  << std::endl;
			return 1;
		}
		catch (const std::logic_error &e)
		{
			std::cerr << "Exception: " << e.what()
					  << std::endl;
			return 1;
		}

#else
		tpcc_db->new_order_tx(tid, w_id, d_id, c_id, item_ids, ol_cnt);
#endif

		ops++;
	}
	fprintf(stderr, "Execution finished. Returning: %lu\n", ops);

	return ops;
}

uint64_t new_orders(uint64_t tid, uint64_t nwarehouse, uint64_t num_items)
{
	long int rand_val;
	int w_id, d_id, c_id, ol_cnt;
	int item_ids[15];
	uint64_t ops = 0;
	fprintf(stderr, "Running received. RAND_MAX=%d\n", RAND_MAX);
	pthread_mutex_t tmp;
	pthread_mutex_init(&tmp, NULL);
	pthread_mutex_lock(&tmp);
	int new_item_id;
	bool match;
	int i;
	while (!stop)
	{
		rand_val = random();
		w_id = (((uint64_t)rand_val) % nwarehouse) + 1;			 // 4 bits
		d_id = ((rand_val >> 4) % N_DISTRICT_PER_WAREHOUSE) + 1; // 4 bits
		c_id = ((rand_val >> 8) % N_CUSTOMER_PER_DISTRICT) + 1;	 // 4 bits

		// Create orderline
		// ol_cnt = get_random(tid, 5, 15);
		ol_cnt = (rand_val >> 12) % OL_RANGE + OL_MIN; // 4 bits
		for (i = 0; i < ol_cnt; i++)
		{
			do
			{
				match = false;
				new_item_id = (random() % num_items) + 1;
				for (int j = 0; j < i; j++)
				{
					if (new_item_id == item_ids[j])
					{
						match = true;
						break;
					}
				}
			} while (match);
			item_ids[i] = new_item_id;
		}

		// std::sort(item_ids, item_ids + ol_cnt);

#ifdef _ENABLE_LIBPMEMOBJ
try
		{
			// pmem::obj::transaction::run(pool, [&]
			// table->update_location(subId, vlr);
			// 							{ table->update_location(subId, vlr); });
			pmem::obj::transaction::run(pool, [&]
										{ table->new_order_tx(tid, w_id, d_id, c_id, item_ids, ol_cnt); });
		}
		catch (const std::runtime_error &e)
		{
			std::cerr << "Exception: " << e.what()
					  << std::endl;
			return 1;
		}
		catch (const std::logic_error &e)
		{
			std::cerr << "Exception: " << e.what()
					  << std::endl;
			return 1;
		}

#else
		tpcc_db->new_order_tx(tid, w_id, d_id, c_id, item_ids, ol_cnt);
#endif

		ops++;
	}
	fprintf(stderr, "Stop received. Returning: %lu\n", ops);
	pthread_mutex_unlock(&tmp);

	return ops;
}

int main(int argc, char *argv[])
{
	if (argc != 6)
	{
		fprintf(stderr, "%s N_WAREHOUSE N_ITEMS NTHREADS DURATION NOPS\n", argv[0]);
		return -1;
	}

	uint64_t nwarehouse = atoi(argv[1]);
	uint64_t nitems = atoi(argv[2]);
	uint64_t nthreads = atoi(argv[3]);
	uint64_t duration = atoi(argv[4]);
	uint64_t nops = atoi(argv[5]);

	assert(duration != 0 || nops != 0);

	uint64_t nlocks = nwarehouse * 10 + nwarehouse * nitems;

	init_db(nthreads, nwarehouse, nitems);

	run(argv, nwarehouse, nitems, nthreads, duration, nops);

	deinit_db(nthreads);

	return 0;
}
