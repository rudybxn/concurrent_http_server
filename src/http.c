/* =============================================================
 * http.c
 *
 * Handles a single HTTP request on an accepted socket:
 *   1. Read and parse the request line (method + path)
 *   2. Sanitize the path (block directory traversal)
 *   3. Map path to a file on disk under WEB_ROOT
 *   4. Send appropriate response (200, 400, 404, 405)
 *
 * Thread-safety: every variable is stack-local. No globals,
 * no static mutables. Safe to call from multiple workers.
 * ============================================================= */

#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUF_SIZE   4096
#define PATH_MAX_  512

/* ---- MIME type lookup -----------------------------------------
   Maps file extension to Content-Type. Returns
   application/octet-stream for unknown extensions. */
static const char *mime_type(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";

    if (strcmp(dot, ".html") == 0) return "text/html";
    if (strcmp(dot, ".htm")  == 0) return "text/html";
    if (strcmp(dot, ".css")  == 0) return "text/css";
    if (strcmp(dot, ".js")   == 0) return "application/javascript";
    if (strcmp(dot, ".json") == 0) return "application/json";
    if (strcmp(dot, ".png")  == 0) return "image/png";
    if (strcmp(dot, ".jpg")  == 0) return "image/jpeg";
    if (strcmp(dot, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(dot, ".gif")  == 0) return "image/gif";
    if (strcmp(dot, ".ico")  == 0) return "image/x-icon";
    if (strcmp(dot, ".txt")  == 0) return "text/plain";
    if (strcmp(dot, ".svg")  == 0) return "image/svg+xml";

    return "application/octet-stream";
}

/* ---- Error response helpers -----------------------------------
   Each builds a complete HTTP response with status line,
   headers, and a short body, then sends it and closes fd. */

static void send_error(int fd, int code, const char *reason, const char *body) {
    char header[BUF_SIZE];
    int body_len = strlen(body);

    int hdr_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n",
        code, reason, body_len);

    write(fd, header, hdr_len);
    write(fd, body, body_len);
    close(fd);
}

static void send_400(int fd) {
    send_error(fd, 400, "Bad Request",
        "<html><body><h1>400 Bad Request</h1></body></html>");
}

static void send_404(int fd) {
    send_error(fd, 404, "Not Found",
        "<html><body><h1>404 Not Found</h1></body></html>");
}

static void send_405(int fd) {
    send_error(fd, 405, "Method Not Allowed",
        "<html><body><h1>405 Method Not Allowed</h1></body></html>");
}

/* ---- Path sanitization ----------------------------------------
   Returns 1 if the path is safe, 0 if it contains traversal
   attempts like ".." that could escape WEB_ROOT. */
static int path_is_safe(const char *path) {
    /* Reject any path containing ".." */
    if (strstr(path, "..") != NULL) return 0;
    /* Must start with / */
    if (path[0] != '/') return 0;
    return 1;
}

/* ---- Parse the request line -----------------------------------
   Extracts method and path from "GET /index.html HTTP/1.1\r\n..."
   Returns 0 on success, -1 on malformed request. */
static int parse_request_line(const char *buf, char *method, size_t method_sz,
                              char *path, size_t path_sz) {
    /* Find first space (after method) */
    const char *sp1 = strchr(buf, ' ');
    if (!sp1) return -1;

    /* Find second space (after path) */
    const char *sp2 = strchr(sp1 + 1, ' ');
    if (!sp2) return -1;

    /* Extract method */
    size_t mlen = sp1 - buf;
    if (mlen >= method_sz) return -1;
    memcpy(method, buf, mlen);
    method[mlen] = '\0';

    /* Extract path */
    size_t plen = sp2 - (sp1 + 1);
    if (plen >= path_sz) return -1;
    memcpy(path, sp1 + 1, plen);
    path[plen] = '\0';

    return 0;
}

/* ---- Main handler ---------------------------------------------
   Called by worker threads. Reads one HTTP request, serves the
   file or returns an error, then closes the connection. */
void http_handle(int client_fd) {
    sleep(2);
    char buf[BUF_SIZE];
    char method[16];
    char uri_path[PATH_MAX_];
    char file_path[PATH_MAX_];

    /* Step 1: Read the request.
       One read() is sufficient for typical HTTP/1.1 GET requests
       which fit in a single TCP segment. For robustness in
       production you'd loop until \r\n\r\n, but this is fine
       for our workload of static file GETs. */
    ssize_t n = read(client_fd, buf, sizeof(buf) - 1);
    if (n <= 0) {
        close(client_fd);
        return;
    }
    buf[n] = '\0';

    /* Step 2: Parse the request line */
    if (parse_request_line(buf, method, sizeof(method),
                           uri_path, sizeof(uri_path)) < 0) {
        send_400(client_fd);
        return;
    }

    /* Step 3: Only GET is supported */
    if (strcmp(method, "GET") != 0) {
        send_405(client_fd);
        return;
    }

    /* Step 4: Sanitize the path */
    if (!path_is_safe(uri_path)) {
        send_400(client_fd);
        return;
    }

    /* Map "/" to "/index.html" */
    if (strcmp(uri_path, "/") == 0) {
        strncpy(uri_path, "/index.html", sizeof(uri_path) - 1);
    }

    /* Step 5: Build the full filesystem path */
    snprintf(file_path, sizeof(file_path), "%s%s", WEB_ROOT, uri_path);

    /* Step 6: Open and stat the file */
    struct stat st;
    if (stat(file_path, &st) < 0) {
        send_404(client_fd);
        return;
    }

    FILE *fp = fopen(file_path, "rb");
    if (!fp) {
        send_404(client_fd);
        return;
    }

    /* Step 7: Send response headers */
    const char *content_type = mime_type(file_path);
    char header[BUF_SIZE];
    int hdr_len = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        content_type, (long)st.st_size);

    write(client_fd, header, hdr_len);

    /* Step 8: Send file body in chunks.
       fread + write loop is binary-safe — works for images,
       HTML, CSS, anything. Can't use fputs/fprintf for binary. */
    char file_buf[BUF_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), fp)) > 0) {
        size_t total_written = 0;
        while (total_written < bytes_read) {
            ssize_t w = write(client_fd, file_buf + total_written,
                              bytes_read - total_written);
            if (w <= 0) {
                /* Client disconnected mid-transfer */
                fclose(fp);
                close(client_fd);
                return;
            }
            total_written += w;
        }
    }

    fclose(fp);
    close(client_fd);
}