/* =============================================================
 * main.c
 * Entry point: The TCP socket is created and binds to port 8080.
 * It listens for incoming connections and runs an accept loop that blocks until a client connects.
 * Once a client connects, the server calls http_handle() to read the request and send a response.
 * ============================================================= */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>  /* Linux std library used for: sockaddr_in, htons, INADDR_ANY */

#include "http.h"
#include "thread_pool.h"  /* stubbed: not yet used */

#define PORT 8080 /* Port to listen on */
#define BACKLOG 10 /* max number of pending connections the OS will queue before we call accept(). 
                        Connections beyond this are refused by the OS automatically. */

int main(void) {
    /* server_fd: the listening socket. Stays open for the life of the server.
       client_fd: a new socket the OS creates for each incoming connection.
       Every accepted connection gets its own fd so we can read/write it
       independently of the listening socket. Currently, the server only handles one connection at a time.*/
    int server_fd;
    int client_fd;

    /* sockaddr_in holds an IPv4 address + port (SocketAddress_Internet).
    Used in two ways:
         1. to specify what address (port) to bind our server to
         2. accept() fills it in with the connecting client's address */
    struct sockaddr_in addr;

    /* accept() needs to know the size of our addr struct upfront,
       and will update this value with the actual bytes written.
       socklen_t is the designated type for socket address sizes. */
    socklen_t addr_len = sizeof(addr);

    /* Step 1: Create TCP socket 
        socket() asks the OS to create a new socket and returns a file descriptor for it.
         AF_INET    = IPv4 (as opposed to AF_INET6 for IPv6)
         SOCK_STREAM = TCP (as opposed to SOCK_DGRAM for UDP)
         0          = let the OS pick the right protocol automatically */
    */
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Step 2: Set socket options
       By default, if the server crashes or is restarted, the OS keeps
       the port in a TIME_WAIT state for ~60 seconds and refuses to let
       anything bind to it again. SO_REUSEADDR disables that behavior so
       we can restart immediately without "address already in use" errors.
       opt=1 means "enable this option". */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* Step 3: Bind to an address
       Binding attaches our socket to a specific IP address and port.
       Zero out the struct first to avoid garbage values in unused fields.
         sin_family      = AF_INET (must match what we passed to socket())
         sin_addr.s_addr = INADDR_ANY means "accept connections on any of
                           this machine's network interfaces" rather than
                           locking to one specific IP address
         sin_port        = htons() converts our port number from host byte
                           order to network byte order (big-endian). network
                           protocols require big-endian; x86 machines are
                           little-endian. htons = "host to network short". */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(PORT);

    /* cast to (struct sockaddr *) because bind() accepts a generic socket
       address pointer — it predates void* being used for this purpose */
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    /* Step 4: Listen
       listen() marks the socket as passive — it will now accept incoming
       connections rather than initiate them. BACKLOG tells the OS how many
       connections to queue up while we're busy handling another one.
       at this point the socket is open and the OS is accepting TCP handshakes,
       but we haven't started reading from any of them yet. */
    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Listening on port %d...\n", PORT);

     /* ---- Step 5: Accept loop ---------------------------------------
       accept() blocks here — the program pauses and waits until a client
       connects. when one does, the OS completes the TCP handshake, creates
       a new socket for that specific connection, and returns its fd.
       server_fd stays open and keeps listening for the next connection.
       this is single-threaded: we fully handle one connection before
       calling accept() again, so any new connections wait in the backlog
       queue until we loop back around. */
    while (1) {
        client_fd = accept(server_fd, (struct sockaddr *)&addr, &addr_len);
        if (client_fd < 0) {
            perror("accept");
            continue; /* skip this iteration and try accepting again */
        }

        /* pass the client socket to the HTTP layer.
           http_handle() will read the request, write the response,
           and close client_fd before returning. */
        http_handle(client_fd);
    }

    close(server_fd);
    return 0;
}