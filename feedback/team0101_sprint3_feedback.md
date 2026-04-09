# Sprint 3 Feedback — Team 0101

## Strengths

- **Paper**: Full IEEE LaTeX source (`docs/paper-draft.tex`) and PDF committed. All six sections present: Abstract, Introduction, Related Work (three thematic subsections: Architecture of Concurrent Servers, Size-Based Scheduling Theory, Modern Production Scheduling Systems), System Design, Methodology, Preliminary Results. Preliminary Results includes actual baseline numbers with wrk measurements.
- **Benchmarks**: 75 configurations (3 workloads × 6 concurrency levels × 5 trials) with all 75 raw `.txt` files committed under `benchmarks/raw/`. Workloads include uniform-small (4 KB), uniform-large (1 MB), and heavy-tailed (90/10 mix via Lua script). A `summary.csv` aggregates results. A `verify.sh` script confirms all files are present. Environment fully documented (VirtualBox Ubuntu 22.04, 4 vCPUs, 8 GB, GCC 11.4 -O2, wrk 4.1.03). This is exemplary benchmark rigor.
- **Functional system**: `bounded_buffer.c` (circular buffer, pthread mutex, `notFull`/`notEmpty` condvars, FCFS/SFF at dequeue time), `thread_pool.c` (worker thread lifecycle), `http.c` (request parsing, file send, status codes 200/404/405). `stat()` is called before the mutex is acquired, keeping I/O out of the critical section. Three test files (`test_buffer.c`, `test_thread_pool.c`, `mock_client.c`) verify correctness. `SIGPIPE` is ignored to prevent crashes on client disconnect.
- **AI Reflections**: All four members (Helena, Ishit, Tomas, Rudrajit) have explicit Sprint 3 entries, each specific about tools used, tasks attempted, and where AI fell short.
- **Administration**: `Makefile` with `make`, `make test`, `make clean`. README documents all CLI flags (`-p`, `-t`, `-b`, `-s`). Language (C) stated. Contribution tracker link present.

## Areas for Improvement

- **SFF documentation vs. implementation mismatch**: The README describes SFF as currently selectable at runtime ("SFF: scans the buffer and dequeues the smallest pending file — O(n)"), but SFF is not yet implemented in the code. Both `bounded_buffer.c` and `thread_pool.c` dequeue FCFS only; the `-s SFF` flag is parsed but has no effect. `main.c` and the header comments explicitly note that `file_size` population (required for SFF) is deferred to Sprint 4. The README should be updated to reflect this before the final paper submission.
- SIGTERM is handled gracefully via `sigaction`, but SIGINT (Ctrl+C) is not wired to the same handler — the README says "Press Ctrl+C to stop it cleanly," which is slightly misleading.
- The README note "the http server currently only supports GET requests for PNG files from the web client" understates what the server actually serves (HTML, CSS, binary files). Worth updating for accuracy.
