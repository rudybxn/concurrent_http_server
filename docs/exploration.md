# Exploration Report

In this report, we are exploring 3 well known concurrent web servers. For each server, we will see it running, check its configurations if present and run the server in baseline and heavy loads.

## 1. Apache HTTP Server 
Apache HTTP Server is a production web server that supports multiple concurrency models via MPM( Multi Processing Modules). For this exploration, we will look at event MPM which can handle large numbers of concurrent connections. The older Worker MPM keeps a thread busy as long as a user is connected (even if they aren't doing anything). However the Event MPM uses a dedicated Listener Thread. This thread manages all incoming sockets and only hands them off to a worker thread when there is actual data to be processed.

- ThreadsPerChild - Defines vertical scaling of each process
- MaxRequestWorkers - hard ceiling for concurrency


### Analysis

- **Low Load Behavior**: Under low load(1,000 total requests with 10 concurrent clients), Apache completed requests quickly with minimal latency. Worker threads were lightly utilized, and the server remained responsive throughout the test
- **High Load Behavior**: Under high concurrency (100,000 total requests with 500 concurrent clients), Apache took significantly longer to complete all requests. Monitoring the system using htop showed many active Apache threads (/usr/sbin/apache2 -k start), indicating heavy utilization of the worker thread pool. As concurrency increased, requests experienced delays due to worker saturation, even though idle connections were efficiently managed by the event-driven component of the MPM. As the linux kernel swaps between 500 + threads it incurs context switching cost


### Observations and Insights
These results demonstrate that while the event MPM improves scalability by avoiding blocking threads on idle connections, it is still constrained by the size of the worker thread pool when processing active requests. Under heavy load, request latency increases as threads can become saturated and scalability is limited by the OS kernel’s ability to schedule a large number of heavy threads.


## 2. Nginx
Nginx is a web server designed around an event driven architecture. Unlike traditional thread-pool-based servers, Nginx uses a small number of worker processes to manage a large number of concurrent connections efficiently.Nginx uses a non blocking event loop architecture. Requests are handled using non-blocking I/O, allowing a single worker to manage thousands of simultaneous connections without dedicating a thread per request. As a result, Nginx avoids the need for a centralized request queue or a large thread pool.

## Analysis

- **Low Load Behavior**: Under low concurrency (1,000 total requests with 10 concurrent clients) Nginx completed requests quickly with minimal CPU usage. The server remained responsive and system monitoring showed only a small number of active worker processes
- **High Load Behavior**: Under high concurrency(100k total requests with 500 concurrent clients) Nginx maintained steady throughput while using few worker processes. Monitoring with htop showed that concurrency was handled primarily through multiplexing rather than an increase in threads or processes. Compared to Apache, Nginx exhibited less visible queueing, less threads and better performance under load. 


### Observation and Insights
Compared to Apache’s event MPM which uses a worker thread pool, Nginx’s showed fewer active worker processes. While the OS only sees one process, NGINX internally tracks hundreds of file descriptors. It seems to utilize CPU with better efficiency.

## 3. Caddy
Caddy is a webserver written in Go that uses goroutines to handle concurrency and request processing. Caddy schedules each incoming request as a goroutine. Go’s runtime scheduler manages this and multiplexes them onto a small number of OS threads. Go uses an M:N Scheduler, which multiplexes thousands of Goroutines (M) onto a small number of real OS threads (N).This allows efficient concurrent request handling without spawning a large number of threads or relying on event loop.

### Analysis

- **Low Load Behavior**: Under low concurrency (1,000 requests with 10 concurrent clients), Caddy responded quickly with minimal CPU usage. System monitoring showed a small number of OS threads, indicating that lightweight goroutines handled request scheduling efficiently
- **High Load Behavior**: Under high concurrency (100,000 requests with 500 concurrent clients), the server continued to process requests smoothly. The number of OS threads remained lower in comparison to Apache. Load average was similar to Nginx


### Observation and Insights
Compared to Apache’s thread pool model and Nginx’s event loop, Caddy provides a hybrid approach with the performance of an event loop with the programming simplicity of threads. 

#### ( Please note: Screenshots for Apache, Nginx and Caddy are provided in the exploration_screenshots folder. The screenshots cover benchmarking of high and low loads. There are also screenshots of htop while handling large loads for Apache, Nginx and Caddy showing the difference in average load, number of processes spawned and CPU usage.) 