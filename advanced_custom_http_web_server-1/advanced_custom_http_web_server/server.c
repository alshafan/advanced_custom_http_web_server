/*
 * Advanced - Custom HTTP Web Server
 * A small HTTP/1.1 static-file server written in C.
 * Build (POSIX):   gcc -Wall -Wextra -O2 server.c -o server
 * Build (Windows): gcc -Wall -Wextra -O2 server.c -o server.exe -lws2_32
 * Run:             ./server [port] [web-root]
 */
#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#define strcasecmp _stricmp
typedef SOCKET socket_t;
typedef int socket_length_t;
#define SOCKET_INVALID INVALID_SOCKET
#else
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
typedef int socket_t;
typedef socklen_t socket_length_t;
#define SOCKET_INVALID (-1)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <sys/stat.h>

#define DEFAULT_PORT 8080
#define DEFAULT_ROOT "public"
#define BUFFER_SIZE 8192
#define MAX_REQUEST_PATH 1024

#ifdef _WIN32
static void close_socket(socket_t socket_fd) {
    closesocket(socket_fd);
}

static int socket_was_interrupted(void) {
    return WSAGetLastError() == WSAEINTR;
}

static void report_socket_error(const char *operation) {
    fprintf(stderr, "%s failed (Winsock error %d).\n", operation, WSAGetLastError());
}
#else
static void close_socket(socket_t socket_fd) {
    close(socket_fd);
}

static int socket_was_interrupted(void) {
    return errno == EINTR;
}

static void report_socket_error(const char *operation) {
    perror(operation);
}
#endif

static const char *mime_type(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (!strcasecmp(ext, ".html")) return "text/html; charset=utf-8";
    if (!strcasecmp(ext, ".css"))  return "text/css; charset=utf-8";
    if (!strcasecmp(ext, ".js"))   return "application/javascript; charset=utf-8";
    if (!strcasecmp(ext, ".json")) return "application/json";
    if (!strcasecmp(ext, ".png"))  return "image/png";
    if (!strcasecmp(ext, ".jpg") || !strcasecmp(ext, ".jpeg")) return "image/jpeg";
    if (!strcasecmp(ext, ".svg"))  return "image/svg+xml";
    if (!strcasecmp(ext, ".ico"))  return "image/x-icon";
    if (!strcasecmp(ext, ".txt"))  return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

static int send_all(socket_t fd, const void *data, size_t len) {
    const char *p = data;
    while (len) {
#ifdef _WIN32
        int chunk_len = len > INT_MAX ? INT_MAX : (int)len;
        int n = send(fd, p, chunk_len, 0);
#else
        ssize_t n = send(fd, p, len, 0);
#endif
        if (n < 0) { if (socket_was_interrupted()) continue; return -1; }
        p += n; len -= (size_t)n;
    }
    return 0;
}

static void send_error(socket_t client, int code, const char *reason, const char *message) {
    char body[512], header[512];
    int blen = snprintf(body, sizeof body,
        "<!doctype html><html><head><title>%d %s</title></head>"
        "<body style='font-family:Arial;padding:40px;background:#0b1220;color:#e8eefc'>"
        "<h1>%d — %s</h1><p>%s</p><a href='/' style='color:#70e1d1'>Return home</a>"
        "</body></html>", code, reason, code, reason, message);
    int hlen = snprintf(header, sizeof header,
        "HTTP/1.1 %d %s\r\nContent-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %d\r\nConnection: close\r\nX-Content-Type-Options: nosniff\r\n\r\n",
        code, reason, blen);
    send_all(client, header, (size_t)hlen);
    send_all(client, body, (size_t)blen);
}

static void handle_client(socket_t client, const char *root) {
    char request[BUFFER_SIZE];
#ifdef _WIN32
    int received = recv(client, request, (int)sizeof request - 1, 0);
#else
    ssize_t received = recv(client, request, sizeof request - 1, 0);
#endif
    if (received <= 0) return;
    request[received] = '\0';

    char method[16] = {0}, target[MAX_REQUEST_PATH] = {0}, version[16] = {0};
    if (sscanf(request, "%15s %1023s %15s", method, target, version) != 3) {
        send_error(client, 400, "Bad Request", "The request could not be understood.");
        return;
    }

    if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0) {
        send_error(client, 405, "Method Not Allowed", "This server currently supports GET and HEAD.");
        return;
    }

    /* Strip query string and reject traversal / encoded path tricks. */
    char *query = strchr(target, '?');
    if (query) *query = '\0';
    if (strstr(target, "..") || strchr(target, '\\') || strstr(target, "%")) {
        send_error(client, 403, "Forbidden", "Unsafe path rejected.");
        return;
    }
    if (strcmp(target, "/") == 0) strcpy(target, "/index.html");

    char fullpath[MAX_REQUEST_PATH * 2];
    if (snprintf(fullpath, sizeof fullpath, "%s%s", root, target) >= (int)sizeof fullpath) {
        send_error(client, 414, "URI Too Long", "The requested path is too long.");
        return;
    }

    struct stat st;
    if (stat(fullpath, &st) < 0 || !S_ISREG(st.st_mode)) {
        send_error(client, 404, "Not Found", "The requested resource does not exist.");
        return;
    }

    FILE *fp = fopen(fullpath, "rb");
    if (!fp) {
        send_error(client, 403, "Forbidden", "The server cannot read this resource.");
        return;
    }

    char header[1024];
    int hlen = snprintf(header, sizeof header,
        "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %lld\r\n"
        "Connection: close\r\nCache-Control: no-cache\r\n"
        "X-Content-Type-Options: nosniff\r\n\r\n",
        mime_type(fullpath), (long long)st.st_size);
    send_all(client, header, (size_t)hlen);

    if (strcmp(method, "HEAD") != 0) {
        char chunk[4096];
        size_t n;
        while ((n = fread(chunk, 1, sizeof chunk, fp)) > 0) {
            if (send_all(client, chunk, n) < 0) break;
        }
    }
    fclose(fp);
}

