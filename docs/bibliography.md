**Annotated Bibliography** 

---

**Research Extension Update**

After reviewing all 12 sources in this annotated bibliography, we have decided to proceed with our original research proposal without major modifications. The literature  validates our approach: Harchol-Balter et al. [4] provides direct theoretical support for our hypothesis that SFF will reduce mean response time by 2-4× under heavy-tailed workloads, while Crovella & Bestavros [12] confirms the heavy-tailed file size distributions our experimental workload must replicate. Lampson [1] reinforces our architectural decisions around separation of concerns and fixed-size thread pools. Anderson et al. [2] quantifies kernel threading overhead, confirming our O(n) buffer scan adds negligible cost. The Bendersky tutorial [5] and Stack Overflow discussion [7] informed our implementation approach using pthreads and justified our thread pool architecture over event-driven models. Our exploration of Apache [8], Nginx [9], and production insights from Netflix [6] confirmed that FCFS is the default in production servers and that scheduling policies matter most at moderate-to-high utilization. Shenango [3] validates that scheduling overhead is negligible compared to context switch costs. Together, these sources confirm our research question is well-founded.

**Concurrent Web Server: Thread Pool Architecture with Size-Based Scheduling**

### [1] Lampson, B. W., "**Hints for Computer System Design**," ACM SIGOPS Operating Systems Review 17(5), 33–48, 1983.  

