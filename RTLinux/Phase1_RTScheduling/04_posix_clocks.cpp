/**
 * PHASE 1 - Bài 04: POSIX Clocks & High-Resolution Timing
 *
 * Mục tiêu:
 *  - Phân biệt các POSIX clock IDs (MONOTONIC, REALTIME, PROCESS...)
 *  - Dùng clock_gettime() để đo thời gian chính xác
 *  - Viết helper: timespec_diff_ns, timespec_add_ns
 *  - Đo clock resolution và overhead
 *
 * Compile: g++ -std=c++17 -O2 04_posix_clocks.cpp -o out -lrt
 */

#include <iostream>
#include <time.h>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <numeric>

// ─── Helper functions ──────────────────────────────────────────────────────

// Convert timespec → nanoseconds
inline int64_t timespec_to_ns(const struct timespec& ts) {
    return (int64_t)ts.tv_sec * 1'000'000'000LL + ts.tv_nsec;
}

// Difference: a - b in nanoseconds
inline int64_t timespec_diff_ns(const struct timespec& a, const struct timespec& b) {
    return timespec_to_ns(a) - timespec_to_ns(b);
}

// Add nanoseconds to timespec (handles overflow)
inline void timespec_add_ns(struct timespec& ts, long ns) {
    ts.tv_nsec += ns;
    while (ts.tv_nsec >= 1'000'000'000L) {
        ts.tv_nsec -= 1'000'000'000L;
        ts.tv_sec++;
    }
}

// ─── Ví dụ 1: Các loại clock ─────────────────────────────────────────────
void example_clock_ids() {
    std::cout << "\n=== Ví dụ 1: POSIX Clock IDs ===\n";

    struct { clockid_t id; const char* name; const char* desc; } clocks[] = {
        { CLOCK_REALTIME,         "CLOCK_REALTIME",         "Wall clock — bị ảnh hưởng bởi NTP/settime" },
        { CLOCK_MONOTONIC,        "CLOCK_MONOTONIC",        "Monotonic — không bị settime ảnh hưởng (DÙNG CHO RT)" },
        { CLOCK_PROCESS_CPUTIME_ID, "CLOCK_PROCESS_CPUTIME_ID", "CPU time của process" },
        { CLOCK_THREAD_CPUTIME_ID,  "CLOCK_THREAD_CPUTIME_ID",  "CPU time của thread hiện tại" },
        { CLOCK_MONOTONIC_RAW,    "CLOCK_MONOTONIC_RAW",    "Không điều chỉnh NTP — dùng đo hardware latency" },
    };

    for (auto& c : clocks) {
        struct timespec ts;
        if (clock_gettime(c.id, &ts) == 0) {
            printf("%-30s = %ld.%09ld s  (%s)\n",
                   c.name, ts.tv_sec, ts.tv_nsec, c.desc);
        }
    }

    std::cout << "\n→ Dùng CLOCK_MONOTONIC cho RT periodic tasks!\n";
}

// ─── Ví dụ 2: Đo clock resolution ────────────────────────────────────────
void example_clock_resolution() {
    std::cout << "\n=== Ví dụ 2: Clock resolution ===\n";

    clockid_t ids[] = { CLOCK_REALTIME, CLOCK_MONOTONIC, CLOCK_MONOTONIC_RAW };
    const char* names[] = { "REALTIME", "MONOTONIC", "MONOTONIC_RAW" };

    for (int i = 0; i < 3; ++i) {
        struct timespec res;
        clock_getres(ids[i], &res);
        printf("%-20s resolution: %ld ns\n",
               names[i], res.tv_sec * 1'000'000'000L + res.tv_nsec);
    }
    std::cout << "(Thường là 1ns trên Linux hiện đại với HPET/TSC)\n";
}

// ─── Ví dụ 3: Đo thời gian thực thi ─────────────────────────────────────
void example_measure_execution_time() {
    std::cout << "\n=== Ví dụ 3: Đo thời gian thực thi ===\n";

    struct timespec t_start, t_end;
    clock_gettime(CLOCK_MONOTONIC, &t_start);

    // Simulate some work
    volatile long x = 0;
    for (long i = 0; i < 10'000'000L; ++i) x += i;

    clock_gettime(CLOCK_MONOTONIC, &t_end);

    int64_t elapsed_ns = timespec_diff_ns(t_end, t_start);
    printf("Work done in: %ld ns (%.3f ms), result=%ld\n",
           elapsed_ns, elapsed_ns / 1'000'000.0, x % 1000);
}

// ─── Ví dụ 4: Đo overhead của clock_gettime ──────────────────────────────
void example_clock_overhead() {
    std::cout << "\n=== Ví dụ 4: clock_gettime overhead ===\n";

    const int N = 100'000;
    std::vector<int64_t> samples;
    samples.reserve(N);

    struct timespec t0, t1;
    for (int i = 0; i < N; ++i) {
        clock_gettime(CLOCK_MONOTONIC, &t0);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        int64_t diff = timespec_diff_ns(t1, t0);
        if (diff > 0) samples.push_back(diff);
    }

    std::sort(samples.begin(), samples.end());
    int64_t sum = std::accumulate(samples.begin(), samples.end(), 0LL);

    printf("clock_gettime() overhead (%d samples):\n", N);
    printf("  min  = %ld ns\n", samples.front());
    printf("  avg  = %.1f ns\n", (double)sum / samples.size());
    printf("  p99  = %ld ns\n", samples[(size_t)(0.99 * samples.size())]);
    printf("  max  = %ld ns\n", samples.back());
    std::cout << "(clock_gettime là vDSO call — không cần syscall, rất nhanh)\n";
}

// ─── Ví dụ 5: timespec helpers ───────────────────────────────────────────
void example_timespec_helpers() {
    std::cout << "\n=== Ví dụ 5: timespec helper functions ===\n";

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    struct timespec future = now;
    timespec_add_ns(future, 5'000'000);  // +5ms

    int64_t diff = timespec_diff_ns(future, now);
    printf("now:    %ld.%09ld\n", now.tv_sec, now.tv_nsec);
    printf("future: %ld.%09ld\n", future.tv_sec, future.tv_nsec);
    printf("diff:   %ld ns = %.3f ms\n", diff, diff / 1'000'000.0);
}

int main() {
    std::cout << "=== Phase1 Bài 04: POSIX Clocks & Timing ===\n";

    example_clock_ids();
    example_clock_resolution();
    example_measure_execution_time();
    example_clock_overhead();
    example_timespec_helpers();

    return 0;
}
