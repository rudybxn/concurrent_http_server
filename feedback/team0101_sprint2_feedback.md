# Sprint 2 Feedback — Team 0101

**Language:** C

---

## Feedback

**Bibliography:**

- Source count: 12/12, all 5 categories present
- Source code entries are done well: Apache `worker.c` [8] and Nginx `ngx_thread_pool.c` [9] both link to specific files, explain the FIFO dequeue mechanism, and draw concrete comparisons relevant to the research extension
- Research Extension Update is well-argued: cites specific papers by number, connects Lampson's "handle normal and worst case separately" to SFF rationale
- Improvement: Source [10] (Kurose & Ross) is cited as "TCP – Transmission Control Protocol" rather than the book title and chapter number — fix to include full book title, edition, and chapter referenced
- Improvement: Source [11] (Love, Linux Kernel Development) links to a full PDF rather than a specific chapter — add chapter number to the header line
- Improvement: Entries [8]–[12] do not use the `### [#]` heading structure required by the rubric; apply consistent formatting throughout

**Design Refinement:**

- Glossary: 22/20 terms, exceeding the minimum; new terms (Head-of-line blocking, Starvation, Work-conserving scheduler, Tail latency, Service time, Mean response time) are directly drawn from the Harchol-Balter paper
- 3 UML diagrams present in `docs/diagrams/`
- Improvement: No statement in the docs confirming whether diagrams changed from Sprint 1 or why they remain valid, diagram PDFs exist but are not accompanied by documentation — add a brief paragraph to satisfy this rubric requirement
- Improvement: Research proposal is stored as a PDF (`updated_cs5800_research_question.pdf`) — a Markdown/Latex version would be more version-control-friendly

**Skeleton Code:**

- Language (C) clearly stated in `README.md`; `make` builds cleanly with separate targets for server, mock client, and test runner
- Meets the Sprint 2 minimum: server listens, accepts, and sends a hardcoded HTTP response
- `main.c` is well-commented — explains each step of socket setup (`socklen_t`, `htons`, `INADDR_ANY`, `SO_REUSEADDR`) at a level demonstrating API understanding
- `thread_pool.c` is correctly stubbed with right function signatures; `thread_pool.h` provides a clean opaque interface
- Improvement: `main.c` ignores `argc`/`argv` entirely while README and Makefile show `./server 8080 4 FCFS` — either implement the CLI argument handling or correct the README
- Improvement: `http_handle` reads the raw request and discards it with no check for `n == 0` (connection closed by client) — add a guard
- Improvement: Makefile places `-lpthread` in `CFLAGS` — move to `LDFLAGS` or `LDLIBS` to avoid compiler warnings on clang
- Improvement: README has multiple empty placeholder sections (Features, Architecture, Testing, directory tree) — fill these in in your next sprint

**AI Reflections:**

- Helena's entry is specific and honest: names the tool, describes anchoring prompts in Apache/Nginx source and OSTEP notes, and recommends peer review of generated code
- Ishit's entry is honest about scope (bibliography formatting only)
- Improvement: Tomas and Rudrajit both submitted entries stating "I didn't use AI" — the rubric requires 100–200 words covering which tools were used, where AI helped, and at least one case where AI was unhelpful or incorrect; even a genuine non-user must explain how they accomplished tasks without AI and reflect on that choice
- Improvement: Helena's entry does not name a specific case where AI produced incorrect output

**Administration:**

- Files at correct paths, contribution tracker linked
- Release tag present

---

## Summary

**Strengths:**

- Bibliography is well-argued with directly relevant source code entries and a focused Research Extension Update
- Glossary adds appropriate new terms drawn demonstrably from the Harchol-Balter paper
- Skeleton compiles cleanly and is well-commented, demonstrating clear understanding of the POSIX networking API

**Areas to Improve:**

- Tomas and Rudrajit's AI reflection entries need to be rewritten to meet the 100–200 word rubric requirement
- Add a UML diagram status note and a prose architecture document to satisfy the design refinement section
- Fix the `argc`/`argv` disconnect between `main.c` and the README run instructions