[Link](https://dl.acm.org/doi/10.1145/773379.806614)

Category: Academic Paper (Classic/Foundational)  

Summary: Lampson distills decades of systems-building experience at Xerox PARC into design heuristics organized around functionality, speed, and fault tolerance. The paper covers interface design, implementation tradeoffs, and performance principles, emphasizing that simplicity and predictability should guide decisions over theoretical elegance.  

Relevance: "Do one thing at a time, and do it well" justifies our clean separation between the main thread (connection acceptance) and worker threads (request processing). "Split resources in a fixed way if in doubt" supports our fixed-size thread pool over dynamic thread creation, and "handle normal and worst case separately" maps directly to our SFF rationale — the normal case (small files dominating web traffic) should be fast, while the worst case (rare large files) must still make progress.

### [2] Anderson, T. E., Bershad, B. N., Lazowska, E. D., & Levy, H. M., "**Scheduler Activations: Effective Kernel Support for the User-Level Management of Parallelism**," ACM Transactions on Computer Systems (TOCS) 10(1), 53–79, 1992.  

[Link](https://dl.acm.org/doi/10.1145/146941.146944) 

Category: Academic Paper (Classic/Foundational)  

Summary: This paper identifies the fundamental threading dilemma: kernel threads integrate correctly with OS services but cost ~100μs per operation, while user-level threads are ~10× faster but stall the entire process on any blocking kernel call. The proposed "scheduler activations" mechanism lets the kernel notify user-level thread packages of events via upcalls, achieving user-level performance with kernel-level correctness.  

Relevance: Our web server uses POSIX pthreads (kernel-level, 1:1 mapped threads), so every pthread_mutex_lock, pthread_cond_wait, and context switch in our bounded buffer crosses the user-kernel boundary at the costs Anderson quantified. This paper explains why our SFF buffer scan adds negligible overhead relative to the thread management costs already present, and contextualizes why production systems like Shenango moved beyond the kernel-thread model we study.

### [3] Ousterhout, A., Fried, J., Behrens, J., Belay, A., & Balakrishnan, H., "**Shenango: Achieving High CPU Efficiency for Latency-sensitive Datacenter Workloads**," Proceedings of the 16th USENIX NSDI, 361–378, 2019.   

[Link](https://www.usenix.org/conference/nsdi19/presentation/ousterhout)   

Category: Academic Paper (Contemporary)  

Summary: Shenango resolves the tension between low tail latency and CPU efficiency by introducing a dedicated IOKernel that reallocates cores across applications at 5μs granularity. The system provides user-level green threads with work-stealing load balancing, matching dedicated-core latency while improving CPU efficiency by over 6× compared to prior kernel-bypass approaches.  

Relevance: Shenango's architecture directly validates our producer-consumer pattern at scale — the IOKernel distributing packets to application runtimes mirrors our main thread enqueuing requests into a bounded buffer for worker threads. Its evaluation demonstrates that scheduling decision overhead (buffer scans, queue operations) is negligible compared to context switch and cache miss costs, supporting our choice of a simple O(n) scan for SFF rather than a complex heap.

### [4] Harchol-Balter, M., Schroeder, B., Bansal, N., & Agrawal, M., "**Size-based Scheduling to Improve Web Performance**," ACM Transactions on Computer Systems (TOCS) 21(2), 207–233, 2003.   

[Link](https://dl.acm.org/doi/10.1145/762483.762486)  

Category: Academic Paper (Classic/Foundational)  

Summary: This paper provides theoretical analysis and trace-driven simulation proving that size-based scheduling (SRPT/SFF) dramatically outperforms FCFS in web servers due to the heavy-tailed nature of web file size distributions. The authors demonstrate that prioritizing small requests does not starve large ones, since large requests are infrequent enough that their additional wait is negligible.  

Relevance: This is the theoretical foundation for our entire research extension. Our primary hypothesis (SFF reduces mean response time by 30–50% over FCFS), our secondary hypothesis (large requests won't starve), and our experimental methodology (heavy-tailed workloads measured under increasing load) all derive directly from this paper. Even our implementation — calling stat() at enqueue time and scanning the buffer for the smallest file — is the practical realization of the SFF policy analyzed here.

### [5] Eli Bendersky, "C**oncurrent Servers: Part 2 - Threads**," Eli Bendersky's Website, 2017.  

[Link](https://eli.thegreenplace.net/2017/concurrent-servers-part-2-threads/)  

Category: Engineering Blog & Industry Deep-Dive  

Summary: This post provides a walkthrough of implementing multi-threaded servers in C using the pthreads library, covering the transition from simple sequential servers to more robust architectures that manage shared resources across multiple execution threads.  

Relevance: This source serves as an implementation blueprint for our project's core requirement of a fixed-size worker thread pool. It provides the practical "how-to" for the producer-consumer pattern we use to enqueue requests into a shared buffer, ensuring our skeleton code correctly uses mutexes and condition variables to safely manage the buffer while performing the O(n) scan required for our SFF scheduling policy.

### [6] Netflix Technology Blog, "**Performance Under Load**," Netflix Tech Blog, 2018.   

[Link](https://netflixtechblog.medium.com/performance-under-load-3e6fa9a60581)  

Category: Engineering Blog & Industry Deep-Dive  

Summary: This deep-dive explores how Netflix manages latency and concurrency limits to prevent system collapse during traffic spikes, discussing the critical relationship between system utilization, throughput, and the explosion of response times when a system reaches saturation.  

Relevance: This source directly validates our hypothesis that scheduling policies are most impactful under moderate-to-high system loads (~0.7 utilization). Netflix's discussion on maintaining stability under overload mirrors our tertiary hypothesis that SFF will preserve system performance while FCFS experiences response time explosion, and their focus on fairness while optimizing performance informs our use of the Jain Fairness Index.

### [7] Various Contributors, "**Why is epoll faster than select?**," Stack Overflow, 2013.   

[Link](https://stackoverflow.com/questions/17355593/why-is-epoll-faster-than-select)  

Category: Community Discourse  

Summary: A Stack Overflow thread where practitioners explain the fundamental differences between blocking I/O multiplexing models (select, poll, epoll), detailing the O(n) cost of scanning file descriptors under select/poll versus the event-notification model of epoll.  

Relevance: This thread contextualizes our architectural decision to use a thread pool over an event-driven model. While epoll-based servers (such as NGINX) avoid per-connection thread overhead, they do not expose a reorderable request queue — a prerequisite for implementing SFF scheduling — which helped us articulate why our thread pool architecture is the right tool for evaluating scheduling policy impact.

### [8] Apache Software Foundation, "**Apache HTTP Server**," Apache HTTP Server Project, 2025.   

[Link](https://github.com/apache/httpd/blob/trunk/server/mpm/worker/worker.c)   

Category: Source Code  

Summary: Apache HTTP Server is open-source and the most widely-used web server on the Internet. Its worker MPM uses one acceptor thread and multiple worker threads with a default fixed-size thread limit of 16 and a max of 20,000, managed via FIFO queues and event-driven request handling.  

Relevance: Apache provides a real-world reference for comparing our implemented scheduling policies. Its FIFO-based handling and fixed-size worker thread pool serve as a useful baseline against which we can measure the performance impact of our SFF implementation and explore extensions beyond standard FIFO scheduling.

### [9] Sysoev, I. and NGINX, Inc., "**NGINX (Version 1.28.2)**," nginx.org, 2026.   

[Link](https://github.com/nginx/nginx/blob/master/src/core/ngx_thread_pool.c)   

Category: Source Code  

Summary: NGINX is a popular open-source web server that operates with a master process overseeing worker processes and a fixed-size worker thread pool. Its core directory implements the thread pool as a FIFO linked-list queue, with ngx_thread_task_post scheduling new tasks and ngx_thread_pool_handler handling task completion.  

Relevance: NGINX's architecture serves as a reference for understanding the role of concurrency in handling blocking requests from the main event loop. Our research compares this standard FCFS implementation against our SFF policy to evaluate how task-size awareness impacts latency, throughput, and overall performance in a concurrent web server.

### [10] Kurose, J. & Ross, K., "**TCP – Transmission Control Protocol**," Computer Networking: A Top-Down Approach / Systems Approach, 2023.   

[Link](https://book.systemsapproach.org/e2e/tcp.html)   

Category: Book (Textbook/Reference)  

Summary: TCP is a reliable, congestion-controlled transport protocol that utilizes a three-way handshake to establish and terminate connections between clients and servers, employing acknowledgements to prevent stale connections from being reused.  

Relevance: Our HTTP server and clients communicate over TCP, meaning both sides must complete the three-way handshake before any request is enqueued into our thread pool. Understanding TCP's connection lifecycle is foundational to correctly implementing the socket accept loop in our main thread.

### [11] Love, R., "**Mutex**,” Linux Kernel Development, 3rd ed., Addison-Wesley, 2010.   

[Link](https://www.cse.iitd.ac.in/~rijurekha/col788_2023/linux_kernel_development.pdf)  

Category: Book (Textbook/Reference)  

Summary: A mutex (mutual exclusion) is a synchronization mechanism that prevents race conditions on shared resources by granting exclusive access to one task at a time, blocking all other tasks that wish to manipulate the same resource.  

Relevance: Our HTTP server runs multiple worker threads that dequeue work from a single shared bounded buffer. Mutexes are employed to prevent race conditions during both enqueue (main thread) and dequeue (worker threads) operations, making this a core synchronization primitive in our implementation.

### [12]Crovella, M. E., & Bestavros, A., "**Self-Similarity in World Wide Web Traffic: Evidence and Possible Causes**," IEEE/ACM Transactions on Networking, 5(6), 835–846, 1997.  

[Link](https://asvk.cs.msu.ru/wp-content/uploads/2023/04/crovella1997_Self-SimilarityInWorldWideWebTraffic.pdf)  

Category: Academic Paper (Classic/Foundational)  

Summary: This paper demonstrates that World Wide Web traffic exhibits self-similar, heavy-tailed behavior; meaning a small number of very large transfers account for a disproportionate share of total bytes, while the vast majority of requests are small. The authors validate this through empirical trace analysis and identify file size distributions as a root cause.  

Relevance: This paper provides the empirical backbone for our entire experimental workload design. The heavy-tailed file size distribution in testing is grounded in exactly the traffic patterns Crovella & Bestavros documented. Without this heavy-tailed reality, SFF's advantage over FCFS would be negligible; it is precisely because most requests are small and rare requests are large that prioritizing by size yields the 30–50% mean response time reduction we hypothesize.
