/**
 * PHASE 2 - Bài 04: UNIX Socket SOCK_DGRAM + Abstract Namespace
 *
 * Mục tiêu:
 *  - SOCK_DGRAM: message-boundary, connectionless, low-latency
 *  - Abstract namespace: không cần file, tự cleanup khi close
 *  - Đo round-trip latency: dgram vs stream
 *  - Ứng dụng: gửi sensor packets từ RT thread → non-RT logger
 *
 * Compile: g++ -std=c++17 -O2 04_unix_socket_dgram.cpp -o out
 */

#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <sys/wait.h>
#include <time.h>
#include <vector>
#include <algorithm>
#include <numeric>

// ─── Abstract namespace helper ────────────────────────────────────────────
// Abstract namespace: sun_path[0] = '\0', sau đó là tên
// Ưu điểm: không tạo file, tự xóa khi socket đóng
struct sockaddr_un make_abstract_addr(const char* name, socklen_t& len) {
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    addr.sun_path[0] = '\0';  // abstract namespace
    size_t name_len = strlen(name);
    memcpy(&addr.sun_path[1], name, name_len);
    len = offsetof(struct sockaddr_un, sun_path) + 1 + name_len;
    return addr;
}

// ─── Ví dụ 1: Cơ bản — DGRAM server + client ──────────────────────────────
struct SensorPacket {
    uint32_t seq;
    double   value;
    uint64_t timestamp_ns;
};

inline int64_t mono_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1'000'000'000LL + ts.tv_nsec;
}

void example_dgram_basic() {
    std::cout << "\n=== Ví dụ 1: SOCK_DGRAM với abstract namespace ===\n";

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        // ─── Child: Server (receiver) ──────────────────────────────────
        int fd = socket(AF_UNIX, SOCK_DGRAM, 0);

        socklen_t server_len;
        auto server_addr = make_abstract_addr("robot_dgram_01", server_len);
        if (bind(fd, (struct sockaddr*)&server_addr, server_len) < 0) {
            perror("bind"); _exit(1);
        }

        printf("[Server] Bound to abstract @robot_dgram_01\n");

        for (int i = 0; i < 3; ++i) {
            SensorPacket pkt;
            struct sockaddr_un peer{};
            socklen_t peer_len = sizeof(peer);

            ssize_t n = recvfrom(fd, &pkt, sizeof(pkt), 0,
                                  (struct sockaddr*)&peer, &peer_len);
            if (n > 0) {
                printf("[Server] Received seq=%u value=%.2f\n",
                       pkt.seq, pkt.value);
                // Echo back
                sendto(fd, &pkt, sizeof(pkt), 0,
                       (struct sockaddr*)&peer, peer_len);
            }
        }

        close(fd);
        _exit(0);

    } else {
        // ─── Parent: Client (sender) ───────────────────────────────────
        // Chờ server ready
        struct timespec sleep{ .tv_sec = 0, .tv_nsec = 50'000'000 };
        nanosleep(&sleep, nullptr);

        int fd = socket(AF_UNIX, SOCK_DGRAM, 0);

        // Client cũng cần bind để nhận echo
        socklen_t client_len;
        auto client_addr = make_abstract_addr("robot_dgram_client_01", client_len);
        bind(fd, (struct sockaddr*)&client_addr, client_len);

        socklen_t server_len;
        auto server_addr = make_abstract_addr("robot_dgram_01", server_len);

        for (int i = 0; i < 3; ++i) {
            SensorPacket pkt{ .seq = (uint32_t)i, .value = i * 2.5, .timestamp_ns = (uint64_t)mono_ns() };
            sendto(fd, &pkt, sizeof(pkt), 0,
                   (struct sockaddr*)&server_addr, server_len);

            SensorPacket echo;
            recv(fd, &echo, sizeof(echo), 0);
            printf("[Client] Echo: seq=%u value=%.2f\n", echo.seq, echo.value);

            nanosleep(&sleep, nullptr);
        }

        close(fd);
        wait(nullptr);
    }
}

