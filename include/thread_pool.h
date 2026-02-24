/* =============================================================
 * thread_pool.h
 * Stubbed: no implementation yet.
 * ============================================================= */
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

typedef struct ThreadPool ThreadPool;

ThreadPool *thread_pool_init(int num_threads);
void        thread_pool_submit(ThreadPool *pool, int client_fd);
void        thread_pool_destroy(ThreadPool *pool);

#endif