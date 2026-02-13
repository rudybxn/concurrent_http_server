# Glossary of Technical Terms
- **thread**
A thread is the abstraction for a single execution context within a running process.

- **thread pool**
A thread pool is a fixed-size pool of worker threads, created upon starting up a web server. Each thread is waiting until it needs to handle an HTTP request. If there are more requests than worker threads, those requests are buffered until there is a worker thread available.

- **multi-threading vs. multiprocessing**
In multi-threading, multiple threads execute within the same process and share a single address space, allowing direct access to shared data. Within that address space, each thread has its own stack. In multiprocessing, each process has its own separate address space in which their data structures are stored.
Multi-threading enables concurrency within a single program, whereas multiprocessing allows for concurrency across multiple processes.

- **HTTP (Hypertext Transfer Protocol)**
HTTP is an application-layer protocol that web browsers and web servers use to interact. A web browser sends a HTTP request to a web server, and the web server replies with an HTTP response.

- **TCP/IP (Transmission Control Protocol/Internet Protocol)**
TCP/IP is a protocol suite that transports data across networks by routing messages to their correct destination, managing network congestion, etc. HTTP runs on top of TCP/IP as a client will establish a TCP connection to a server's IP address and specified port, and HTTP messages are then sent over the TCP connection. One TCP connection can carry more than one HTTP request.

- **concurrency**
Concurrency refers to the ability to manage multiple requests at overlapping times. The operating system is an example of a concurrent program. Multi-threaded web servers are supported by the operating system via primitives such as locks and condition variables.

- **synchronization**
Synchronization refers to the mechanisms used to coordinate concurrent threads and control their access to shared resources in a controlled and correct manner. It involves managing data access to the shared address space and utilizing locks and condition variables to synchronize thread waiting and signalling.

- **bounded-buffer (producer-consumer problem)**
The bounded-buffer problem, also known as the producer-consumer problem, is a synchronization problem. For a multi-threaded web server, a bounded buffer is used for the work queue where a producer thread will put HTTP requests into and a consumer thread will retrieve a request from to process. It is a shared resource that requires synchronized access and its solution involves the use of condition variables and locks.

- **sockets**
A socket is a software abstraction representing one endpoint of a network connection, used for communication. It includes a port and also contains information about the protocol used, IP addresses, and ports used. 

- **ports**
A port is identified by a number and represents a service endpoint on a machine. A web server uses a port to listen for incoming network connections. Web servers usually run on port 80.

- **locks**
A lock is a variable that allows critical sections of code to be executed atomically. The state of the lock may be either available (also called unlocked or free) or acquired (also called locked or held). The 'mutex' lock from the POSIX library is an example of a type of lock that provides mutual exclusion between threads.

- **condition variables**
A condition variable is a synchronization primitive that allows threads to wait for a condition to become true. When another thread changes the condition, it can signal to the waiting threads to wake and continue their execution. It is used in conjunction with locks, using the wait() operation to release a lock and the signal() operation to wake a waiting thread.

- **scheduling policies**
Scheduling policies for a multi-threaded web server determine which HTTP request will be selected for handling by each of the waiting worker threads.

- **scheduling policy: First-Come-First-Served (FCFS)**
The FCFS scheduling policy selects requests in the order of arrival, implementing a First-In-First-Out (FIFO) approach to request handling. It does not account for request size or execution time.

- **scheduling policy: Smallest-File-Served (SFF)**
Under the SFF scheduling policy, the system selects the pending request associated with the smallest file size for service. SFF is strongly related to the Shortest-Job-First (SJF) scheduling policy, with file size used as a proxy to approximate job length.

- **fairness**
Fairness concerns the degree to which requests receive comparable service, regardless of size or arrival time.

- **throughput**
Throughput refers to the rate at which the system completes requests, typically measured as requests per unit time.

- **service time**
The service time of a request is the amount of time required to process it once it begins execution.

- **mean response time**
The mean response time of a request is the average time from its arrival, to when it is fully served.

- **tail latency**
Tail latency refers to the response time experienced by the slowest fraction of requests (e.g., 95th or 99th percentile). In the context of certain scheduling policies such as SFF, while metrics such as mean response times are improved, tail latency for large files is often worsened.

- **head-of-line blocking**
Head-of-line blocking is a condition where a large or slow request at the front of a queue delays all subsequent requests.

- **starvation**
Starvation refers to the situation where certain requests  may experience unbounded waiting time, thus being 'starved'. An example would be large-size requests under a SFF policy.

- **work-conserving scheduler**
A work-conserving scheduler is a type of scheduler that never leaves worker threads idle when there are pending requests.
