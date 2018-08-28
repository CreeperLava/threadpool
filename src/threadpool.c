/*
 * Copyright (c) 2016, Mathias Brossard <mathias@brossard.org>.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _THREADPOOL_C_
#define _THREADPOOL_C_

/**
 * @file threadpool.c
 * @brief Threadpool implementation file
 */
#include <stdlib.h>
#include <stdio.h>

#ifdef __linux__
#include <pthread.h> // if using Linux, compile using default pthread header
#elif _WIN32
#include "pthread.h" // if using Windows, compile using pthread wrapper
#endif

#include "threadpool.h"

static threadpool_t * global_threadpool; // global variable for threadpool

/**
 * @function void *threadpool_thread(void *threadpool)
 * @brief the worker thread
 * @param threadpool the global_threadpool which own the thread
 */
void threadpool_initialize() {
    if (global_threadpool == NULL) {
        if(threadpool_create() != NULL) { // create threadpool
            threadpool_persist();
            printf("Threadpool started with %d threads and queue size of %d\n", MAX_THREADS, MAX_JOBS);
        } else
            printf("Error creating threadpool, multithreading disabled\n"); // print error ID
    }
}

int threadpool_create() {
    int i;

    if ((global_threadpool = malloc(sizeof(threadpool_t))) == NULL)
        goto err;

    /* Initialize */
	global_threadpool->shutdown = 0;
    	global_threadpool->head = global_threadpool->tail = 0;
	global_threadpool->count_threads = global_threadpool->count_jobs = 0;

    /* Initialize mutex and conditional variable first */
	if ((pthread_mutex_init(&(global_threadpool->pool_lock), NULL) != 0) || (pthread_cond_init(&(global_threadpool->new_job_received), NULL) != 0) ||
		(pthread_cond_init(&(global_threadpool->all_jobs_done), NULL) != 0) || (global_threadpool->threads == NULL) || (global_threadpool->queue == NULL))
		goto err;

    /* Start worker threads */
    for (i = 0; i < MAX_THREADS; i++) {
        if (pthread_create(&(global_threadpool->threads[i]), NULL, threadpool_thread, NULL) != 0) {
            threadpool_destroy();
            return NULL;
        }
    }

	return 1;

    err:
    if (global_threadpool)
        threadpool_free();
    return NULL;
}

// fonction permettant d'ajouter du travail à la pool
int threadpool_add(void(*function)(void *), void *argument) {

	/* Are we full ? */
	pthread_mutex_lock(&(global_threadpool->pool_lock));
	if (global_threadpool->count_jobs == MAX_JOBS - 1) // threadpool full
	while (global_threadpool->count_jobs != 0 || global_threadpool->count_threads != 0) // wait
		pthread_cond_wait(&(global_threadpool->all_jobs_done), &(global_threadpool->pool_lock));

	global_threadpool->count_jobs++; // one more job queued

	/* Add job to queue */
	global_threadpool->queue[global_threadpool->tail].function = function;
	global_threadpool->queue[global_threadpool->tail].argument = argument;
	global_threadpool->tail = (global_threadpool->tail + 1) % MAX_JOBS; // if successful, increase tail

	pthread_cond_signal(&(global_threadpool->new_job_received));
	pthread_mutex_unlock(&(global_threadpool->pool_lock));

	return 0;
}

// fonction lancée sur chaque thread pour attendre et lancer les jobs
static void *threadpool_thread(void *argument) {
	threadpool_task_t job;

	for (;;) {
		/* Lock must be taken to wait on conditional variable */
		pthread_mutex_lock(&(global_threadpool->pool_lock));

		/* Wait on condition variable, check for spurious wakeups.
		When returning from pthread_cond_wait(), we own the pool_lock. */
		while (global_threadpool->count_jobs == 0)
			pthread_cond_wait(&(global_threadpool->new_job_received), &(global_threadpool->pool_lock)); // wait for a job to be added
		if (global_threadpool->shutdown)
			break;

		/* Grab our job */
		job.function = global_threadpool->queue[global_threadpool->head].function;
		job.argument = global_threadpool->queue[global_threadpool->head].argument;
		global_threadpool->head = (global_threadpool->head + 1) % MAX_JOBS;
		global_threadpool->count_jobs--; // job assigned, free the slot for a new job
		global_threadpool->count_threads++; // thread working
		pthread_mutex_unlock(&(global_threadpool->pool_lock));

		/* Get to work */
		(*(job.function))(job.argument);

		pthread_mutex_lock(&(global_threadpool->pool_lock));
		global_threadpool->count_threads--; // thread free
		if (global_threadpool->count_jobs == 0 && global_threadpool->count_threads == 0)
			pthread_cond_signal(&(global_threadpool->all_jobs_done));
		pthread_mutex_unlock(&(global_threadpool->pool_lock));
	}

	pthread_exit(NULL);
	return (NULL);
}

/* Wait until all tasks are finished */
void threadpool_wait() {
	pthread_mutex_lock(&(global_threadpool->pool_lock));
	while (global_threadpool->count_jobs != 0 || global_threadpool->count_threads != 0)
		pthread_cond_wait(&(global_threadpool->all_jobs_done), &(global_threadpool->pool_lock));
	pthread_mutex_unlock(&(global_threadpool->pool_lock));
}

int threadpool_free() {
	if (global_threadpool == NULL)
		return -1;

	pthread_mutex_lock(&(global_threadpool->pool_lock));
	pthread_mutex_unlock(&(global_threadpool->pool_lock));
	pthread_mutex_destroy(&(global_threadpool->pool_lock));
	pthread_cond_destroy(&(global_threadpool->all_jobs_done));
	pthread_cond_destroy(&(global_threadpool->new_job_received));

	free(global_threadpool);
	return 0;
}

void threadpool_terminate() {
    int err;
    threadpool_wait();
    if((err = threadpool_destroy()) != 0)
        printf("Error %d destroying threadpool\n", err); // print error ID
    global_threadpool = NULL;
}

int threadpool_destroy() {
	int i;

	if (global_threadpool == NULL)
		return threadpool_invalid;

	if (pthread_mutex_lock(&(global_threadpool->pool_lock)) != 0)
		return threadpool_lock_failure;

	/* Wake up all worker threads */
	global_threadpool->shutdown = 1; // set shutdown flag so that the threads exit after broadcast
	if (pthread_cond_broadcast(&(global_threadpool->new_job_received)) != 0 && pthread_mutex_unlock(&(global_threadpool->pool_lock)) != 0)
		return threadpool_lock_failure;

	/* Join all worker thread */
	for (i = 0; i < MAX_THREADS; i++)
	if (pthread_join(global_threadpool->threads[i], NULL) != 0)
		return threadpool_thread_failure;

	/* Only if everything went well do we deallocate the global_threadpool */
	threadpool_free();

	return 0;
}

#endif /* _THREADPOOL_C_ */
