
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
#include <iomanip>
#include <math.h>
#include "common.h"

std::atomic<uint64_t> count;
std::atomic<bool> stop;

TATP_DB *my_tatp_db;
void *pop = NULL;

void init_db(uint64_t num_subscribers, uint64_t nthreads)
{
	my_tatp_db = new TATP_DB(num_subscribers);
	my_tatp_db->initialize(num_subscribers, nthreads);
}

uint64_t update_locations(uint64_t nops, uint64_t id)
{
	uint64_t ops = 0;
	fprintf(stdout, "Running received.\n");
	while (!stop)
	{

		long subId = my_tatp_db->get_random_s_id(id);
		uint64_t vlr = my_tatp_db->get_random_vlr(id);

		// fprintf(stdout, "subId=%ld, vlr=%lu\n", subId, vlr);

#ifdef _ENABLE_LOGGING
		pthread_mutex_lock(&my_tatp_db->lock_[subId]);

		// Backup memory is within thread local.
		// But don't allow other thread to change it to avoid stale backup.
		my_tatp_db->backup_location(id, subId);
#endif

		my_tatp_db->update_location(subId, vlr);

#ifdef _ENABLE_LOGGING
		pthread_mutex_unlock(&my_tatp_db->lock_[subId]);

		// Backup memory is within thread local.
		// Don't have to be inside the critial section.
		my_tatp_db->discard_backup(id, subId);
#endif
		ops++;
	}

	fprintf(stdout, "Stop received. Returning: %lu\n", ops);

	return ops;
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
	uint64_t from, to;
	bool load;
};

/* An operation every thread runs during the execution */
void *threadRun(void *arg)
{
	struct thread_data *tData = (struct thread_data *)arg;
	uint32_t id = tData->tid;
	uint64_t ops = tData->ops;
	uint64_t from = tData->from;
	uint64_t to = tData->to;
	bool load = tData->load;

	// Set CPU affinity
	set_cpu(id - 1);

	if(load)
	{
		my_tatp_db->populate_tables(from, to);
		std::cout << "Populate Table done: " << id << "\n";
	}
	else
	{
		barrier_cross(&init_barrier);
		barrier_cross(&barrier_global);

		tData->ops = update_locations(ops, id);
	}

	return NULL;
}

void run(char *argv[], uint64_t nsub, uint64_t nops, uint64_t nthreads, uint64_t duration)
{
	struct timespec tv_start, tv_end;
	std::ofstream fexec;
	fexec.open("tatp.csv", std::ios_base::app);

	/* Load */
	unsigned max_nthreads = (nthreads > LOAD_THREADS)?nthreads:LOAD_THREADS;
	pthread_t threads[max_nthreads];
	thread_data allThreadsData[max_nthreads];
	unsigned per_thread_sub = nsub / LOAD_THREADS;

	for (uint32_t j = 1; j < LOAD_THREADS + 1; j++)
	{
		thread_data &tData = allThreadsData[j - 1];
		tData.tid = j;
		tData.ops = 0;
		tData.from = (j - 1) * per_thread_sub;
		tData.to = j * per_thread_sub;
		tData.load = true;
		pthread_create(&threads[j - 1], NULL, threadRun, (void *)&tData);
	}

	for (int i = 0; i < LOAD_THREADS; i++)
	{
		pthread_join(threads[i], NULL);
	}

	/* Run */
	barrier_init(&barrier_global, nthreads + 1);
	barrier_init(&init_barrier, nthreads);
	stop = (false);
	for (uint32_t j = 1; j < nthreads + 1; j++)
	{
		thread_data &tData = allThreadsData[j - 1];
		tData.tid = j;
		tData.ops = 0;
		tData.load = false;
		pthread_create(&threads[j - 1], NULL, threadRun, (void *)&tData);
	}

	barrier_cross(&barrier_global);

	sleep(duration);
	printf("Wakeup\n");

	stop = (true);
	for (int i = 0; i < nthreads; i++)
	{
		pthread_join(threads[i], NULL);
	}

	/* Collect */
	uint64_t totalOps = 0;
	for (uint32_t j = 0; j < nthreads; j++)
	{
		totalOps += allThreadsData[j].ops;
	}

	uint64_t tput = (uint64_t)((double)totalOps) / ((double)duration);
	double mtput = (double)tput / (1000000UL);

	uint64_t precision = 4;
	fexec << argv[0] << "," << std::to_string(nsub) << "," << std::to_string(totalOps) << "," << std::to_string(nthreads) << "," << std::to_string(duration) << "," << std::to_string(tput) << std::endl;
	std::cout << argv[0] << "," << std::to_string(nsub) << "," << std::to_string(totalOps) << "," << std::to_string(nthreads) << "," << std::to_string(duration) << "," << std::to_string(tput) << "," << std::setprecision(precision) << mtput << std::endl;
	std::cerr << argv[0] << "," << std::to_string(nsub) << "," << std::to_string(totalOps) << "," << std::to_string(nthreads) << "," << std::to_string(duration) << "," << std::to_string(tput) << "," << std::setprecision(precision) << mtput << std::endl;

	fexec.close();
}
/************************************************************/

int main(int argc, char *argv[])
{
	if (argc != 5)
	{
		fprintf(stderr, "%s NSUBSCRIBER NOPS NTHREADS DURATION\n", argv[0]);
		return -1;
	}

	uint64_t nsub = atoi(argv[1]);
	uint64_t nops = atoi(argv[2]);
	uint64_t nthreads = atoi(argv[3]);
	uint64_t duration = atoi(argv[4]);

	assert((nsub % LOAD_THREADS == 0) && "NSUB must be divisible by NTHREADS");

	init_db(nsub, nthreads);

	run(argv, nsub, nops, nthreads, duration);

	free(my_tatp_db);

	return 0;
}
