/**
 * PHASE 1 - Bài 07: Latency Measurement
 *
 * Mục tiêu:
 *  - Đo wakeup latency của RT thread
 *  - Tính min/avg/max/stddev
 *  - In histogram dạng text
 *  - Phân tích nguyên nhân latency cao
 *
 * Compile: g++ -std=c++17 -O2 07_latency_measure.cpp -o out -lrt -lpthread -lm
 * Run:     sudo ./out
 */

#include <iostream>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <time.h>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>
#include <iomanip>

// ─── Helpers ──────────────────────────────────────────────────────────────
inline int64_t now_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1'000'000'000LL + ts.tv_nsec;
}

struct Stats {
    int64_t min_ns, max_ns;
    double  avg_ns, stddev_ns;
    std::vector<int64_t> samples;

    void compute() {
        min_ns = *std::min_element(samples.begin(), samples.end());
        max_ns = *std::max_element(samples.begin(), samples.end());
        double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
        avg_ns = sum / samples.size();

        double sq_sum = 0;
        for (auto v : samples) sq_sum += (v - avg_ns) * (v - avg_ns);
        stddev_ns = std::sqrt(sq_sum / samples.size());
    }

    int64_t percentile_ns(double p) const {
        std::vector<int64_t> sorted = samples;
        std::sort(sorted.begin(), sorted.end());
        size_t idx = (size_t)(p / 100.0 * sorted.size());
        return sorted[std::min(idx, sorted.size() - 1)];
    }

    void print(const char* title) const {
        printf("\n%s:\n", title);
        printf("  samples:  %zu\n",          samples.size());
        printf("  min:      %ld ns (%.1f μs)\n", min_ns, min_ns / 1000.0);
        printf("  avg:      %.0f ns (%.1f μs)\n", avg_ns, avg_ns / 1000.0);
        printf("  stddev:   %.0f ns\n",       stddev_ns);
        printf("  p95:      %ld ns (%.1f μs)\n", percentile_ns(95), percentile_ns(95) / 1000.0);
        printf("  p99:      %ld ns (%.1f μs)\n", percentile_ns(99), percentile_ns(99) / 1000.0);
        printf("  max:      %ld ns (%.1f μs)\n", max_ns, max_ns / 1000.0);
    }
};

// ─── Ví dụ 1: Đo wakeup latency (non-RT) ─────────────────────────────────
void example_measure_wakeup_latency_nonrt() {
    std::cout << "\n=== Ví dụ 1: Wakeup latency — SCHED_OTHER ===\n";

    const int    N         = 5000;
    const long   PERIOD_NS = 1'000'000;  // 1ms

    Stats stats;
    stats.samples.reserve(N);

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    for (int i = 0; i < N; ++i) {
        next.tv_nsec += PERIOD_NS;
        if (next.tv_nsec >= 1'000'000'000L) {
            next.tv_nsec -= 1'000'000'000L;
            next.tv_sec++;
        }

        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);

        struct timespec actual;
        clock_gettime(CLOCK_MONOTONIC, &actual);

        int64_t latency = ((int64_t)actual.tv_sec - next.tv_sec) * 1'000'000'000LL
                        + ((int64_t)actual.tv_nsec - next.tv_nsec);
        if (latency >= 0) stats.samples.push_back(latency);
    }

    stats.compute();
    stats.print("SCHED_OTHER wakeup latency");
}

// ─── Ví dụ 2: Đo wakeup latency (RT thread) ──────────────────────────────
void* rt_latency_thread(void* arg) {
    auto* stats = static_cast<Stats*>(arg);

    // RT setup
    mlockall(MCL_CURRENT | MCL_FUTURE);
    volatile char prefault[64 * 1024];
    for (size_t i = 0; i < sizeof(prefault); i += 4096) prefault[i] = 0;

    const int    N         = 5000;
    const long   PERIOD_NS = 1'000'000;

    stats->samples.reserve(N);

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    for (int i = 0; i < N; ++i) {
        next.tv_nsec += PERIOD_NS;
        if (next.tv_nsec >= 1'000'000'000L) {
            next.tv_nsec -= 1'000'000'000L;
            next.tv_sec++;
        }

        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);

        struct timespec actual;
        clock_gettime(CLOCK_MONOTONIC, &actual);

        int64_t latency = ((int64_t)actual.tv_sec - next.tv_sec) * 1'000'000'000LL
                        + ((int64_t)actual.tv_nsec - next.tv_nsec);
        if (latency >= 0) stats->samples.push_back(latency);
    }
    return nullptr;
}

