/* =============================================================
 * bounded_buffer.h
 *
 * Thread-safe bounded buffer (circular queue) for the
 * producer-consumer pattern. The main thread (producer)
 * enqueues accepted connection descriptors; worker threads
 * (consumers) dequeue them for processing.
 *
 * The buffer holds request_t structs rather than bare ints
 * so that metadata (file size, arrival time) is available
 * for scheduling policies (FCFS / SFF) without refactoring.
 *
 * Synchronization:
 *   - One pthread_mutex_t protects all shared state.
 *   - Two condition variables:
 *       not_empty  — workers wait here when the buffer is empty
 *       not_full   — producer waits here when the buffer is full
 * ============================================================= */

#ifndef BOUNDED_BUFFER_H
#define BOUNDED_BUFFER_H

#include <pthread.h>
#include <sys/types.h>

/* ---- Request descriptor ----------------------------------------
   Populated by the producer (main thread) before enqueueing.
   file_size and filename will be used in Sprint 4 for SFF;
   for now only client_fd needs to be set. */
typedef struct {
    int   client_fd;        /* accepted socket descriptor          */
    char  filename[256];    /* parsed URI path (set later)         */
    off_t file_size;        /* from stat(), populated by producer  */
    long  arrival_ms;       /* timestamp for FCFS ordering         */
} request_t;

/* ---- Bounded buffer ------------------------------------------- */
typedef struct {
    request_t *entries;     /* heap-allocated array of capacity     */
    int capacity;           /* max items the buffer can hold        */
    int count;              /* current number of items              */
    int head;               /* index of next item to dequeue (out)  */
    int tail;               /* index of next empty slot (in)        */

    pthread_mutex_t lock;
    pthread_cond_t  not_empty;  /* signaled after enqueue           */
    pthread_cond_t  not_full;   /* signaled after dequeue           */
} bounded_buffer_t;

/* Allocate and initialize a buffer with the given capacity.
   Returns NULL on failure. */
bounded_buffer_t *buffer_init(int capacity);

/* Enqueue a request in arrival order (FCFS). Blocks if full.
   Called by the producer (main thread). */
void buffer_put(bounded_buffer_t *buf, request_t req);

/* Enqueue a request in ascending file_size order (SFF).
   Shifts existing entries to insert the new one in its sorted
   position so that buffer_get always dequeues the shortest file.
   Blocks if full. Called by the producer (main thread). */
void buffer_put_sff(bounded_buffer_t *buf, request_t req);

/* Dequeue from head. Blocks if empty.
   Works for both FCFS (arrival order) and SFF (size order)
   because the ordering is established at insert time.
   Called by consumer (worker) threads. */
request_t buffer_get(bounded_buffer_t *buf);

/* Tear down: free the entries array, destroy mutex/condvars,
   then free the buffer struct itself. */
void buffer_destroy(bounded_buffer_t *buf);

#endif
