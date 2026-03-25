/* =============================================================
 * test_thread_pool.c
 *
 * Tests for the thread pool + bounded buffer working together.
 * We temporarily replace http_handle() with a test version
 * that increments a counter instead of doing real HTTP work.
 *
 * Build:  gcc -Wall -Wextra -pthread -o test_pool \
 *             test_thread_pool.c bounded_buffer.c thread_pool.c
 * Run:    ./test_pool
 * ============================================================= */

#include "thread_pool.h"
#include "bounded_buffer.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* ---- Shared test state ----------------------------------------
   Workers call http_handle(fd), which in production reads/writes
   the socket. For testing, we define our own http_handle() here
   that just counts how many times it was called. Since multiple
   workers will increment this concurrently, we protect it with
   a mutex. */

static int handled_count = 0;
static pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;

/* This overrides the real http_handle() at link time because
   we don't link http.c into this test binary. */
void http_handle(int client_fd) {
    (void)client_fd;  /* unused in test */

    pthread_mutex_lock(&count_lock);
    handled_count++;
    pthread_mutex_unlock(&count_lock);
}

/* Helper: build a request_t with just a client_fd */
static request_t make_req(int fd) {
    request_t r;
    memset(&r, 0, sizeof(r));
    r.client_fd = fd;
    return r;
}

/* ---- Test 1: all submitted requests get handled --------------- */
static void test_all_requests_handled(void) {
    handled_count = 0;
    int num_threads = 4;
    int buf_capacity = 8;
    int num_requests = 100;

    bounded_buffer_t *buf  = buffer_init(buf_capacity);
    ThreadPool       *pool = thread_pool_init(num_threads, buf);
    assert(pool != NULL);

    /* Submit work */
    for (int i = 0; i < num_requests; i++) {
        thread_pool_submit(pool, make_req(i));
    }

    /* Give workers time to drain the buffer.
       In a real system you'd have a more robust drain check,
       but for a test this is fine. */
    usleep(500000);  /* 500ms */

    thread_pool_destroy(pool);
    buffer_destroy(buf);

    assert(handled_count == num_requests);
    printf("PASS: test_all_requests_handled (%d/%d)\n",
           handled_count, num_requests);
}

/* ---- Test 2: pool works with buffer size 1 --------------------
   This is the minimum buffer — every put immediately fills it,
   every get immediately empties it. Tests tight producer-consumer
   handoff. */
static void test_buffer_size_one(void) {
    handled_count = 0;
    int num_requests = 50;

    bounded_buffer_t *buf  = buffer_init(1);
    ThreadPool       *pool = thread_pool_init(2, buf);
    assert(pool != NULL);

    for (int i = 0; i < num_requests; i++) {
        thread_pool_submit(pool, make_req(i));
    }

    usleep(500000);

    thread_pool_destroy(pool);
    buffer_destroy(buf);

    assert(handled_count == num_requests);
    printf("PASS: test_buffer_size_one (%d/%d)\n",
           handled_count, num_requests);
}

/* ---- Test 3: single thread pool -------------------------------
   Degenerate case: 1 worker, should still process everything
   correctly (just sequentially). */
static void test_single_thread(void) {
    handled_count = 0;
    int num_requests = 30;

    bounded_buffer_t *buf  = buffer_init(4);
    ThreadPool       *pool = thread_pool_init(1, buf);
    assert(pool != NULL);

    for (int i = 0; i < num_requests; i++) {
        thread_pool_submit(pool, make_req(i));
    }

    usleep(500000);

    thread_pool_destroy(pool);
    buffer_destroy(buf);

    assert(handled_count == num_requests);
    printf("PASS: test_single_thread (%d/%d)\n",
           handled_count, num_requests);
}

/* ---- Test 4: graceful shutdown with empty buffer --------------
   Create a pool, submit nothing, destroy immediately.
   Should not hang or crash. */
static void test_shutdown_empty(void) {
    bounded_buffer_t *buf  = buffer_init(4);
    ThreadPool       *pool = thread_pool_init(4, buf);
    assert(pool != NULL);

    /* No work submitted — workers are all blocked on empty buffer */
    usleep(100000);  /* let them settle into wait */

    thread_pool_destroy(pool);
    buffer_destroy(buf);

    printf("PASS: test_shutdown_empty\n");
}

/* ---- Test 5: concurrent proof ---------------------------------
   Submit requests with a small sleep in http_handle to simulate
   work. With 4 threads, N requests should finish much faster
   than with 1 thread. We don't assert exact timing, just that
   it completes. */
static int slow_handled_count = 0;
static pthread_mutex_t slow_lock = PTHREAD_MUTEX_INITIALIZER;

/* We can't easily swap http_handle mid-binary, so this test
   just verifies the pool handles high volume without deadlock
   or lost requests using the fast handler above. */
static void test_high_volume(void) {
    handled_count = 0;
    int num_requests = 1000;

    bounded_buffer_t *buf  = buffer_init(16);
    ThreadPool       *pool = thread_pool_init(4, buf);
    assert(pool != NULL);

    for (int i = 0; i < num_requests; i++) {
        thread_pool_submit(pool, make_req(i));
    }

    usleep(1000000);  /* 1 second for 1000 fast requests */

    thread_pool_destroy(pool);
    buffer_destroy(buf);

    assert(handled_count == num_requests);
    printf("PASS: test_high_volume (%d/%d)\n",
           handled_count, num_requests);
}

/* ---- Run all tests ------------------------------------------- */
int main(void) {
    printf("=== Thread Pool Tests ===\n\n");

    test_all_requests_handled();
    test_buffer_size_one();
    test_single_thread();
    test_shutdown_empty();
    test_high_volume();

    printf("\nAll tests passed.\n");
    return 0;
}