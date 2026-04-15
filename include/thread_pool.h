#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "bounded_buffer.h"
// Thread pool implementation

typedef struct {
    pthread_t *threads; //array of worker threads
    int num_threads; //size of array
    bounded_buffer_t *buffer; // shared work queue
    int shutdown;  // flag to signal workers to stop
    int use_sff;   // 1 = SFF scheduling, 0 = FCFS
} ThreadPool;

/* Create thread pool. Caller will provd initialized buffer.
   schedalg must be "FCFS" or "SFF". */
ThreadPool *thread_pool_init(int num_threads, bounded_buffer_t *buffer,
                             const char *schedalg);

/* Enqueue work for the thread pool. Blocks if buffer is full. */
void thread_pool_submit(ThreadPool *pool, request_t req);

/* Shut down: set the stop flag, wake blocked workers, join every thread, free the struct.
Call buffer_destroy() after this returns. */
void thread_pool_destroy(ThreadPool *pool);

#endif