// ─── Ví dụ 2: Đo round-trip latency ──────────────────────────────────────
void example_latency_measurement() {
    std::cout << "\n=== Ví dụ 2: Round-trip latency measurement ===\n";

    const int N = 1000;

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        // ─── Server: echo ngay lập tức
        int fd = socket(AF_UNIX, SOCK_DGRAM, 0);

        socklen_t slen;
        auto saddr = make_abstract_addr("robot_rtt_server", slen);
        bind(fd, (struct sockaddr*)&saddr, slen);

        uint8_t buf[64];
        for (int i = 0; i < N; ++i) {
            struct sockaddr_un peer{};
            socklen_t plen = sizeof(peer);
            ssize_t n = recvfrom(fd, buf, sizeof(buf), 0,
                                  (struct sockaddr*)&peer, &plen);
            sendto(fd, buf, n, 0, (struct sockaddr*)&peer, plen);
        }
        close(fd);
        _exit(0);
    }

    struct timespec sleep{ .tv_sec = 0, .tv_nsec = 50'000'000 };
    nanosleep(&sleep, nullptr);

    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);

    socklen_t clen;
    auto caddr = make_abstract_addr("robot_rtt_client", clen);
    bind(fd, (struct sockaddr*)&caddr, clen);

    socklen_t slen;
    auto saddr = make_abstract_addr("robot_rtt_server", slen);

    uint8_t buf[64]{};
    std::vector<int64_t> rtts;
    rtts.reserve(N);

    for (int i = 0; i < N; ++i) {
        int64_t t0 = mono_ns();
        sendto(fd, buf, sizeof(buf), 0, (struct sockaddr*)&saddr, slen);
        recv(fd, buf, sizeof(buf), 0);
        int64_t rtt = mono_ns() - t0;
        rtts.push_back(rtt);
    }

    close(fd);
    wait(nullptr);

    std::sort(rtts.begin(), rtts.end());
    double avg = std::accumulate(rtts.begin(), rtts.end(), 0.0) / rtts.size();

    printf("SOCK_DGRAM round-trip latency (%d samples):\n", N);
    printf("  min:  %.1f μs\n", rtts.front() / 1000.0);
    printf("  avg:  %.1f μs\n", avg / 1000.0);
    printf("  p99:  %.1f μs\n", rtts[(size_t)(0.99 * rtts.size())] / 1000.0);
    printf("  max:  %.1f μs\n", rtts.back() / 1000.0);
}

// ─── Ví dụ 3: connect() trên DGRAM — gọn hơn ─────────────────────────────
void example_connected_dgram() {
    std::cout << "\n=== Ví dụ 3: Connected DGRAM (dùng send thay sendto) ===\n";

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
        socklen_t slen;
        auto saddr = make_abstract_addr("robot_connected_dgram", slen);
        bind(fd, (struct sockaddr*)&saddr, slen);

        uint8_t buf[32];
        recv(fd, buf, sizeof(buf), 0);
        printf("[Server] Received %zd bytes\n", sizeof(buf));
        close(fd);
        _exit(0);
    }

    struct timespec sleep{ .tv_sec = 0, .tv_nsec = 50'000'000 };
    nanosleep(&sleep, nullptr);

    int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    socklen_t slen;
    auto saddr = make_abstract_addr("robot_connected_dgram", slen);

    // connect() trên DGRAM: set default destination
    connect(fd, (struct sockaddr*)&saddr, slen);

    uint8_t buf[32]{};
    send(fd, buf, sizeof(buf), 0);  // Không cần sendto sau khi connect
    printf("[Client] Sent via connected DGRAM\n");

    close(fd);
    wait(nullptr);
}

int main() {
    std::cout << "=== Phase2 Bài 04: UNIX Socket SOCK_DGRAM ===\n";

    example_dgram_basic();
    example_latency_measurement();
    example_connected_dgram();

    return 0;
}
