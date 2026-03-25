# OSTEP G.5 Concurrent Web Server - Team 0101
*This project explores the impact of size-based scheduling on concurrent web server performance, focusing on two scheduling policies: First-Come-First-Served (FCFS) and Shortest-File-First (SFF). Our web server implements a fixed-size thread pool with a bounded buffer and is based on a producer-consumer model.*

## Overview
Codebase Language: C
<!-- Expand on the description, research question, and concurrency model (3–5 sentences) -->

## Features
<!-- Bullet list of implemented features (thread pool, bounded buffer, FCFS/SFF, etc.) -->

## Architecture
<!-- Describe the producer-consumer model, thread pool, and how scheduling plugs in -->


## Requirements

- OS: Linux (tested on Ubuntu 22.04 LTS)
- Compiler: GCC 11+ with C99 support
- Libraries: pthreads (POSIX threads)
- Build tools: GNU Make

## Instructions

### Build the server

```bash
make
```

### Run the server

```bash
./server -p 8080 -t 4 -b 16 -s FCFS
```

| Flag | Description | Default |
|------|-------------|---------|
| `-p` | Port to listen on | 8080 |
| `-t` | Number of worker threads | 4 |
| `-b` | Bounded buffer capacity | 16 |
| `-s` | Scheduling policy (`FCFS` or `SFF`) | FCFS |

All flags are optional — running `./server` uses the defaults.

The server serves static files from the `www/` directory. Press `Ctrl+C` for graceful shutdown.

### Test with curl

```bash
# Request a file
curl http://localhost:8080/

# See full request/response headers
curl -v http://localhost:8080/

# Request a specific file
curl http://localhost:8080/hello.txt

# Test 404
curl http://localhost:8080/doesnotexist.html
```

### Run tests

```bash
# Run all tests (buffer unit tests, thread pool tests, integration test)
make test-all

# Run individually
make test-buffer    # bounded buffer unit tests
make test-pool      # thread pool unit tests
make test           # integration test (starts server, runs mock client, shuts down)
```

### Clean build artifacts

```bash
make clean
```

## Project Structure

```
project-webserver-team0101/
├── Makefile
├── README.md
├── docs/
│   └── diagrams/       # UML diagrams (PDF)
├── include/
│   ├── bounded_buffer.h
│   ├── http.h
│   └── thread_pool.h
├── src/
│   ├── main.c          # Entry point, arg parsing, accept loop (producer)
│   ├── http.c          # HTTP parsing, file serving, status codes
│   ├── bounded_buffer.c # Thread-safe circular queue (producer-consumer)
│   └── thread_pool.c   # Fixed-size worker thread pool (consumers)
├── tests/
│   ├── mock_client.c   # Simple HTTP client for integration testing
│   ├── test_buffer.c   # Bounded buffer unit tests
│   └── test_thread_pool.c # Thread pool unit tests
└── www/                # Static files served by the server
    
```

## Testing
<!-- How to run tests and reproduce scheduling comparison results (e.g. curl, ApacheBench) -->

## Team
- Helena Zhuo
- Ishit Arhatia
- Tomas Colorado
- Rudrajit Banerjee

## Links
[Research Proposal](https://github.com/cs5600-sp26/project-webserver-team0101/blob/main/docs/updated_cs5800_research_question.pdf)

[Contribution Tracker - Google Sheet](https://docs.google.com/spreadsheets/d/1h_nNWYMj3j0Qd1xJ3MwdG4fjjc7fUvO87jzA1AlHy-k/edit?gid=0#gid=0)
