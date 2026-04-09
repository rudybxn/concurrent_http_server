#define _POSIX_C_SOURCE 200809L  /* for getaddrinfo, sigaction, etc. */
/* =============================================================
 * main.c
 *
 * Entry point for the concurrent web server.
 *
 * Flow:
 *   1. Parse command-line args (-p port, -t threads, -b bufsize)
 *   2. Create TCP listening socket
 *   3. Initialize bounded buffer and thread pool
 *   4. Accept loop (producer): accept connection → build
 *      request_t → submit to thread pool
 *   5. On SIGINT: graceful shutdown
 *
 * The accept loop is the producer. Worker threads in the pool
 * are the consumers. The bounded buffer decouples them.
 * ============================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/time.h>

#include "http.h"
#include "bounded_buffer.h"
#include "thread_pool.h"

/* ---- Defaults ------------------------------------------------- */
#define DEFAULT_PORT     8080
#define DEFAULT_THREADS  4
#define DEFAULT_BUFSIZE  16
#define BACKLOG          512

/* ---- Global state for signal handler --------------------------
   These are global because the SIGINT handler needs to access
   them to initiate shutdown. In normal operation, only main()
   writes to them (except running, which the handler sets to 0). */
static volatile int running = 1;
static int          server_fd_global = -1;
static ThreadPool       *pool_global = NULL;
static bounded_buffer_t *buf_global  = NULL;

/* ---- Signal handler -------------------------------------------
   Called asynchronously when the user hits Ctrl+C.
   Sets running=0 so the accept loop exits on its next iteration.
   Closes the listening socket to unblock accept() if it's
   currently waiting — accept() will return -1 with errno=EBADF,
   and the loop checks !running and breaks out cleanly. */
static void handle_sigint(int sig) {
    (void)sig;
    running = 0;
    if (server_fd_global >= 0) {
        close(server_fd_global);
        server_fd_global = -1;
    }
}

/* ---- Get current time in milliseconds -------------------------
   Used to timestamp each request on arrival. This becomes the
   arrival_ms field in request_t, which FCFS uses for ordering
   and which we'll use in Sprint 4 for latency measurements. */
static long now_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000L + tv.tv_usec / 1000L;
}

