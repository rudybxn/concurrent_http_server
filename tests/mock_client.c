/*
 * mock_client.c
 * A simple mock HTTP client for testing the server locally.
 * Connects to localhost:8080 and sends a basic GET request,
 * printing the server's response to stdout.
 */
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server = {
        .sin_family = AF_INET,
        .sin_port = htons(8080)
    };
    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    connect(sock, (struct sockaddr *)&server, sizeof(server));
    printf("======= CLIENT STARTED. =======\n");
    printf("Sending request...\n");

    char *request = "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n";
    send(sock, request, strlen(request), 0);
    printf("Request sent.\n");

    char response[1024] = {0};
    recv(sock, response, sizeof(response), 0);
    printf("--- Received Response ---\n");
    printf("%s\n", response);

    close(sock);
    return 0;
}