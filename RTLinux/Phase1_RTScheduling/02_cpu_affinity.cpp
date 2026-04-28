/**
 * PHASE 1 - Bài 02: CPU Affinity
 *
 * Mục tiêu:
 *  - Set CPU affinity cho thread (pin vào CPU cụ thể)
 *  - So sánh hiệu năng trước/sau khi pin CPU
 *  - Kiểm tra isolation: thread không migrate khi CPU bận
 *  - Xem affinity bằng taskset / /proc
 *
 * Compile: g++ -std=c++17 -O2 02_cpu_affinity.cpp -o out -lpthread
 * Run:     sudo ./out
 */

#include <iostream>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <chrono>

// ─── Ví dụ 1: Xem số CPU và affinity mặc định ────────────────────────────
void example_show_cpu_info() {
    std::cout << "\n=== Ví dụ 1: CPU info ===\n";

    int n_cpu = (int)sysconf(_SC_NPROCESSORS_ONLN);
    std::cout << "Online CPUs: " << n_cpu << "\n";

    cpu_set_t mask;
    sched_getaffinity(0, sizeof(mask), &mask);  // 0 = current process

    std::cout << "Current process affinity: CPUs { ";
    for (int i = 0; i < n_cpu; ++i)
        if (CPU_ISSET(i, &mask)) std::cout << i << " ";
    std::cout << "}\n";
}

// ─── Ví dụ 2: Pin thread vào CPU cụ thể ──────────────────────────────────
struct AffinityArg {
    int target_cpu;
    long work_count;
};

void* pinned_thread_fn(void* arg) {
    auto* a = static_cast<AffinityArg*>(arg);

    // Kiểm tra CPU thread đang chạy
    int cpu_before = sched_getcpu();
    std::cout << "[Thread] Running on CPU " << cpu_before
              << " (requested: " << a->target_cpu << ")\n";

    // Do work
    volatile long x = 0;
    for (long i = 0; i < a->work_count; ++i) x += i;

    int cpu_after = sched_getcpu();
    std::cout << "[Thread] Finished on CPU " << cpu_after
              << " sum=" << x % 1000 << "\n";
    return nullptr;
}

void example_pin_thread_to_cpu() {
    std::cout << "\n=== Ví dụ 2: Pin thread to CPU ===\n";

    int n_cpu = (int)sysconf(_SC_NPROCESSORS_ONLN);
    int target_cpu = (n_cpu > 1) ? (n_cpu - 1) : 0;  // Dùng CPU cuối

    AffinityArg arg{ .target_cpu = target_cpu, .work_count = 200'000'000L };

    // Tạo thread với affinity
    pthread_attr_t attr;
    pthread_attr_init(&attr);

    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(target_cpu, &cpuset);
    pthread_attr_setaffinity_np(&attr, sizeof(cpuset), &cpuset);

    // Set SCHED_FIFO để giữ CPU
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    sched_param sp{ .sched_priority = 60 };
    pthread_attr_setschedparam(&attr, &sp);

    pthread_t tid;
    int ret = pthread_create(&tid, &attr, pinned_thread_fn, &arg);
    if (ret != 0) {
        std::cerr << "pthread_create failed: " << strerror(ret)
                  << " (try sudo)\n";
        // Thử lại không có RT priority
        pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
        sp.sched_priority = 0;
        pthread_attr_setschedparam(&attr, &sp);
        pthread_create(&tid, &attr, pinned_thread_fn, &arg);
    }

    pthread_attr_destroy(&attr);
    pthread_join(tid, nullptr);
}

// ─── Ví dụ 3: Thay đổi affinity của thread đang chạy ─────────────────────
void example_change_affinity_runtime() {
    std::cout << "\n=== Ví dụ 3: Change affinity at runtime ===\n";

    int n_cpu = (int)sysconf(_SC_NPROCESSORS_ONLN);
    std::cout << "Current CPU: " << sched_getcpu() << "\n";

    // Pin process hiện tại vào CPU 0
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) == 0)
        std::cout << "Pinned to CPU 0, now on: " << sched_getcpu() << "\n";
    else
        std::cerr << "sched_setaffinity failed: " << strerror(errno) << "\n";

    // Restore: allow all CPUs
    for (int i = 0; i < n_cpu; ++i) CPU_SET(i, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
    std::cout << "Restored all CPUs\n";
}

// ─── Ví dụ 4: Đo thời gian với và không có CPU pinning ───────────────────
void example_benchmark_with_affinity() {
    std::cout << "\n=== Ví dụ 4: Benchmark — no affinity vs pinned ===\n";

    auto do_work = []() {
        auto t0 = std::chrono::steady_clock::now();
        volatile long x = 0;
        for (long i = 0; i < 500'000'000L; ++i) x += i;
        auto t1 = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    };

    // Không pin
    long ms_free = do_work();
    std::cout << "No affinity:    " << ms_free << " ms\n";

    // Pin vào CPU 0
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);

    long ms_pinned = do_work();
    std::cout << "Pinned to CPU0: " << ms_pinned << " ms\n";
    std::cout << "(Difference small unless system is loaded; "
                 "benefit shows under load due to cache locality)\n";

    // Restore
    int n_cpu = (int)sysconf(_SC_NPROCESSORS_ONLN);
    for (int i = 0; i < n_cpu; ++i) CPU_SET(i, &mask);
    sched_setaffinity(0, sizeof(mask), &mask);
}

int main() {
    std::cout << "=== Phase1 Bài 02: CPU Affinity ===\n";

    example_show_cpu_info();
    example_pin_thread_to_cpu();
    example_change_affinity_runtime();
    example_benchmark_with_affinity();

    std::cout << "\nTip: Xem affinity bên ngoài:\n"
              << "  taskset -cp <PID>\n"
              << "  cat /proc/<PID>/status | grep Cpus_allowed\n";
    return 0;
}
