
/*
 Author: Vaibhav Gogte <vgogte@umich.edu>
				 Aasheesh Kolli <akolli@umich.edu>


This file is the TATP benchmark, performs various transactions as per the specifications.
*/

#include "tatp_db.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <cstdint>
#include <assert.h>
#include <sys/time.h>
#include <string>
#include <fstream>
#include <thread>
#include "../common/common.h"

std::atomic<uint64_t> count;

#ifdef _ENABLE_AGR
extern void *my_context_void();
#endif

TATP_DB *my_tatp_db;
void *pop = NULL;

void init_db(uint64_t num_subscribers, uint64_t nthreads)
{
	init_pmalloc();
	my_tatp_db = new TATP_DB(num_subscribers);
	my_tatp_db->initialize(num_subscribers, nthreads);
}

void update_locations(uint64_t nops, uint64_t id)
{
	for (int i = 0; i < nops; i++)
	{
		long subId = my_tatp_db->get_sub_id();
		uint64_t vlr = my_tatp_db->get_random_vlr(0);

#ifdef _ENABLE_LOGGING
		pthread_mutex_lock(&my_tatp_db->lock_[subId]);
		my_tatp_db->backup_location(subId);
#endif

		my_tatp_db->update_location(subId, vlr);

#ifdef _ENABLE_LOGGING
		pthread_mutex_unlock(&my_tatp_db->lock_[subId]);
		my_tatp_db->discard_backup(subId);
#endif
	}
}

// from stackoverflow: https://stackoverflow.com/questions/68804469/subtract-two-timespec-objects-find-difference-in-time-or-duration
struct timespec diff_timespec(const struct timespec *time1,
							  const struct timespec *time0)
{
	assert(time1 && time1->tv_nsec < 1000000000);
	assert(time0 && time0->tv_nsec < 1000000000);
	struct timespec diff = {.tv_sec = time1->tv_sec - time0->tv_sec, .tv_nsec = time1->tv_nsec - time0->tv_nsec};
	if (diff.tv_nsec < 0)
	{
		diff.tv_nsec += 1000000000;
		diff.tv_sec--;
	}
	return diff;
}

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
	uint32_t tid;
	uint64_t ops;
};

/* An operation every thread runs during the execution */
void *threadRun(void *arg)
{
	struct thread_data *tData = (struct thread_data *)arg;
	uint32_t id = tData->tid;
	uint64_t ops = tData->ops;

	// Set CPU affinity
	set_cpu(id - 1);

#ifdef _ENABLE_AGR
	while (!((bool)my_context_void()))
	{
	}
	// fprintf(stdout, "threadRun (ID:%lu)\n", (uint64_t)my_context_void());
#endif

	barrier_cross(&init_barrier);

	update_locations(ops, id);

	// barrier_cross(&barrier_global);
	return NULL;
}

void run(char *argv[], uint64_t nsub, uint64_t nops, uint64_t nthreads)
{
	struct timespec tv_start, tv_end;
	std::ofstream fexec;
	fexec.open("exec.csv", std::ios_base::app);

	barrier_init(&barrier_global, nthreads + 1);
	barrier_init(&init_barrier, nthreads + 1);

	// std::thread *threads[nthreads];
	pthread_t threads[nthreads];
	thread_data allThreadsData[nthreads];

	for (uint32_t j = 1; j < nthreads + 1; j++)
	{
		thread_data &tData = allThreadsData[j - 1];
		tData.tid = j;
		tData.ops = nops / nthreads;
		// threads[j - 1] = new std::thread(threadRun, &tData);
		pthread_create(&threads[j - 1], NULL, threadRun, (void *)&tData);
	}

	barrier_cross(&init_barrier);

	clock_gettime(CLOCK_REALTIME, &tv_start);

	for (int i = 0; i < nthreads; i++)
	{
		// threads[i]->join();
		pthread_join(threads[i], NULL);
	}

	clock_gettime(CLOCK_REALTIME, &tv_end);

	struct timespec difftime = diff_timespec(&tv_end, &tv_start);
	uint64_t duration = difftime.tv_sec * 1000000000LLU + difftime.tv_nsec;
	double duration_sec = (double)duration / 1000000000LLU;

	fexec << argv[0] << "," << std::to_string(nsub) << "," << std::to_string(nops) << "," << std::to_string(nthreads) << "," << std::to_string(duration) << std::endl;
	std::cout << argv[0] << "," << std::to_string(nsub) << "," << std::to_string(nops) << "," << std::to_string(nthreads) << "," << std::to_string(duration) << std::endl;
	std::cerr << argv[0] << "," << std::to_string(nsub) << "," << std::to_string(nops) << "," << std::to_string(nthreads) << "," << std::to_string(duration) << std::endl;
	fprintf(stderr, "%s,%lf,%lu\n", argv[0], duration_sec, count.load());

	fexec.close();
}
/************************************************************/

int main(int argc, char *argv[])
{
	if (argc != 4)
	{
		fprintf(stderr, "%s NSUBSCRIBER NOPS NTHREADS\n", argv[0]);
		return -1;
	}

	uint64_t nsub = atoi(argv[1]);
	uint64_t nops = atoi(argv[2]);
	uint64_t nthreads = atoi(argv[3]);

	init_db(nsub, nthreads);

	my_tatp_db->populate_tables(nsub);

	run(argv, nsub, nops, nthreads);

	free(my_tatp_db);

	return 0;
}
