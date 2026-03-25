/* =============================================================
 * http.h
 *
 * HTTP request handling. Given an accepted socket descriptor,
 * reads the request, parses method and path, serves the file
 * from disk (or returns an error), and closes the connection.
 *
 * Thread-safety: http_handle() uses only stack-local variables,
 * so multiple workers can call it concurrently without locks.
 * ============================================================= */

#ifndef HTTP_H
#define HTTP_H

/* Root directory for static files. Requests are resolved
   relative to this path. Can be overridden before calling
   http_handle() if needed. */
#define WEB_ROOT "./www"

void http_handle(int client_fd);

#endif