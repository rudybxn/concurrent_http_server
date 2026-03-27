# OSTEP G.5 Concurrent Web Server — Team 0101

This project explores the impact of size-based scheduling on concurrent web
server performance. We implement a multi-threaded HTTP server using a
fixed-size thread pool and a bounded buffer following the producer-consumer
pattern, as described in the OSTEP curriculum (G.5). The main thread accepts
incoming TCP connections and enqueues them into a shared buffer; a pool of
worker threads dequeue and service requests. Our research extension compares
two scheduling policies — **First-Come-First-Served (FCFS)** and
**Shortest-File-First (SFF)** — to evaluate their impact on mean response
time and tail latency under varying load and file-size distributions.

---

## Features

- Fixed-size thread pool with configurable number of worker threads (`-t N`)
- Producer-consumer bounded buffer with configurable capacity (`-b N`)
- Two scheduling policies selectable at runtime (`-s FCFS` or `-s SFF`)
- FCFS: dequeues requests in strict arrival order — O(1)
- SFF: scans the buffer and dequeues the smallest pending file — O(n)
- Binary-safe static file serving (HTML, images, CSS, JS, etc.)
- Correct HTTP status codes: `200 OK`, `404 Not Found`, `405 Method Not Allowed`
- Graceful shutdown on `SIGINT` (Ctrl+C) with thread pool teardown
- Mutex and condition variables (`notFull` / `notEmpty`) for safe concurrency

---

## Architecture

The server follows a **producer-consumer** model with three logical layers:

1. **Main thread (producer):** Opens a listening socket, accepts connections
   in a loop, and enqueues each connected file descriptor into the bounded
   buffer. The `stat()` call to obtain file size is made here, before the
   mutex is acquired, so no filesystem I/O occurs inside the critical section.

2. **Bounded buffer (shared resource):** A fixed-capacity circular array
   protected by a `pthread_mutex_t` and two condition variables. Workers
   block on `notEmpty` when the buffer is empty; the main thread blocks on
   `notFull` when the buffer is at capacity. Scheduling policy (FCFS or SFF)
   is applied at dequeue time.

3. **Worker threads (consumers):** Each worker loops indefinitely: dequeue a
   request, parse the HTTP request line, locate the file, send the response,
   close the connection. All state is stack-local, making workers fully
   thread-safe without additional locking.

```
Main Thread                Bounded Buffer             Worker Threads
(Producer)                (Shared Resource)           (Consumers)
    │                           │                           │
    │  accept() → stat()        │                           │
    │──── enqueue(fd, size) ───►│                           │
    │                           │◄─── dequeue(FCFS|SFF) ───│
    │                           │                           │
    │                    [mutex + condvars]          parse → stat()
    │                    [notFull / notEmpty]        send file
    │                                                close fd
```

---

## Requirements

- **OS:** Linux (Ubuntu 22.04 LTS recommended); macOS not supported for builds
- **Compiler:** GCC 11.4+ (`gcc`)
- **Libraries:** POSIX threads (`-lpthread`), standard C library
- **Build tool:** GNU Make
- **Benchmarking:** [`wrk`](https://github.com/wg/wrk) 4.2.0 (for `/benchmarks/`)

---

## Instructions


### Build the server

```bash
make
```

### Run the server

```bash
./wserver -p <port> -t <threads> -b <buffer_size> -s <FCFS|SFF>
```

**Example — 4 worker threads, buffer of 16, FCFS scheduling on port 8080:**
```bash
./wserver -p 8080 -t 4 -b 16 -s FCFS
```

**Example — SFF scheduling:**
```bash
./wserver -p 8080 -t 4 -b 16 -s SFF
```

Defaults: `-t 1`, `-b 1`, `-s FCFS`. The server will start listening on the
specified port. Press `Ctrl+C` to stop it cleanly.

### Run tests

```bash
make test
```

Builds and starts the server, waits for it to be ready, then runs the mock
client against it.

### Run benchmarks

```bash
cd benchmarks/
bash benchmark.sh
```

See [`benchmarks/README.md`](benchmarks/README.md) for full instructions on
what is measured and how to interpret the raw output in `benchmarks/raw/`.

### Clean build artifacts

```bash
make clean
```

### How to connect to http server via web client
After running a server do the following steps to access the http server from a web client.
1. Open FireFox web browser from the same machine that the server is running on.
1. In the top search bar of the web browser, type in `http://localhost:8080/`.
1. Voila! You are now able to interact with the http server.
    - Please note that the http server currently only supports GET requests for PNG files from the web client.

---

## Project Structure

```
project-webserver-team0101/
├── benchmarks/
│   ├── benchmark.sh        # Automated wrk load-testing script
│   ├── README.md           # How to run benchmarks and interpret results
│   └── raw/                # Raw wrk output from each trial
├── docs/
│   ├── bibliography.md     # Annotated bibliography (Sprint 2)
│   ├── glossary.md         # Project glossary
│   ├── ai-reflections.md   # Per-sprint AI reflection log
│   ├── figs/               # UML diagrams (PDF) referenced by paper
│   └── paper-draft.pdf     # Compiled IEEE-format paper (Sprint 3)
├── feedback/               # Instructor sprint feedback
├── src/
│   ├── main.c              # Entry point: arg parsing, accept loop, enqueue
│   ├── bounded_buffer.c    # Bounded buffer: mutex, condvars, FCFS/SFF dequeue
│   ├── thread_pool.c       # Thread pool init, worker thread creation, teardown
│   └── http.c              # HTTP request parsing, file send, status codes
├── include/
│   ├── bounded_buffer.h
│   ├── thread_pool.h
│   └── http.h
├── tests/
│   └── mock_client.c       # Mock client for server correctness testing
├── files/                  # Static files served by the server
├── Makefile
├── HowToGit.md             # Git workflow guide for the team
└── README.md
```

---

## Testing

The test suite (`make test`) runs a mock client against the live server and
verifies correct HTTP responses. For manual testing, open a browser and
navigate to `http://localhost:8080/<filename>` after starting the server.

To reproduce the scheduling comparison:

```bash
# Terminal 1 — start server with FCFS
./wserver -p 8080 -t 4 -b 16 -s FCFS

# Terminal 2 — run wrk load test
wrk -t4 -c50 -d30s http://localhost:8080/small.html
```

Replace `-s FCFS` with `-s SFF` to compare policies. Full automated
comparison is handled by `benchmarks/benchmark.sh`.

---

## Team

| Name | Program |
|---|---|
| Rudrajit Banerjee | MS Computer Science, Northeastern University |
| Ishit Arhatia | MS Data Science, Northeastern University |
| Helena Zhuo | MS Computer Science, Northeastern University |
| Tomas Colorado | MS Computer Science, Northeastern University |

---

## Links

- [Research Proposal](https://github.com/cs5600-sp26/project-webserver-team0101/blob/main/docs/updated_cs5800_research_question.pdf)
- [Contribution Tracker](https://docs.google.com/spreadsheets/d/1h_nNWYMj3j0Qd1xJ3MwdG4fjjc7fUvO87jzA1AlHy-k/edit?gid=0#gid=0)