/* ---- Usage ---------------------------------------------------- */
static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [-p port] [-t threads] [-b bufsize] [-s schedalg]\n"
        "  -p port       Port to listen on (default: %d)\n"
        "  -t threads    Number of worker threads (default: %d)\n"
        "  -b bufsize    Bounded buffer capacity (default: %d)\n"
        "  -s schedalg   Scheduling policy: FCFS or SFF (default: FCFS)\n",
        prog, DEFAULT_PORT, DEFAULT_THREADS, DEFAULT_BUFSIZE);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int port         = DEFAULT_PORT;
    int num_threads  = DEFAULT_THREADS;
    int buf_capacity = DEFAULT_BUFSIZE;
    char schedalg[16] = "FCFS";

    /* ================================================================
     * Step 1: Parse command-line arguments
     *
     * getopt() is the standard POSIX way to parse flags. The string
     * "p:t:b:s:" means each flag takes a required argument (the colon).
     * optarg points to the argument string after each flag.
     *
     * Examples:
     *   ./server -p 9000 -t 8 -b 32 -s SFF
     *   ./server                              (uses all defaults)
     * ================================================================ */
    int opt;
    while ((opt = getopt(argc, argv, "p:t:b:s:")) != -1) {
        switch (opt) {
            case 'p': port         = atoi(optarg); break;
            case 't': num_threads  = atoi(optarg); break;
            case 'b': buf_capacity = atoi(optarg); break;
            case 's': strncpy(schedalg, optarg, sizeof(schedalg) - 1); break;
            default:  usage(argv[0]);
        }
    }

    if (port <= 0 || num_threads <= 0 || buf_capacity <= 0) {
        fprintf(stderr, "Error: port, threads, and bufsize must be > 0\n");
        usage(argv[0]);
    }

    printf("\n======= CONFIGURATION =======\n");
    printf("Port: %d\nThreads: %d\nBuffer Capacity: %d\nScheduling Policy: %s\n",
           port, num_threads, buf_capacity, schedalg);

    /* ================================================================
     * Step 2: Set up SIGINT handler for graceful shutdown
     *
     * sigaction() is preferred over signal() because its behavior is
     * well-defined across platforms. When the user presses Ctrl+C,
     * the kernel delivers SIGINT, which calls handle_sigint().
     *
     * sa_flags = 0 means we DON'T set SA_RESTART, so accept() will
     * be interrupted and return -1 instead of automatically retrying.
     * This is intentional — we want the accept loop to notice
     * running=0 and exit.
     *
     * sigemptyset() clears the signal mask so no other signals are
     * blocked while our handler runs.
     * ================================================================ */
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sa.sa_flags   = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGTERM, &sa, NULL);

    /* Ignore SIGPIPE so write() returns -1/EPIPE instead of killing
       the process when a client closes the connection mid-transfer.
       http.c already checks write() return values and handles it. */
    signal(SIGPIPE, SIG_IGN);

    /* ================================================================
     * Step 3: Create TCP listening socket
     *
     * Same setup as Sprint 2, now using the port from command-line args.
     *
     * socket()     — ask the OS for a new TCP/IPv4 socket
     * SO_REUSEADDR — let us rebind immediately after restart (skip
     *                the ~60s TIME_WAIT state)
     * bind()       — attach the socket to our port on all interfaces
     * listen()     — mark it as passive (accepting connections)
     *
     * After listen(), the OS is completing TCP handshakes in the
     * background. Connections queue up in the backlog (up to BACKLOG)
     * until we call accept().
     * ================================================================ */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    server_fd_global = server_fd;

    int reuse = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /* ================================================================
     * Step 4: Initialize bounded buffer and thread pool
     *
     * The bounded buffer is the shared queue between the producer
     * (this main thread) and the consumers (worker threads).
     *
     * buffer_init() allocates the circular array and initializes
     * the mutex and condition variables (not_empty, not_full).
     *
     * thread_pool_init() allocates the worker thread array and
     * spawns num_threads workers via pthread_create(). Each worker
     * immediately starts its loop: lock → wait on not_empty →
     * dequeue → unlock → http_handle(). They block right away
     * because the buffer starts empty.
     *
     * After this point, the system is ready: workers are waiting,
     * the socket is listening, we just need to start accepting.
     * ================================================================ */
    bounded_buffer_t *buf = buffer_init(buf_capacity);
    if (!buf) {
        fprintf(stderr, "Failed to initialize buffer\n");
        exit(EXIT_FAILURE);
    }
    buf_global = buf;

    ThreadPool *pool = thread_pool_init(num_threads, buf);
    if (!pool) {
        fprintf(stderr, "Failed to initialize thread pool\n");
        buffer_destroy(buf);
        exit(EXIT_FAILURE);
    }
    pool_global = pool;

    printf("\n======= SERVER RUNNING =======\n");
    printf("Ctrl+click (cmd+click on Mac) to open in your browser: http://localhost:%d\n\n", port);
    printf("Press Ctrl+C to stop the server.\n");

    /* ================================================================
     * Step 5: Accept loop (producer side of producer-consumer)
     *
     * This is the heart of the server. The main thread's only job
     * is to accept new connections and submit them to the thread pool.
     *
     * accept() blocks until a client connects. When one does, the OS
     * completes the TCP handshake and returns a new socket fd for
     * that specific connection. server_fd stays open for the next one.
     *
     * We build a request_t with the client fd and a timestamp, then
     * call thread_pool_submit() which calls buffer_put(). If the
     * buffer is full, buffer_put() blocks here — this is backpressure.
     * The main thread pauses until a worker finishes a request and
     * frees a slot, which is exactly the behavior we want under load.
     *
     * Note: right now we only populate client_fd and arrival_ms.
     * For SFF scheduling in Sprint 4, we'll read the HTTP request
     * line and call stat() HERE (before enqueueing) to populate
     * filename and file_size. The proposal requires stat() to happen
     * outside the critical section so no I/O occurs while the
     * mutex is held.
     * ================================================================ */
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr,
                               &client_len);
        if (client_fd < 0) {
            /* accept() returns -1 when we close server_fd in the
               signal handler, or on a transient error. If we're
               shutting down, just break. Otherwise, skip and retry. */
            if (!running) break;
            perror("accept");
            continue;
        }

        request_t req;
        memset(&req, 0, sizeof(req));
        req.client_fd  = client_fd;
        req.arrival_ms = now_ms();

        thread_pool_submit(pool, req);
    }

    /* ================================================================
     * Step 6: Graceful shutdown
     *
     * We reach here when running=0 (Ctrl+C was pressed).
     *
     * thread_pool_destroy() sets the shutdown flag, broadcasts on
     * not_empty to wake all workers (they're blocked waiting for
     * work), and joins every thread. Workers drain any remaining
     * requests in the buffer before exiting.
     *
     * buffer_destroy() frees the circular array, destroys the mutex
     * and condition variables, and frees the buffer struct.
     *
     * server_fd was already closed by the signal handler to unblock
     * accept(), but we guard against double-close just in case.
     * ================================================================ */
    printf("\nShutting down...\n");
    thread_pool_destroy(pool);
    buffer_destroy(buf);

    if (server_fd_global >= 0) {
        close(server_fd_global);
    }

    printf("Server stopped.\n");
    return 0;
}