void example_measure_wakeup_latency_rt() {
    std::cout << "\n=== Ví dụ 2: Wakeup latency — SCHED_FIFO (priority 80) ===\n";

    Stats stats;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    sched_param sp{ .sched_priority = 80 };
    pthread_attr_setschedparam(&attr, &sp);

    pthread_t tid;
    int ret = pthread_create(&tid, &attr, rt_latency_thread, &stats);
    if (ret != 0) {
        std::cerr << "RT thread failed: " << strerror(ret) << " (run with sudo)\n";
        // Fallback to normal thread for demo
        pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
        sp.sched_priority = 0;
        pthread_attr_setschedparam(&attr, &sp);
        pthread_create(&tid, &attr, rt_latency_thread, &stats);
    }
    pthread_attr_destroy(&attr);
    pthread_join(tid, nullptr);

    stats.compute();
    stats.print("SCHED_FIFO wakeup latency");
}

// ─── Ví dụ 3: Text histogram ──────────────────────────────────────────────
void print_histogram(const std::vector<int64_t>& samples, int64_t bucket_us = 10) {
    if (samples.empty()) return;

    int64_t max_val = *std::max_element(samples.begin(), samples.end()) / 1000;
    int64_t n_buckets = std::min((int64_t)20, max_val / bucket_us + 1);

    std::vector<long> hist(n_buckets, 0);
    long overflow = 0;

    for (auto ns : samples) {
        int64_t us    = ns / 1000;
        int64_t idx   = us / bucket_us;
        if (idx < n_buckets)
            hist[idx]++;
        else
            overflow++;
    }

    long max_count = *std::max_element(hist.begin(), hist.end());

    printf("\nLatency histogram (bucket = %ld μs):\n", bucket_us);
    for (int64_t i = 0; i < n_buckets; ++i) {
        int bar = (max_count > 0) ? (int)(40.0 * hist[i] / max_count) : 0;
        printf("%4ld-%4ld μs | %s %ld\n",
               i * bucket_us, (i + 1) * bucket_us - 1,
               std::string(bar, '#').c_str(), hist[i]);
    }
    if (overflow > 0)
        printf(">%4ld μs     | %ld (overruns!)\n", n_buckets * bucket_us, overflow);
}

void example_histogram() {
    std::cout << "\n=== Ví dụ 3: Histogram của latency ===\n";

    // Đo nhanh 1000 samples
    const int    N         = 1000;
    const long   PERIOD_NS = 1'000'000;
    std::vector<int64_t> samples;
    samples.reserve(N);

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    for (int i = 0; i < N; ++i) {
        next.tv_nsec += PERIOD_NS;
        if (next.tv_nsec >= 1'000'000'000L) {
            next.tv_nsec -= 1'000'000'000L;
            next.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);

        struct timespec actual;
        clock_gettime(CLOCK_MONOTONIC, &actual);

        int64_t latency = ((int64_t)actual.tv_sec - next.tv_sec) * 1'000'000'000LL
                        + ((int64_t)actual.tv_nsec - next.tv_nsec);
        if (latency >= 0) samples.push_back(latency);
    }

    print_histogram(samples, 5);  // 5μs buckets
}

int main() {
    std::cout << "=== Phase1 Bài 07: Latency Measurement ===\n";
    std::cout << "(Cần sudo để so sánh SCHED_FIFO vs SCHED_OTHER)\n";

    example_measure_wakeup_latency_nonrt();
    example_measure_wakeup_latency_rt();
    example_histogram();

    return 0;
}
