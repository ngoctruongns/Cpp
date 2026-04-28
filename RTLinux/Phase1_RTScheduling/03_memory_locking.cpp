/**
 * PHASE 1 - Bài 03: Memory Locking (mlockall + stack prefault)
 *
 * Mục tiêu:
 *  - Hiểu tại sao page fault gây latency spike trong RT code
 *  - Dùng mlockall() để lock toàn bộ memory vào RAM
 *  - Prefault stack để tránh page fault lúc runtime
 *  - Pre-allocate heap, đo latency trước và sau mlockall
 *
 * Compile: g++ -std=c++17 -O2 03_memory_locking.cpp -o out -lpthread
 * Run:     sudo ./out
 */

#include <iostream>
#include <sys/mman.h>
#include <pthread.h>
#include <chrono>
#include <cstring>
#include <cerrno>
#include <vector>
#include <numeric>
#include <algorithm>

// ─── Ví dụ 1: Tại sao cần lock memory ───────────────────────────────────
void example_why_lock_memory() {
    std::cout << "\n=== Ví dụ 1: Tại sao cần lock memory ===\n";
    std::cout <<
        "Vấn đề: Linux dùng demand paging\n"
        "  - Khi cấp phát memory (new/malloc), kernel chưa map physical page ngay\n"
        "  - Lần đầu truy cập page đó → Page Fault → kernel phải map → ~10μs-1ms\n"
        "  - Với RT code cần latency < 100μs: 1 page fault = FAIL\n\n"
        "Giải pháp:\n"
        "  1. mlockall(MCL_CURRENT | MCL_FUTURE): lock tất cả pages hiện tại và tương lai\n"
        "  2. Stack prefault: touch từng page trong stack để trigger page fault ngay\n"
        "  3. Pre-allocate heap: new/malloc trước khi vào RT loop\n";
}

// ─── Ví dụ 2: mlockall ───────────────────────────────────────────────────
void example_mlockall() {
    std::cout << "\n=== Ví dụ 2: mlockall ===\n";

    // MCL_CURRENT: lock tất cả pages đang có trong process address space
    // MCL_FUTURE:  lock tự động các pages được map trong tương lai
    int ret = mlockall(MCL_CURRENT | MCL_FUTURE);
    if (ret != 0) {
        std::cerr << "mlockall failed: " << strerror(errno)
                  << "\n  → Cần chạy với sudo hoặc set RLIMIT_MEMLOCK\n"
                  << "  → Hoặc: sudo setcap cap_ipc_lock+ep ./out\n";
        return;
    }
    std::cout << "mlockall() success — all memory locked to RAM\n";

    // Sau khi mlockall: tất cả future allocations cũng bị lock
    std::vector<char> buf(1024 * 1024);  // 1MB — locked ngay lập tức
    std::cout << "Allocated 1MB buffer (locked)\n";

    // Unlock nếu muốn (thường không cần)
    // munlockall();
}

// ─── Ví dụ 3: Stack prefault ─────────────────────────────────────────────
void stack_prefault(size_t stack_size = 8 * 1024 * 1024) {
    // Touch từng page trong stack để trigger page fault ngay bây giờ
    // Phải làm TRƯỚC khi vào RT loop
    volatile char dummy[stack_size];
    for (size_t i = 0; i < stack_size; i += 4096)
        dummy[i] = 0;
    // Ngăn compiler optimize out bằng volatile
}

void example_stack_prefault() {
    std::cout << "\n=== Ví dụ 3: Stack prefault ===\n";

    auto t0 = std::chrono::steady_clock::now();
    stack_prefault(256 * 1024);  // 256KB stack prefault
    auto t1 = std::chrono::steady_clock::now();

    long us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
    std::cout << "stack_prefault(256KB) took: " << us << " μs\n";
    std::cout << "After prefault: no more page faults on stack access\n";
}

// ─── Ví dụ 4: Đo latency có/không có mlockall ────────────────────────────
long measure_access_latency_us(size_t buf_size) {
    // Không prefault — first access sẽ trigger page fault
    char* buf = new char[buf_size];

    auto t0 = std::chrono::steady_clock::now();
    // First access to each page
    for (size_t i = 0; i < buf_size; i += 4096)
        buf[i] = 1;
    auto t1 = std::chrono::steady_clock::now();

    delete[] buf;
    return std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
}

void example_latency_comparison() {
    std::cout << "\n=== Ví dụ 4: Latency — first access (page faults) ===\n";

    const size_t BUF = 4 * 1024 * 1024;  // 4MB

    long us = measure_access_latency_us(BUF);
    std::cout << "First access of " << BUF / 1024 << "KB: " << us << " μs\n";
    std::cout << "(~1000 page faults × ~" << us / (BUF / 4096)
              << "μs each = bad for RT!)\n";

    // Sau khi mlockall: tất cả future alloc được prefaulted
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == 0) {
        long us2 = measure_access_latency_us(BUF);
        std::cout << "After mlockall, first access: " << us2 << " μs\n";
        std::cout << "Improvement: " << (us - us2) << " μs saved\n";
    }
}

// ─── Ví dụ 5: Pre-allocate heap với object pool ───────────────────────────
template<typename T, size_t N>
class StaticPool {
    alignas(T) char storage_[sizeof(T) * N];
    bool  used_[N]{};

public:
    T* acquire() {
        for (size_t i = 0; i < N; ++i)
            if (!used_[i]) {
                used_[i] = true;
                return new (&storage_[i * sizeof(T)]) T{};
            }
        return nullptr;  // pool exhausted
    }

    void release(T* p) {
        size_t i = (reinterpret_cast<char*>(p) - storage_) / sizeof(T);
        p->~T();
        used_[i] = false;
    }
};

void example_object_pool() {
    std::cout << "\n=== Ví dụ 5: Static Object Pool (no malloc in RT loop) ===\n";

    struct Packet { double data[8]; uint64_t ts; };
    StaticPool<Packet, 32> pool;

    // Pre-allocate (trước RT loop)
    auto* p1 = pool.acquire();
    auto* p2 = pool.acquire();
    std::cout << "Acquired 2 packets from pool (no malloc)\n";

    // Use
    p1->data[0] = 1.23;
    p2->ts      = 456789;

    // Release
    pool.release(p1);
    pool.release(p2);
    std::cout << "Released back to pool\n";
    std::cout << "RT loop: acquire/release pool — O(N) but no syscall, no page fault\n";
}

int main() {
    std::cout << "=== Phase1 Bài 03: Memory Locking ===\n";
    std::cout << "(Một số ví dụ cần sudo)\n";

    example_why_lock_memory();
    example_mlockall();
    example_stack_prefault();
    example_latency_comparison();
    example_object_pool();

    return 0;
}
