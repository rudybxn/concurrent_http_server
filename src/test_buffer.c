/* =============================================================
 * test_buffer.c
 *
 * Standalone tests for the bounded buffer. No threads needed
 * for most of these — we're testing the data structure logic
 * (enqueue, dequeue, ordering, wraparound) in isolation.
 *
 * The last test (test_blocking) uses two threads to verify
 * that put blocks on a full buffer and get blocks on empty.
 *
 * Build:  gcc -Wall -Wextra -pthread -o test_buffer \
 *             test_buffer.c bounded_buffer.c
 * Run:    ./test_buffer
 * ============================================================= */

#include "bounded_buffer.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* Helper: build a request_t with just a client_fd set */
static request_t make_req(int fd) {
    request_t r;
    memset(&r, 0, sizeof(r));
    r.client_fd = fd;
    return r;
}

/* ---- Test 1: basic put/get ordering (FCFS) -------------------- */
static void test_fifo_order(void) {
    bounded_buffer_t *buf = buffer_init(4);
    assert(buf != NULL);

    buffer_put(buf, make_req(10));
    buffer_put(buf, make_req(20));
    buffer_put(buf, make_req(30));

    /* Should come out in the same order */
    assert(buffer_get(buf).client_fd == 10);
    assert(buffer_get(buf).client_fd == 20);
    assert(buffer_get(buf).client_fd == 30);

    buffer_destroy(buf);
    printf("PASS: test_fifo_order\n");
}

/* ---- Test 2: fill to capacity, then drain --------------------- */
static void test_fill_and_drain(void) {
    bounded_buffer_t *buf = buffer_init(3);

    buffer_put(buf, make_req(1));
    buffer_put(buf, make_req(2));
    buffer_put(buf, make_req(3));
    /* buffer is now full (count == capacity) */

    assert(buffer_get(buf).client_fd == 1);
    assert(buffer_get(buf).client_fd == 2);
    assert(buffer_get(buf).client_fd == 3);
    /* buffer is now empty */

    buffer_destroy(buf);
    printf("PASS: test_fill_and_drain\n");
}

/* ---- Test 3: circular wraparound ----------------------------- 
   Put and get in alternating batches so head and tail wrap
   around the array boundary. Catches off-by-one errors in
   the modular arithmetic. */
static void test_wraparound(void) {
    bounded_buffer_t *buf = buffer_init(3);

    /* Fill and partially drain several times to force wraparound */
    buffer_put(buf, make_req(100));
    buffer_put(buf, make_req(200));
    assert(buffer_get(buf).client_fd == 100);  /* head moves to 1 */
    assert(buffer_get(buf).client_fd == 200);  /* head moves to 2 */

    buffer_put(buf, make_req(300));  /* tail wraps: slot 2 */
    buffer_put(buf, make_req(400));  /* tail wraps: slot 0 */
    buffer_put(buf, make_req(500));  /* tail wraps: slot 1 */
    /* buffer full again, head=2, tail=2 */

    assert(buffer_get(buf).client_fd == 300);
    assert(buffer_get(buf).client_fd == 400);
    assert(buffer_get(buf).client_fd == 500);

    buffer_destroy(buf);
    printf("PASS: test_wraparound\n");
}

/* ---- Test 4: metadata fields preserved ----------------------- 
   Verify that filename, file_size, and arrival_ms survive
   the round-trip through the buffer. */
static void test_metadata(void) {
    bounded_buffer_t *buf = buffer_init(2);

    request_t in;
    memset(&in, 0, sizeof(in));
    in.client_fd  = 42;
    in.file_size  = 1048576;  /* 1 MB */
    in.arrival_ms = 1234567890L;
    strncpy(in.filename, "/index.html", sizeof(in.filename) - 1);

    buffer_put(buf, in);
    request_t out = buffer_get(buf);

    assert(out.client_fd  == 42);
    assert(out.file_size  == 1048576);
    assert(out.arrival_ms == 1234567890L);
    assert(strcmp(out.filename, "/index.html") == 0);

    buffer_destroy(buf);
    printf("PASS: test_metadata\n");
}

/* ---- Test 5: init rejects bad capacity ----------------------- */
static void test_bad_capacity(void) {
    assert(buffer_init(0)  == NULL);
    assert(buffer_init(-1) == NULL);
    printf("PASS: test_bad_capacity\n");
}

/* ---- Test 6: blocking behavior (requires threads) -------------
   Producer pushes into a full buffer on a separate thread;
   main thread sleeps briefly, then drains one slot. If put
   was truly blocked, the producer finishes only after we drain.

   Similarly: consumer tries to get from empty buffer on a
   separate thread; main thread sleeps, then puts one item. */

static void *producer_thread(void *arg) {
    bounded_buffer_t *buf = (bounded_buffer_t *)arg;
    /* Buffer capacity is 1 and already has one item,
       so this should block until the main thread drains it */
    buffer_put(buf, make_req(999));
    return NULL;
}

static void *consumer_thread(void *arg) {
    bounded_buffer_t *buf = (bounded_buffer_t *)arg;
    /* Buffer is empty, so this should block until
       the main thread puts something in */
    request_t r = buffer_get(buf);
    assert(r.client_fd == 777);
    return NULL;
}

static void test_blocking(void) {
    /* Test put-blocks-when-full */
    bounded_buffer_t *buf = buffer_init(1);
    buffer_put(buf, make_req(111));  /* fill it */

    pthread_t tid;
    pthread_create(&tid, NULL, producer_thread, buf);

    usleep(100000);  /* 100ms — give producer time to block */

    /* Producer should be stuck in buffer_put right now.
       Drain one slot to unblock it. */
    assert(buffer_get(buf).client_fd == 111);
    pthread_join(tid, NULL);

    /* The producer's item should now be in the buffer */
    assert(buffer_get(buf).client_fd == 999);
    buffer_destroy(buf);

    /* Test get-blocks-when-empty */
    buf = buffer_init(1);

    pthread_create(&tid, NULL, consumer_thread, buf);

    usleep(100000);  /* 100ms — give consumer time to block */

    /* Consumer should be stuck in buffer_get right now.
       Put an item to unblock it. */
    buffer_put(buf, make_req(777));
    pthread_join(tid, NULL);

    buffer_destroy(buf);
    printf("PASS: test_blocking\n");
}

/* ---- Run all tests ------------------------------------------- */
int main(void) {
    printf("=== Bounded Buffer Tests ===\n\n");

    test_fifo_order();
    test_fill_and_drain();
    test_wraparound();
    test_metadata();
    test_bad_capacity();
    test_blocking();

    printf("\nAll tests passed.\n");
    return 0;
}