/**
 * PHASE 2 - Bài 03: UNIX Socket SOCK_STREAM
 *
 * Mục tiêu:
 *  - Server: bind, listen, accept, recv/send
 *  - Client: connect, send, recv
 *  - Chạy server + client trong cùng file bằng fork
 *  - Non-blocking mode, graceful shutdown
 *
 * Compile: g++ -std=c++17 -O2 03_unix_socket_stream.cpp -o out -lpthread
 */

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <sys/wait.h>
#include <fcntl.h>

constexpr const char* SOCK_PATH = "/tmp/robot_stream.sock";

// ─── Helpers ──────────────────────────────────────────────────────────────
int make_unix_stream_server(const char* path) {
    unlink(path);  // Remove stale socket

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(fd); return -1;
    }
    if (listen(fd, 5) < 0) {
        perror("listen"); close(fd); return -1;
    }
    return fd;
}

int make_unix_stream_client(const char* path) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return -1; }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect"); close(fd); return -1;
    }
    return fd;
}

// ─── Ví dụ 1: Cơ bản — stream server và client ────────────────────────────
struct Packet {
    uint32_t seq;
    double   value;
};

void run_server(const char* path, int n_packets) {
    int srv_fd = make_unix_stream_server(path);
    if (srv_fd < 0) return;

    printf("[Server] Listening on %s\n", path);

    // Accept one connection
    struct sockaddr_un peer{};
    socklen_t peer_len = sizeof(peer);
    int conn_fd = accept(srv_fd, (struct sockaddr*)&peer, &peer_len);
    if (conn_fd < 0) { perror("accept"); close(srv_fd); return; }
    printf("[Server] Client connected\n");

    // Receive packets
    for (int i = 0; i < n_packets; ++i) {
        Packet pkt;
        ssize_t n = recv(conn_fd, &pkt, sizeof(pkt), MSG_WAITALL);
        if (n == sizeof(pkt)) {
            printf("[Server] Received seq=%u value=%.2f\n", pkt.seq, pkt.value);
            // Echo back
            send(conn_fd, &pkt, sizeof(pkt), 0);
        } else if (n == 0) {
            printf("[Server] Client disconnected\n");
            break;
        }
    }

    close(conn_fd);
    close(srv_fd);
    unlink(path);
}

void run_client(const char* path, int n_packets) {
    // Retry connect a few times (server may not be ready)
    int fd = -1;
    for (int retry = 0; retry < 5 && fd < 0; ++retry) {
        fd = make_unix_stream_client(path);
        if (fd < 0) {
            struct timespec sleep{ .tv_sec = 0, .tv_nsec = 50'000'000 };
            nanosleep(&sleep, nullptr);
        }
    }
    if (fd < 0) { printf("[Client] Failed to connect\n"); return; }

    printf("[Client] Connected\n");

    for (int i = 0; i < n_packets; ++i) {
        Packet pkt{ .seq = (uint32_t)i, .value = i * 1.1 };
        send(fd, &pkt, sizeof(pkt), 0);

        Packet echo;
        recv(fd, &echo, sizeof(echo), MSG_WAITALL);
        printf("[Client] Echo: seq=%u value=%.2f\n", echo.seq, echo.value);

        struct timespec sleep{ .tv_sec = 0, .tv_nsec = 10'000'000 };
        nanosleep(&sleep, nullptr);
    }

    close(fd);
}

void example_basic_stream() {
    std::cout << "\n=== Ví dụ 1: SOCK_STREAM — server + client ===\n";

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        // Child: Server
        run_server(SOCK_PATH, 3);
        _exit(0);
    } else {
        // Parent: Client (wait a bit for server to start)
        struct timespec sleep{ .tv_sec = 0, .tv_nsec = 100'000'000 };
        nanosleep(&sleep, nullptr);
        run_client(SOCK_PATH, 3);
        wait(nullptr);
    }
}

// ─── Ví dụ 2: Set non-blocking ────────────────────────────────────────────
void example_nonblocking() {
    std::cout << "\n=== Ví dụ 2: Non-blocking socket ===\n";

    int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (fd < 0) { perror("socket"); return; }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/nonexistent.sock", sizeof(addr.sun_path) - 1);

    // connect() sẽ fail ngay với ENOENT (không có server) hoặc EINPROGRESS
    int ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        printf("connect() returned -1, errno=%d (%s) — expected for non-blocking\n",
               errno, strerror(errno));
    }

    // Thay vào đó dùng SOCK_NONBLOCK khi accept để không block nếu không có client:
    printf("Non-blocking: EAGAIN khi không có data thay vì block\n");
    close(fd);
}

// ─── Ví dụ 3: Throughput test ─────────────────────────────────────────────
void example_throughput() {
    std::cout << "\n=== Ví dụ 3: Throughput — gửi 1000 packets ===\n";

    constexpr const char* PATH = "/tmp/robot_throughput.sock";
    const int N = 1000;

    struct timespec t_start, t_end;

    pid_t pid = fork();
    if (pid == 0) {
        // Server side
        int srv = make_unix_stream_server(PATH);
        int conn = accept(srv, nullptr, nullptr);

        uint8_t buf[64];
        for (int i = 0; i < N; ++i)
            recv(conn, buf, sizeof(buf), MSG_WAITALL);

        close(conn);
        close(srv);
        unlink(PATH);
        _exit(0);
    }

    // Client side
    struct timespec sleep{ .tv_sec = 0, .tv_nsec = 100'000'000 };
    nanosleep(&sleep, nullptr);

    int fd = make_unix_stream_client(PATH);

    uint8_t buf[64]{};
    clock_gettime(CLOCK_MONOTONIC, &t_start);
    for (int i = 0; i < N; ++i)
        send(fd, buf, sizeof(buf), 0);
    clock_gettime(CLOCK_MONOTONIC, &t_end);

    long ms = ((long)(t_end.tv_sec - t_start.tv_sec)) * 1000
            + (t_end.tv_nsec - t_start.tv_nsec) / 1'000'000;
    printf("Sent %d x %zu bytes in %ld ms = %.0f MB/s\n",
           N, sizeof(buf), ms,
           (N * (double)sizeof(buf)) / (ms * 1000.0));

    close(fd);
    wait(nullptr);
}

int main() {
    std::cout << "=== Phase2 Bài 03: UNIX Socket SOCK_STREAM ===\n";

    example_basic_stream();
    example_nonblocking();
    example_throughput();

    return 0;
}
