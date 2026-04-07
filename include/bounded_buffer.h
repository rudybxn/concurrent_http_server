/* =============================================================
 * bounded_buffer.h
 *
 * Thread-safe bounded buffer (circular queue) shared between
 * the producer (main/accept loop) and consumers (worker threads).
 *
 * Supports two dequeue policies via buffer_get()'s use_sff flag:
 *   FCFS (use_sff=0) — items come out in the order they went in.
 *   SFF  (use_sff=1) — the item whose file is smallest on disk
 *                      comes out first; ties broken by arrival
 *                      order (FCFS among equals).
 *
 * For SFF to work, request_t.uri must be populated before the
 * request is enqueued so buffer_get() can stat() each entry
 * while scanning for the shortest file.
 * ============================================================= */

#ifndef BOUNDED_BUFFER_H
#define BOUNDED_BUFFER_H

#include <pthread.h>

/* A single queued request.
   client_fd  — accepted socket to read the request from and
                write the response to.
   arrival_ms — wall-clock timestamp (ms) recorded by main()
                the moment accept() returns; used to break ties
                in SFF (earliest arrival wins) and for latency
                measurements.
   uri        — request path extracted by main() via MSG_PEEK
                before enqueueing (e.g. "/index.html"); used by
                buffer_get() to stat() the file for SFF.       */
typedef struct {
    int   client_fd;    /* accepted socket descriptor          */
    long  arrival_ms;   /* timestamp for FCFS ordering         */
    char  uri[512];     /* full path to file to get size       */
} request_t;

/* The circular buffer -------------------------------------------
   Producers wait on not_full; consumers wait on not_empty.
   All fields except the condition variables are protected by
   lock and must not be read or written without holding it.    */
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

bounded_buffer_t *buffer_init(int capacity);
void              buffer_put(bounded_buffer_t *buf, request_t req);

/* Dequeue one request according to the chosen policy.
   use_sff = 0 → FCFS: dequeue from head (oldest first).
   use_sff = 1 → SFF:  scan all live entries and dequeue the one
                        whose file is smallest; ties broken by
                        arrival order (FCFS among equals).
   Blocks if the buffer is empty.                               */
request_t         buffer_get(bounded_buffer_t *buf, int use_sff);
void              buffer_destroy(bounded_buffer_t *buf);

#endif
