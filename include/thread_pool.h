/* =============================================================
 * thread_pool.h
 *
 * Fixed-size pool of worker threads that consume requests from
 * a shared bounded buffer and dispatch them to http_handle().
 *
 * The scheduling policy is selected at runtime via use_sff:
 *   0 → FCFS: workers always take the oldest queued request.
 *   1 → SFF:  workers always take the smallest queued file.
 * The flag is set by main() after thread_pool_init() returns
 * and before any requests arrive, so it is effectively read-only
 * during normal operation and requires no synchronization.
 * ============================================================= */

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "bounded_buffer.h"

/* Fixed-size thread pool.
   threads     — array of spawned pthread handles.
   num_threads — length of the threads array.
   buffer      — shared bounded buffer; owned by the caller.
   shutdown    — set to 1 by thread_pool_destroy() to signal
                 workers to drain remaining requests and exit.
   use_sff     — scheduling policy passed to buffer_get() on
                 every dequeue (0 = FCFS, 1 = SFF).            */
typedef struct {
    pthread_t        *threads;      // array of worker threads
    int               num_threads;  //size of array
    bounded_buffer_t *buffer;       // shared work queue
    int               shutdown;     // flag to signal workers to stop
    int               use_sff;      // flag of which alg to use
} ThreadPool;

/* Allocate the pool and spawn num_threads worker threads.
   Workers block immediately on the empty buffer until work
   arrives. Returns NULL on allocation or pthread failure.     */
ThreadPool *thread_pool_init(int num_threads, bounded_buffer_t *buffer);

/* Submit a request to the pool. Wraps buffer_put(), which
   blocks if the buffer is full (backpressure).               */
void        thread_pool_submit(ThreadPool *pool, request_t req);

/* Signal all workers to exit, drain remaining requests, join
   every thread, and free the pool. Does NOT free the buffer;
   the caller is responsible for calling buffer_destroy().    */
void        thread_pool_destroy(ThreadPool *pool);

#endif
