/* =============================================================
 * http.c
 * Reads the incoming HTTP request and responds. 
 * NOTE: Current implementation is hardcoded and does not parse the request.
 * ============================================================= */

#include "http.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define BUF_SIZE 4096

#define HARDCODED_TXT "Hello, World!\n" // HACK: Implement dynamic responses later and remove this hardcoded text.

static const char *RESPONSE =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/plain\r\n"
    "Content-Length: 14\r\n"
    "Connection: close\r\n"
    "\r\n"
    HARDCODED_TXT;

void http_handle(int client_fd) {
    char buf[BUF_SIZE];

    /* Read the request (we don't parse it yet — just drain it) */
    ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
    if (n > 0) {
        buf[n] = '\0';
        /* Optional: print the raw request for debugging */
        printf("--- Request ---\n%s\n", buf);
    }

    /* Send hardcoded response */
    write(client_fd, RESPONSE, strlen(RESPONSE));

    close(client_fd);
}
