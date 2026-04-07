/* =============================================================
 * bounded_buffer.c
 *
 * Implementation of thread-safe bounded buffer (circular queue).
 * Supports two dequeue policies selected at call time:
 *
 *   FCFS (use_sff=0): items come out in the order they went in.
 *   SFF  (use_sff=1): the item whose file is smallest comes out
 *                     first; ties broken by arrival time (FCFS).
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
#include "http.h"    /* WEB_ROOT — needed to build full paths for stat() */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>  /* stat(), struct stat */
#include <limits.h>    /* LONG_MAX            */

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
 * use_sff = 0  →  FCFS policy: dequeue from head (oldest item).
 * use_sff = 1  →  SFF policy: scan all live entries, pick the one
 *                 with the smallest file on disk, remove it from
 *                 the middle of the circular buffer, and return it.
 *
 * Both policies share the same lock/wait/signal boilerplate; only
 * the selection and removal step differs.
 *
 * 1. Acquire lock
 * 2. While buffer is empty → wait on not_empty
 * 3. Select the target entry (head for FCFS, smallest file for SFF)
 * 4. Remove the entry and repair the buffer
 * 5. Signal not_full (wake producer if it was blocked)
 * 6. Release lock
 * 7. Return the dequeued request
 * ---------------------------------------------------------------- */
request_t buffer_get(bounded_buffer_t *buf, int use_sff) {
    pthread_mutex_lock(&buf->lock);

    /* Wait while empty — while-loop guards against spurious wakeups */
    while (buf->count == 0) {
        pthread_cond_wait(&buf->not_empty, &buf->lock);
    }

    /* best is a logical offset from head into the circular buffer.
       0 means "take from head", which is exactly FCFS behaviour.
       For SFF we scan all entries and update best to point at the
       entry whose file is smallest on disk. */
    int best = 0;

    if (use_sff) {
        long best_size = LONG_MAX;

        /* Scan every live slot in the buffer.
           We walk logical indices 0..count-1 and convert each to a
           physical array index with the modulo wrap.  We call stat()
           on each entry's file so we can compare sizes.  If stat()
           fails (e.g. file not found) we treat the size as LONG_MAX
           so that valid files are always preferred over broken ones. */
        for (int i = 0; i < buf->count; i++) {
            int idx = (buf->head + i) % buf->capacity;

            /* Reconstruct the full filesystem path the same way
               http_handle() does: WEB_ROOT + uri
               e.g. "./www" + "/index.html" → "./www/index.html" */
            char full[512 + sizeof(WEB_ROOT)];
            snprintf(full, sizeof(full), "%s%s", WEB_ROOT,
                     buf->entries[idx].uri);

            struct stat st;
            long sz = (stat(full, &st) == 0) ? (long)st.st_size : LONG_MAX;

            /* Strict less-than means ties are broken by arrival order:
               the earlier-enqueued entry has a lower logical index i
               and wins, giving FCFS behaviour among equal-sized files. */
            if (sz < best_size) {
                best_size = sz;
                best = i;
            }
        }

        /* Capture the chosen request before we overwrite its slot. */
        request_t req = buf->entries[(buf->head + best) % buf->capacity];

        /* Remove the chosen entry from the middle of the circular buffer
           by overwriting it with each successive entry, walking forward
           until we reach the slot just before tail.  This closes the gap
           without disturbing head or the relative order of the remaining
           entries — those after `best` shift one slot toward head, those
           before `best` are untouched. */
        for (int i = best; i < buf->count - 1; i++) {
            int cur  = (buf->head + i)     % buf->capacity;
            int next = (buf->head + i + 1) % buf->capacity;
            buf->entries[cur] = buf->entries[next];
        }

        /* Retract tail by one to reflect the removed slot.
           The +capacity before the modulo prevents a negative result
           when tail is currently 0. */
        buf->tail = (buf->tail - 1 + buf->capacity) % buf->capacity;
        buf->count--;

        /* Wake producer if it was blocked on a full buffer */
        pthread_cond_signal(&buf->not_full);
        pthread_mutex_unlock(&buf->lock);
        return req;
    }

    /* FCFS path: dequeue from head (oldest item). */
    request_t req = buf->entries[buf->head];
    buf->head = (buf->head + 1) % buf->capacity;
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
