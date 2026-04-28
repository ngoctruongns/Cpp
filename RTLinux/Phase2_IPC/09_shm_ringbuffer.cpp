/**
 * PHASE 2 - Bài 09: Lock-free Ring Buffer qua Shared Memory
 *
 * Mục tiêu:
 *  - Implement lock-free ring buffer dùng atomic indices
 *  - Đặt ring buffer vào POSIX shared memory → zero-copy IPC
 *  - Producer/consumer trong 2 processes
 *  - Cache-line padding để tránh false sharing
 *  - Benchmark throughput và latency
 *
 * Compile: g++ -std=c++17 -O2 09_shm_ringbuffer.cpp -o out -lrt -lpthread
 */

#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <atomic>
#include <sys/wait.h>
#include <time.h>
#include <cassert>

constexpr const char* SHM_RB_NAME = "/robot_ringbuf_09";

// ─── Lock-free Ring Buffer ────────────────────────────────────────────────
// Yêu cầu: size là power of 2, T là POD
// Single-producer single-consumer (SPSC)

template<typename T, size_t N>
struct alignas(64) LockFreeRingBuffer {
    static_assert((N & (N - 1)) == 0, "N must be power of 2");
    static_assert(std::is_trivially_copyable<T>::value, "T must be POD");

    // Cache-line padding để tránh false sharing
    alignas(64) std::atomic<size_t> head_{ 0 };  // producer writes here
    char pad1_[64 - sizeof(std::atomic<size_t>)];

    alignas(64) std::atomic<size_t> tail_{ 0 };  // consumer reads from here
    char pad2_[64 - sizeof(std::atomic<size_t>)];

    alignas(64) T data_[N];

    static constexpr size_t MASK = N - 1;

    // Producer: return false nếu full
    bool push(const T& item) {
        size_t head = head_.load(std::memory_order_relaxed);
        size_t next = (head + 1) & MASK;

        if (next == tail_.load(std::memory_order_acquire))
            return false;  // full

        data_[head] = item;
        head_.store(next, std::memory_order_release);
        return true;
    }

    // Consumer: return false nếu empty
    bool pop(T& item) {
        size_t tail = tail_.load(std::memory_order_relaxed);

        if (tail == head_.load(std::memory_order_acquire))
            return false;  // empty

        item = data_[tail];
        tail_.store((tail + 1) & MASK, std::memory_order_release);
        return true;
    }

    bool empty() const {
        return tail_.load(std::memory_order_acquire) ==
               head_.load(std::memory_order_acquire);
    }

    size_t size() const {
        size_t h = head_.load(std::memory_order_acquire);
        size_t t = tail_.load(std::memory_order_acquire);
        return (h - t + N) & MASK;
    }

    void reset() {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }
};

// ─── Data type ────────────────────────────────────────────────────────────
struct SensorSample {
    uint32_t seq;
    double   velocity_left;
    double   velocity_right;
    double   imu_yaw;
    uint64_t timestamp_ns;
};

using SensorRingBuf = LockFreeRingBuffer<SensorSample, 256>;

inline int64_t mono_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1'000'000'000LL + ts.tv_nsec;
}

// ─── Ví dụ 1: Cơ bản — push/pop trong cùng process ──────────────────────
void example_basic_ringbuf() {
    std::cout << "\n=== Ví dụ 1: Lock-free ring buffer — basic ===\n";

    LockFreeRingBuffer<int, 8> rb;
    rb.reset();

    // Push 5 items
    for (int i = 0; i < 5; ++i) {
        bool ok = rb.push(i * 10);
        printf("push(%d): %s\n", i * 10, ok ? "ok" : "full");
    }
    printf("size=%zu\n", rb.size());

    // Pop all
    int val;
    while (rb.pop(val))
        printf("pop → %d\n", val);
    printf("empty=%d\n", (int)rb.empty());
}

// ─── Ví dụ 2: Shared memory ring buffer giữa 2 processes ──────────────────
void example_shm_ringbuf() {
    std::cout << "\n=== Ví dụ 2: Shared memory ring buffer (fork) ===\n";

    // Tạo shared memory
    int fd = shm_open(SHM_RB_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(SensorRingBuf));
    auto* rb = static_cast<SensorRingBuf*>(
        mmap(nullptr, sizeof(SensorRingBuf),
             PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);
    rb->reset();

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        // ─── Child: Consumer ────────────────────────────────────────────
        SensorRingBuf* crb = static_cast<SensorRingBuf*>(
            mmap(nullptr, sizeof(SensorRingBuf),
                 PROT_READ | PROT_WRITE, MAP_SHARED,
                 shm_open(SHM_RB_NAME, O_RDWR, 0), 0));

        int received = 0;
        int64_t deadline = mono_ns() + 2'000'000'000LL;  // 2s timeout

        while (received < 10 && mono_ns() < deadline) {
            SensorSample s;
            if (crb->pop(s)) {
                printf("[Consumer] seq=%u vel=(%.2f,%.2f)\n",
                       s.seq, s.velocity_left, s.velocity_right);
                received++;
            } else {
                // Busy wait or yield
                sched_yield();
            }
        }

        munmap(crb, sizeof(SensorRingBuf));
        _exit(0);

    } else {
        // ─── Parent: Producer ────────────────────────────────────────────
        for (int i = 0; i < 10; ++i) {
            SensorSample s{
                .seq            = (uint32_t)i,
                .velocity_left  = i * 0.1,
                .velocity_right = i * 0.1 + 0.02,
                .imu_yaw        = i * 0.01,
                .timestamp_ns   = (uint64_t)mono_ns()
            };

            while (!rb->push(s)) sched_yield();  // retry if full

            printf("[Producer] sent seq=%u\n", s.seq);

            struct timespec sleep{ .tv_sec = 0, .tv_nsec = 30'000'000 };
            nanosleep(&sleep, nullptr);
        }

        wait(nullptr);
        munmap(rb, sizeof(SensorRingBuf));
        shm_unlink(SHM_RB_NAME);
    }
}

// ─── Ví dụ 3: Benchmark throughput ──────────────────────────────────────
void example_throughput() {
    std::cout << "\n=== Ví dụ 3: Throughput benchmark ===\n";

    const int N = 1'000'000;
    LockFreeRingBuffer<int, 1024> rb;
    rb.reset();

    int64_t t0 = mono_ns();

    // Single-threaded push/pop alternating (worst case latency)
    long pushed = 0, popped = 0;
    for (int i = 0; i < N; ++i) {
        if (rb.push(i)) pushed++;
        int v;
        if (rb.pop(v)) popped++;
    }

    int64_t elapsed = mono_ns() - t0;
    printf("N=%d, pushed=%ld popped=%ld in %.1fms\n",
           N, pushed, popped, elapsed / 1e6);
    printf("Throughput: %.0f ops/ms = %.0f M/s\n",
           (double)(pushed + popped) / (elapsed / 1e6),
           (double)(pushed + popped) / (elapsed / 1000.0));
}

int main() {
    std::cout << "=== Phase2 Bài 09: Lock-free Ring Buffer via Shared Memory ===\n";

    example_basic_ringbuf();
    example_shm_ringbuf();
    example_throughput();

    return 0;
}
