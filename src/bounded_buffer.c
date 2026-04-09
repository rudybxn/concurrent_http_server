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
 * buffer_get_sff  (consumer — worker thread, SFF policy)
 *
 * Shortest-File-First: instead of always taking from head,
 * we scan all count entries, pick the one with the smallest
 * file_size, remove it, and shift the remaining entries to
 * fill the gap — keeping the circular buffer consistent.
 *
 * The scan is O(n) in the number of buffered items, which is
 * acceptable for the buffer sizes used in this project.
 *
 * 1. Acquire lock
 * 2. While buffer is empty → wait on not_empty
 * 3. Linear scan for minimum file_size
 * 4. Copy winner out; shift all entries after it back by one
 * 5. Adjust tail and count
 * 6. Signal not_full; release lock
 * 7. Return winner
 * ---------------------------------------------------------------- */
request_t buffer_get_sff(bounded_buffer_t *buf) {
    pthread_mutex_lock(&buf->lock);

    /* Wait while empty */
    while (buf->count == 0) {
        pthread_cond_wait(&buf->not_empty, &buf->lock);
    }

    /* Find the offset (from head) of the entry with minimum file_size.
       min_i is a logical index: 0 = head, 1 = head+1, etc. */
    int min_i = 0;
    for (int i = 1; i < buf->count; i++) {
        int idx     = (buf->head + i)     % buf->capacity;
        int min_idx = (buf->head + min_i) % buf->capacity;
        if (buf->entries[idx].file_size < buf->entries[min_idx].file_size) {
            min_i = i;
        }
    }

    /* Copy out the winner */
    int winner_idx = (buf->head + min_i) % buf->capacity;
    request_t req  = buf->entries[winner_idx];

    /* Shift entries that come after the winner one position back
       to fill the gap left by removing the winner. */
    for (int i = min_i; i < buf->count - 1; i++) {
        int curr = (buf->head + i)     % buf->capacity;
        int next = (buf->head + i + 1) % buf->capacity;
        buf->entries[curr] = buf->entries[next];
    }

    /* The tail slot just became unused; back tail up by one */
    buf->tail = (buf->tail - 1 + buf->capacity) % buf->capacity;
    buf->count--;

    /* Wake producer if it was blocked on a full buffer */
    pthread_cond_signal(&buf->not_full);

    pthread_mutex_unlock(&buf->lock);

    return req;
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