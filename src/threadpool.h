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

#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#define MAX_THREADS 8 // how many threads are used for parallelism
#define MAX_JOBS 256  // how many jobs can be queued at max

#ifdef __cplusplus
extern "C" {
#endif

/**
* @file threadpool.h
* @brief Threadpool Header File
*/
typedef struct t_arguments {
    int i, j;
} t_arguments;

/**
*  @struct threadpool_task
*  @brief the work struct
*
*  @var function Pointer to the function that will perform the job.
*  @var argument Argument to be passed to the function.
*/
typedef struct threadpool_t {
    t_arguments args[MAX_JOBS]; // array to store the multithreaded function's arguments
    pthread_mutex_t pool_lock; // mutex for variable synchronization
    pthread_cond_t all_jobs_done; // signal sent when all the tasks are done
    pthread_cond_t new_job_received; // signal sent when a new task is added
    int head; // index of the first element in the queue
    int tail; // index of the last element
    int shutdown; // indicator used when the threadpool is shut down
    int count_jobs; // number of tasks queued
    int count_threads; // number of threads working
    pthread_t threads[MAX_THREADS]; // array to store the POSIX threads
} threadpool_t;

/**
*  @struct threadpool_task
*  @brief the work struct
*
*  @var function Pointer to the function that will perform the job.
*  @var argument Argument to be passed to the function.
*/
typedef struct {
    void (*function)(void *);
    void *argument;
} threadpool_task_t;

typedef enum {
    threadpool_invalid = -1,
    threadpool_lock_failure = -2,
    threadpool_queue_full = -3,
    threadpool_shutdown = -4,
    threadpool_thread_failure = -5
} threadpool_error_t;

/**
* @function void *threadpool_thread(void *threadpool)
* @brief the worker thread
* @param threadpool the global_threadpool which own the thread
*/
static void *threadpool_thread(void *argument);

int threadpool_free();
int threadpool_create();
int threadpool_destroy();
void threadpool_persist();

/**
* @function threadpool_initialize
* @brief Creates a threadpool_t object.
*/
void threadpool_initialize();

/**
* @function threadpool_add
* @brief add a new task in the queue of a thread pool
* @param function Pointer to the function that will perform the task.
* @param argument Argument to be passed to the function.
* @return 0 if all goes well, negative values in case of error (@see
* threadpool_error_t for codes).
*/
int threadpool_add(void(*routine)(void *), void *arg);

/**
* @function threadpool_terminate
* @brief Stops and destroys a thread pool.
*/
void threadpool_terminate();

/**
* @function threadpool_wait
* @brief Wait until all the tasks are finished in the thread pool.
*/
void threadpool_wait();

#ifdef __cplusplus
}
#endif

#endif /* _THREADPOOL_H_ */
