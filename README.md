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
<!-- OS, compiler/version, libraries (pthreads), build tools -->
- Compiler: GNU C Compiler (gcc)

## Instructions
### Building
<!-- Steps and commands to compile the project (e.g. make, gcc flags) -->
### Running
<!-- How to start the server, CLI flags/options with descriptions and defaults -->
<!-- MAKE SURE TO UPDATE BEFORE SUBMISSION -->
**Sprint 2 instructions**: To compile and run this program, run the below:
```
gcc main.c http.c thread_pool.c -o server
./server
```

## Project Structure
<!-- Directory tree with a one-line description of each file or folder -->
```
project-webserver-team0101/
├── docs/
│   └── diagrams/  # UML diagrams (PDF)
├── src/
│   └── *.c files
├── include/
│   └── *.h files
├── tests/
│   └── test files
├── www/
│   └── static files our server will serve
├── Makefile
└── README.md
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
