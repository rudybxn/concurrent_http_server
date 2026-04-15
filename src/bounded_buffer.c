/* =============================================================
 * bounded_buffer.c
 *
 * Implementation of thread-safe bounded buffer (circular queue).
 * FCFS dequeue: items come out in the order they went in.
 *
 * Invariants:
 *   - head == tail && count == 0          → buffer is empty
 *   - head == tail && count == capacity   → buffer is full
 *   - count is always in [0, capacity]
 *
 * Critical section discipline:
 *   - Lock is held only during enqueue/dequeue array manipulation.
 *   - No I/O or blocking syscalls while the lock is held.
 *   - Condition variables are checked in while-loops to guard
 *     against spurious wakeups (POSIX requirement).
 * ============================================================= */
 
#include "bounded_buffer.h"
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
/* ----------------------------------------------------------------
 * buffer_init
 * Allocates the buffer struct and the internal entries array.
 * Initializes the mutex and both condition variables.
 * Returns NULL if any allocation or init call fails.
 * ---------------------------------------------------------------- */
bounded_buffer_t *buffer_init(int capacity) {
    if (capacity <= 0) {
        fprintf(stderr, "buffer_init: capacity must be > 0\n");
        return NULL;
    }
 
    bounded_buffer_t *buf = malloc(sizeof(bounded_buffer_t));
    if (!buf) {
        perror("buffer_init: malloc buffer");
        return NULL;
    }
 
    buf->entries = malloc(sizeof(request_t) * capacity);
    if (!buf->entries) {
        perror("buffer_init: malloc entries");
        free(buf);
        return NULL;
    }
 
    buf->capacity = capacity;
    buf->count    = 0;
    buf->head     = 0;
    buf->tail     = 0;
 
    pthread_mutex_init(&buf->lock,      NULL);
    pthread_cond_init(&buf->not_empty,  NULL);
    pthread_cond_init(&buf->not_full,   NULL);
 
    return buf;
}
 
/* ----------------------------------------------------------------
 * buffer_put  (producer — main thread)
 *
 * 1. Acquire lock
 * 2. While buffer is full → wait on not_full
 * 3. Copy request into entries[tail], advance tail circularly
 * 4. Signal not_empty (wake one blocked worker)
 * 5. Release lock
 * ---------------------------------------------------------------- */
void buffer_put(bounded_buffer_t *buf, request_t req) {
    pthread_mutex_lock(&buf->lock);
 
    /* Wait while full — while-loop guards against spurious wakeups */
    while (buf->count == buf->capacity) {
        pthread_cond_wait(&buf->not_full, &buf->lock);
    }
 
    /* Enqueue at tail */
    buf->entries[buf->tail] = req;
    buf->tail = (buf->tail + 1) % buf->capacity;
    buf->count++;
 
    /* Wake one waiting worker */
    pthread_cond_signal(&buf->not_empty);
 
    pthread_mutex_unlock(&buf->lock);
}
 
/* ----------------------------------------------------------------
 * buffer_get  (consumer — worker thread)
 *
 * FCFS policy: dequeue from head (oldest item).
 *
 * 1. Acquire lock
 * 2. While buffer is empty → wait on not_empty
 * 3. Copy request from entries[head], advance head circularly
 * 4. Signal not_full (wake producer if it was blocked)
 * 5. Release lock
 * 6. Return the dequeued request
 * ---------------------------------------------------------------- */
request_t buffer_get(bounded_buffer_t *buf) {
    pthread_mutex_lock(&buf->lock);
 
    /* Wait while empty */
    while (buf->count == 0) {
        pthread_cond_wait(&buf->not_empty, &buf->lock);
    }
 
    /* Dequeue from head (FCFS) */
    request_t req = buf->entries[buf->head];
    buf->head = (buf->head + 1) % buf->capacity;
    buf->count--;
 
    /* Wake producer if it was blocked on a full buffer */
    pthread_cond_signal(&buf->not_full);
 
    pthread_mutex_unlock(&buf->lock);
 
    return req;
}
 
/* ----------------------------------------------------------------
 * buffer_put_sff  (producer — main thread, SFF policy)
 *
 * Inserts the new request in ascending file_size order so that
 * buffer_get (which always takes from head) naturally dequeues
 * the shortest file first — no special consumer-side logic needed.
 *
 * Shifting happens at insert time (producer), once per request.
 * This means consumers use the identical head-dequeue path as
 * FCFS, which keeps the shutdown logic simple and correct.
 *
 * 1. Acquire lock
 * 2. While buffer is full → wait on not_full
 * 3. Scan backwards from the last occupied slot toward head,
 *    shifting entries that are larger than req one slot forward,
 *    until we find an entry <= req or exhaust all entries
 * 4. Write req into the opened slot; advance tail
 * 5. Signal not_empty; release lock
 * ---------------------------------------------------------------- */
void buffer_put_sff(bounded_buffer_t *buf, request_t req) {
    pthread_mutex_lock(&buf->lock);
 
    /* Wait while full */
    while (buf->count == buf->capacity) {
        pthread_cond_wait(&buf->not_full, &buf->lock);
    }
 
    /* Scan backwards from the last occupied slot toward head.
       Shift any entry whose file_size > req.file_size one slot
       forward (toward tail), opening a gap for req. Stop when we
       reach an entry that is <= req, or when we've checked all. */
    int i = buf->count - 1;
    while (i >= 0) {
        int physical = (buf->head + i) % buf->capacity;
        if (buf->entries[physical].file_size <= req.file_size) {
            break;
        }
        int dest = (buf->head + i + 1) % buf->capacity;
        buf->entries[dest] = buf->entries[physical];
        i--;
    }
 
    /* Place req in the slot just after the last entry that was <= it */
    int insert = (buf->head + i + 1) % buf->capacity;
    buf->entries[insert] = req;
    buf->tail = (buf->tail + 1) % buf->capacity;
    buf->count++;
 
    pthread_cond_signal(&buf->not_empty);
    pthread_mutex_unlock(&buf->lock);
}
 
/* ----------------------------------------------------------------
 * buffer_destroy
 *
 * Assumes all threads have stopped using the buffer (call after
 * thread_pool_destroy has joined all workers).
 * ---------------------------------------------------------------- */
void buffer_destroy(bounded_buffer_t *buf) {
    if (!buf) return;
 
    pthread_mutex_destroy(&buf->lock);
    pthread_cond_destroy(&buf->not_empty);
    pthread_cond_destroy(&buf->not_full);
 
    free(buf->entries);
    free(buf);
}


