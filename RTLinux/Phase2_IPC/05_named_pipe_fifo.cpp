/**
 * PHASE 2 - Bài 05: Named FIFO (mkfifo)
 *
 * Mục tiêu:
 *  - Tạo FIFO với mkfifo, mở và đọc/ghi
 *  - Blocking vs non-blocking open
 *  - Bidirectional IPC với 2 FIFOs
 *  - FIFO vs socket: khi nào dùng FIFO?
 *
 * Compile: g++ -std=c++17 -O2 05_named_pipe_fifo.cpp -o out
 */

#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <sys/wait.h>
#include <time.h>

constexpr const char* FIFO_PATH  = "/tmp/robot_fifo_01";
constexpr const char* FIFO_PATH2 = "/tmp/robot_fifo_02";

// ─── Ví dụ 1: Cơ bản — tạo, ghi, đọc FIFO ────────────────────────────────
void example_basic_fifo() {
    std::cout << "\n=== Ví dụ 1: Basic FIFO ===\n";

    // Tạo FIFO (nếu đã có thì unlink trước)
    unlink(FIFO_PATH);
    if (mkfifo(FIFO_PATH, 0666) < 0) {
        perror("mkfifo"); return;
    }
    printf("Created FIFO: %s\n", FIFO_PATH);

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        // ─── Child: Reader
        // open() BLOCK cho đến khi có writer
        printf("[Reader] Opening FIFO for read (will block until writer opens)...\n");
        int fd = open(FIFO_PATH, O_RDONLY);
        if (fd < 0) { perror("open"); _exit(1); }

        char buf[64];
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            printf("[Reader] Received: '%s'\n", buf);
        }
        close(fd);
        _exit(0);

    } else {
        // ─── Parent: Writer
        // open() cũng BLOCK cho đến khi có reader
        printf("[Writer] Opening FIFO for write...\n");
        int fd = open(FIFO_PATH, O_WRONLY);
        if (fd < 0) { perror("open"); return; }

        const char* msg = "Hello from writer!";
        write(fd, msg, strlen(msg));
        printf("[Writer] Sent: '%s'\n", msg);
        close(fd);

        wait(nullptr);
        unlink(FIFO_PATH);
    }
}

// ─── Ví dụ 2: Non-blocking open ───────────────────────────────────────────
void example_nonblocking_open() {
    std::cout << "\n=== Ví dụ 2: Non-blocking FIFO open ===\n";

    unlink(FIFO_PATH);
    mkfifo(FIFO_PATH, 0666);

    // O_RDONLY | O_NONBLOCK: return ngay dù không có writer
    int fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
    } else {
        printf("FIFO opened non-blocking (no writer yet)\n");

        char buf[64];
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n < 0 && errno == EAGAIN)
            printf("read() returned EAGAIN — no data available (expected)\n");

        close(fd);
    }

    // O_WRONLY | O_NONBLOCK: return ngay với ENXIO nếu không có reader
    fd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
    if (fd < 0 && errno == ENXIO)
        printf("open(O_WRONLY|O_NONBLOCK) → ENXIO (no reader — expected)\n");
    else if (fd >= 0) close(fd);

    unlink(FIFO_PATH);
}

// ─── Ví dụ 3: Bidirectional — 2 FIFOs ────────────────────────────────────
struct Request  { int id; double value; };
struct Response { int id; double result; };

void example_bidirectional_fifo() {
    std::cout << "\n=== Ví dụ 3: Bidirectional IPC — 2 FIFOs ===\n";

    unlink(FIFO_PATH);
    unlink(FIFO_PATH2);
    mkfifo(FIFO_PATH,  0666);  // req:  parent→child
    mkfifo(FIFO_PATH2, 0666);  // resp: child→parent

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        // ─── Child: Server — đọc request, ghi response
        int req_fd  = open(FIFO_PATH,  O_RDONLY);
        int resp_fd = open(FIFO_PATH2, O_WRONLY);

        for (int i = 0; i < 3; ++i) {
            Request req;
            ssize_t n = read(req_fd, &req, sizeof(req));
            if (n != sizeof(req)) break;

            printf("[Server] Request id=%d value=%.2f\n", req.id, req.value);

            Response resp{ .id = req.id, .result = req.value * 2.0 };
            write(resp_fd, &resp, sizeof(resp));
        }

        close(req_fd);
        close(resp_fd);
        _exit(0);

    } else {
        // ─── Parent: Client — ghi request, đọc response
        int req_fd  = open(FIFO_PATH,  O_WRONLY);
        int resp_fd = open(FIFO_PATH2, O_RDONLY);

        for (int i = 0; i < 3; ++i) {
            Request req{ .id = i, .value = i * 1.5 };
            write(req_fd, &req, sizeof(req));

            Response resp;
            read(resp_fd, &resp, sizeof(resp));
            printf("[Client] Response id=%d result=%.2f\n", resp.id, resp.result);

            struct timespec sleep{ .tv_sec = 0, .tv_nsec = 50'000'000 };
            nanosleep(&sleep, nullptr);
        }

        close(req_fd);
        close(resp_fd);
        wait(nullptr);

        unlink(FIFO_PATH);
        unlink(FIFO_PATH2);
    }
}

// ─── Ví dụ 4: Khi nào dùng FIFO? ─────────────────────────────────────────
void example_fifo_vs_socket() {
    std::cout << "\n=== Ví dụ 4: FIFO vs UNIX Socket ===\n";
    std::cout <<
        "FIFO (Named Pipe):\n"
        "  + Đơn giản: chỉ cần open() — không cần bind/listen/accept\n"
        "  + Visible trong filesystem → dễ debug (ls /tmp/)\n"
        "  - Unidirectional: cần 2 FIFOs cho bidirectional\n"
        "  - Latency cao hơn socket (~5μs vs ~1μs)\n"
        "  - Không có message boundary (byte stream như pipe)\n"
        "  - Blocking open: cần cả 2 đầu mở cùng lúc\n"
        "  → Dùng khi: simple data pipeline, log forwarding, shell-like IPC\n\n"
        "UNIX Socket (SOCK_DGRAM):\n"
        "  + Thấp latency (~1μs round-trip)\n"
        "  + Message boundary (DGRAM)\n"
        "  + Abstract namespace — không cần cleanup file\n"
        "  + Bidirectional dễ dàng\n"
        "  → Dùng khi: RT data path, sensor→controller, low-latency IPC\n";
}

int main() {
    std::cout << "=== Phase2 Bài 05: Named FIFO ===\n";

    example_basic_fifo();
    example_nonblocking_open();
    example_bidirectional_fifo();
    example_fifo_vs_socket();

    return 0;
}