int main(int argc, char **argv) {
    int port = DEFAULT_PORT;
    const char *root = DEFAULT_ROOT;
    if (argc > 1) port = atoi(argv[1]);
    if (argc > 2) root = argv[2];
    if (port < 1 || port > 65535) {
        fprintf(stderr, "Invalid port. Use a value from 1 to 65535.\\n");
        return 1;
    }

#ifdef _WIN32
    WSADATA winsock_data;
    int startup_result = WSAStartup(MAKEWORD(2, 2), &winsock_data);
    if (startup_result != 0) {
        fprintf(stderr, "WSAStartup failed (Winsock error %d).\n", startup_result);
        return 1;
    }
#else
    signal(SIGPIPE, SIG_IGN);
#endif

    socket_t server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == SOCKET_INVALID) {
        report_socket_error("socket");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    int yes = 1;
#ifdef _WIN32
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof yes);
#else
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
#endif
    struct sockaddr_in address;
    memset(&address, 0, sizeof address);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons((unsigned short)port);

    if (bind(server, (struct sockaddr *)&address, sizeof address) < 0) {
        report_socket_error("bind");
        close_socket(server);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    if (listen(server, 16) < 0) {
        report_socket_error("listen");
        close_socket(server);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    printf("Custom HTTP server running at http://localhost:%d\n", port);
    printf("Serving files from: %s\nPress Ctrl+C to stop.\n", root);

    for (;;) {
        struct sockaddr_in peer;
        socket_length_t peer_len = sizeof peer;
        socket_t client = accept(server, (struct sockaddr *)&peer, &peer_len);
        if (client == SOCKET_INVALID) {
            if (socket_was_interrupted()) continue;
            report_socket_error("accept");
            continue;
        }
        printf("%s:%d connected\n", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
        handle_client(client, root);
        close_socket(client);
    }
    close_socket(server);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
