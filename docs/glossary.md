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
Scheduling policies for a multi-threaded web server will determine which HTTP request will be handled by each of the waiting worker threads. Two examples of scheduling policies are a First-In-First-Out (FIFO) policy where the oldest request in the buffer will be handled first, and a Smallest-File-First (SFF) policy where a worker thread will handle the request for the smallest file upon waking.