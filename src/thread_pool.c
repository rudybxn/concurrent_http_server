/* =============================================================
 * thread_pool.c
 * Stubbed — functions declared but not implemented.
 * ============================================================= */

#include "thread_pool.h"
#include <stdio.h>

ThreadPool *thread_pool_init(int num_threads) {
    /* TODO: spawn worker threads, initialize queue and mutex */
    printf("thread_pool_init: stub, %d threads requested\n", num_threads);
    return NULL;
}

void thread_pool_submit(ThreadPool *pool, int client_fd) {
    /* TODO: enqueue client_fd for a worker thread to handle */
    (void)pool;
    (void)client_fd;
}

void thread_pool_destroy(ThreadPool *pool) {
    /* TODO: signal workers to stop, join threads, free resources */
    (void)pool;
}