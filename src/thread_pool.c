/* =============================================================
 * thread_pool.c
 *
 * Fixed-size pool of worker threads consuming from a shared
 * bounded buffer. Each worker runs an infinite loop:
 *
 *     lock → wait while empty → dequeue → unlock → handle → repeat
 *
 * Shutdown sequence:
 *   1. thread_pool_destroy() sets pool->shutdown = 1
 *   2. Broadcasts on not_empty to wake all blocked workers
 *   3. Each worker sees shutdown flag and exits its loop
 *   4. Main thread joins all workers, then frees the pool
 * ============================================================= */

#include "thread_pool.h"
#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* ----------------------------------------------------------------
 * worker_func — the function each worker thread runs
 *
 * The worker needs access to both the buffer (to dequeue) and
 * the pool (to check the shutdown flag). We pass the pool
 * pointer as the thread argument since it contains both.
 *
 * Note: we can't just call buffer_get() directly because
 * buffer_get blocks unconditionally on empty. During shutdown,
 * we need workers to wake up and check the flag. So we inline
 * the lock/wait/check logic here instead of using buffer_get().
 * ---------------------------------------------------------------- */
static void *worker_func(void *arg) {
    ThreadPool       *pool = (ThreadPool *)arg;
    bounded_buffer_t *buf  = pool->buffer;

    while (1) {
        pthread_mutex_lock(&buf->lock);

        /* Wait while buffer is empty AND we haven't been told to stop.
           The while-loop handles spurious wakeups. */
        while (buf->count == 0 && !pool->shutdown) {
            pthread_cond_wait(&buf->not_empty, &buf->lock);
        }

        /* Check shutdown: if the buffer is empty and we're shutting
           down, release the lock and exit. If there are still items
           in the buffer, drain them first — don't drop requests. */
        if (pool->shutdown && buf->count == 0) {
            pthread_mutex_unlock(&buf->lock);
            break;
        }

        request_t req;

        if (pool->use_sff) {
            /* SFF: release the lock so buffer_get_sff can acquire it.
               buffer_get_sff handles the case where another worker
               races in and empties the buffer before us. */
            pthread_mutex_unlock(&buf->lock);
            req = buffer_get_sff(buf);
        } else {
            /* FCFS: dequeue from head (oldest item) */
            req = buf->entries[buf->head];
            buf->head = (buf->head + 1) % buf->capacity;
            buf->count--;

            /* Wake producer if it was blocked on a full buffer */
            pthread_cond_signal(&buf->not_full);

            pthread_mutex_unlock(&buf->lock);
        }

        /* ---- Handle the request (outside the critical section) ---- */
        http_handle(req.client_fd);
    }

    return NULL;
}

/* ----------------------------------------------------------------
 * thread_pool_init
 *
 * Allocates the pool struct, stores the buffer pointer, and
 * spawns num_threads workers. Each worker starts immediately
 * and blocks on the empty buffer until work arrives.
 * ---------------------------------------------------------------- */
ThreadPool *thread_pool_init(int num_threads, bounded_buffer_t *buffer,
                             const char *schedalg) {
    if (num_threads <= 0 || !buffer) {
        fprintf(stderr, "thread_pool_init: invalid arguments\n");
        return NULL;
    }

    ThreadPool *pool = malloc(sizeof(ThreadPool));
    if (!pool) {
        perror("thread_pool_init: malloc pool");
        return NULL;
    }

    pool->threads     = malloc(sizeof(pthread_t) * num_threads);
    if (!pool->threads) {
        perror("thread_pool_init: malloc threads");
        free(pool);
        return NULL;
    }

    pool->num_threads = num_threads;
    pool->buffer      = buffer;
    pool->shutdown    = 0;
    pool->use_sff     = (schedalg && strcmp(schedalg, "SFF") == 0);

    /* Spawn workers */
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_func, pool) != 0) {
            perror("thread_pool_init: pthread_create");
            /* Clean up threads we already created */
            pool->num_threads = i;
            thread_pool_destroy(pool);
            return NULL;
        }
    }

    printf("thread_pool_init: %d workers spawned\n", num_threads);
    return pool;
}

/* ----------------------------------------------------------------
 * thread_pool_submit
 *
 * Called by the producer (main thread). Wraps buffer_put(),
 * which blocks if the buffer is full — this is the backpressure
 * mechanism that prevents unbounded queue growth.
 * ---------------------------------------------------------------- */
void thread_pool_submit(ThreadPool *pool, request_t req) {
    buffer_put(pool->buffer, req);
}

/* ----------------------------------------------------------------
 * thread_pool_destroy
 *
 * Graceful shutdown:
 *   1. Set the shutdown flag so workers know to exit
 *   2. Broadcast on not_empty — any worker blocked in
 *      pthread_cond_wait will wake up, see shutdown=1
 *      and count==0, and break out of its loop
 *   3. Join all threads (waits for them to finish)
 *   4. Free the threads array and the pool struct
 *
 * Does NOT call buffer_destroy() — the caller owns that.
 * ---------------------------------------------------------------- */
void thread_pool_destroy(ThreadPool *pool) {
    if (!pool) return;

    /* Signal all workers to shut down */
    pthread_mutex_lock(&pool->buffer->lock);
    pool->shutdown = 1;
    pthread_cond_broadcast(&pool->buffer->not_empty);
    pthread_mutex_unlock(&pool->buffer->lock);

    /* Wait for every worker to finish */
    for (int i = 0; i < pool->num_threads; i++) {
        pthread_join(pool->threads[i], NULL);
    }

    printf("thread_pool_destroy: all workers joined\n");

    free(pool->threads);
    free(pool);
}