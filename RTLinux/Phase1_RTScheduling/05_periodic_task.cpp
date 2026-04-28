/**
 * PHASE 1 - Bài 05: Periodic Task với clock_nanosleep (Absolute Time)
 *
 * Mục tiêu:
 *  - Dùng clock_nanosleep với TIMER_ABSTIME để tránh drift
 *  - So sánh: nanosleep (relative) vs clock_nanosleep (absolute)
 *  - Đo jitter thực tế của periodic task
 *  - Áp dụng cho control loop robot (1kHz)
 *
 * Compile: g++ -std=c++17 -O2 05_periodic_task.cpp -o out -lrt -lpthread
 * Run:     sudo ./out
 */

#include <iostream>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <cstring>
#include <cerrno>
#include <vector>
#include <algorithm>
#include <cmath>
#include <atomic>
#include <cstdint>

std::atomic<bool> g_running{ true };

// ─── Helpers ──────────────────────────────────────────────────────────────
inline void timespec_add_ns(struct timespec& ts, long ns) {
    ts.tv_nsec += ns;
    while (ts.tv_nsec >= 1'000'000'000L) {
        ts.tv_nsec -= 1'000'000'000L;
        ts.tv_sec++;
    }
}

inline int64_t timespec_diff_ns(const struct timespec& a, const struct timespec& b) {
    return ((int64_t)a.tv_sec - b.tv_sec) * 1'000'000'000LL
         + ((int64_t)a.tv_nsec - b.tv_nsec);
}

// ─── Ví dụ 1: Drift với nanosleep (SAIT) ──────────────────────────────────
void example_relative_sleep_drift() {
    std::cout << "\n=== Ví dụ 1: Drift với nanosleep (relative) ===\n";

    const long PERIOD_NS = 1'000'000;  // 1ms
    const int  N         = 20;

    struct timespec t0, now;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int i = 0; i < N; ++i) {
        // --- Work time (simulate 200μs processing) ---
        struct timespec work_ts{ .tv_sec = 0, .tv_nsec = 200'000 };
        nanosleep(&work_ts, nullptr);

        // --- WRONG: relative sleep — drift accumulates ---
        struct timespec sleep_ts{ .tv_sec = 0, .tv_nsec = PERIOD_NS - 200'000 };
        nanosleep(&sleep_ts, nullptr);

        clock_gettime(CLOCK_MONOTONIC, &now);
        int64_t elapsed = timespec_diff_ns(now, t0);
        int64_t expected = (int64_t)(i + 1) * PERIOD_NS;
        int64_t drift    = elapsed - expected;
        if (i < 5 || i == N - 1)
            printf("  iter %2d: elapsed=%.3fms expected=%.3fms drift=%+ld μs\n",
                   i, elapsed / 1e6, expected / 1e6, drift / 1000);
    }
    std::cout << "→ Drift tích lũy theo thời gian!\n";
}

// ─── Ví dụ 2: Absolute sleep — không drift ───────────────────────────────
void example_absolute_sleep_no_drift() {
    std::cout << "\n=== Ví dụ 2: clock_nanosleep với TIMER_ABSTIME ===\n";

    const long PERIOD_NS = 1'000'000;  // 1ms
    const int  N         = 20;

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);   // điểm bắt đầu

    struct timespec t0 = next;
    std::vector<int64_t> jitters;

    for (int i = 0; i < N; ++i) {
        // Tính thời điểm wakeup TUYỆT ĐỐI tiếp theo
        timespec_add_ns(next, PERIOD_NS);

        // Sleep đến ĐÚNG thời điểm đó
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);

        // Đo thực tế
        struct timespec actual;
        clock_gettime(CLOCK_MONOTONIC, &actual);
        int64_t latency = timespec_diff_ns(actual, next);  // > 0 = wakeup muộn
        jitters.push_back(latency);

        if (i < 5 || i == N - 1)
            printf("  iter %2d: latency=%+ld ns\n", i, latency);
    }

    // Thống kê
    std::sort(jitters.begin(), jitters.end());
    double avg = 0;
    for (auto v : jitters) avg += v;
    avg /= jitters.size();

    printf("Jitter: min=%ld ns  avg=%.0f ns  max=%ld ns\n",
           jitters.front(), avg, jitters.back());
    std::cout << "→ Không drift! Mỗi iteration đúng với thời điểm tuyệt đối.\n";
}

// ─── Ví dụ 3: RT periodic task thread ────────────────────────────────────
struct PeriodicStats {
    int64_t min_jitter_ns  = INT64_MAX;
    int64_t max_jitter_ns  = INT64_MIN;
    int64_t total_jitter   = 0;
    int     count          = 0;
    int     overruns       = 0;  // jitter > half_period
};

void* control_loop_thread(void* arg) {
    auto* stats = static_cast<PeriodicStats*>(arg);

    const long PERIOD_NS = 1'000'000;  // 1ms = 1kHz
    const long DEADLINE  = PERIOD_NS;
    const int  N         = 200;       // 200ms total

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    for (int i = 0; i < N && g_running; ++i) {
        timespec_add_ns(next, PERIOD_NS);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);

        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        int64_t jitter = timespec_diff_ns(now, next);
        stats->min_jitter_ns = std::min(stats->min_jitter_ns, jitter);
        stats->max_jitter_ns = std::max(stats->max_jitter_ns, jitter);
        stats->total_jitter += jitter;
        stats->count++;
        if (jitter > DEADLINE / 2) stats->overruns++;

        // Simulate control computation
        volatile double result = 0;
        for (int k = 0; k < 1000; ++k) result += k * 0.001;
        (void)result;
    }
    return nullptr;
}

void example_rt_control_loop() {
    std::cout << "\n=== Ví dụ 3: RT Control Loop thread (1kHz) ===\n";

    PeriodicStats stats;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    sched_param sp{ .sched_priority = 80 };
    pthread_attr_setschedparam(&attr, &sp);

    pthread_t tid;
    int ret = pthread_create(&tid, &attr, control_loop_thread, &stats);
    if (ret != 0) {
        std::cerr << "create RT thread failed: " << strerror(ret)
                  << " (run with sudo). Falling back to normal thread.\n";
        pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
        sp.sched_priority = 0;
        pthread_attr_setschedparam(&attr, &sp);
        pthread_create(&tid, &attr, control_loop_thread, &stats);
    }
    pthread_attr_destroy(&attr);
    pthread_join(tid, nullptr);

    printf("Control loop (1kHz, %d iters):\n", stats.count);
    printf("  min jitter: %ld ns\n", stats.min_jitter_ns);
    printf("  avg jitter: %ld ns\n", stats.total_jitter / stats.count);
    printf("  max jitter: %ld ns\n", stats.max_jitter_ns);
    printf("  overruns:   %d\n",     stats.overruns);
}

int main() {
    std::cout << "=== Phase1 Bài 05: Periodic Task với clock_nanosleep ===\n";
    std::cout << "(Cần sudo để set RT priority)\n";

    example_relative_sleep_drift();
    example_absolute_sleep_no_drift();
    example_rt_control_loop();

    return 0;
}
