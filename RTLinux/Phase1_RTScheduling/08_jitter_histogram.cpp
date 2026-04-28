/**
 * PHASE 1 - Bài 08: Jitter Histogram với Percentiles
 *
 * Mục tiêu:
 *  - Đo jitter của periodic task (deviation khỏi period)
 *  - Tính percentiles: p50, p95, p99, p99.9
 *  - In histogram và phân tích phân phối
 *  - Export dữ liệu để vẽ đồ thị (gnuplot format)
 *
 * Compile: g++ -std=c++17 -O2 08_jitter_histogram.cpp -o out -lrt -lpthread -lm
 * Run:     sudo ./out
 * Plot:    gnuplot -e "plot 'jitter.dat' with boxes"
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
#include <fstream>
#include <map>

// ─── Helpers ──────────────────────────────────────────────────────────────
struct timespec now_ts() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts;
}

inline int64_t timespec_diff_ns(const struct timespec& a, const struct timespec& b) {
    return ((int64_t)a.tv_sec - b.tv_sec) * 1'000'000'000LL
         + ((int64_t)a.tv_nsec - b.tv_nsec);
}

// ─── Jitter collector ─────────────────────────────────────────────────────
class JitterCollector {
    std::vector<int64_t> samples_;  // signed jitter in ns
    int64_t period_ns_;

public:
    explicit JitterCollector(int64_t period_ns, size_t reserve = 10000)
        : period_ns_(period_ns) {
        samples_.reserve(reserve);
    }

    void add_sample(int64_t actual_ns, int64_t expected_ns) {
        samples_.push_back(actual_ns - expected_ns);
    }

    size_t count() const { return samples_.size(); }

    // Percentile (0-100)
    int64_t percentile(double p) const {
        std::vector<int64_t> sorted = samples_;
        std::sort(sorted.begin(), sorted.end());
        size_t idx = std::min((size_t)(p / 100.0 * sorted.size()), sorted.size() - 1);
        return sorted[idx];
    }

    double mean() const {
        double sum = std::accumulate(samples_.begin(), samples_.end(), 0.0);
        return sum / samples_.size();
    }

    double stddev() const {
        double m = mean();
        double sq = 0;
        for (auto v : samples_) sq += (v - m) * (v - m);
        return std::sqrt(sq / samples_.size());
    }

    int64_t min() const { return *std::min_element(samples_.begin(), samples_.end()); }
    int64_t max() const { return *std::max_element(samples_.begin(), samples_.end()); }

    void print_summary() const {
        printf("Jitter summary (%zu samples, period=%ldms):\n",
               samples_.size(), period_ns_ / 1'000'000);
        printf("  min:    %+ld ns (%+.1f μs)\n",   min(),           min() / 1000.0);
        printf("  mean:   %+.0f ns (%+.1f μs)\n",  mean(),          mean() / 1000.0);
        printf("  stddev: %.0f ns (%.1f μs)\n",     stddev(),        stddev() / 1000.0);
        printf("  p50:    %+ld ns (%+.1f μs)\n",   percentile(50),  percentile(50) / 1000.0);
        printf("  p95:    %+ld ns (%+.1f μs)\n",   percentile(95),  percentile(95) / 1000.0);
        printf("  p99:    %+ld ns (%+.1f μs)\n",   percentile(99),  percentile(99) / 1000.0);
        printf("  p99.9:  %+ld ns (%+.1f μs)\n",   percentile(99.9),percentile(99.9) / 1000.0);
        printf("  max:    %+ld ns (%+.1f μs)\n",   max(),           max() / 1000.0);
    }

    void print_histogram(int64_t bucket_us = 5) const {
        int64_t bucket_ns = bucket_us * 1000;

        // Count buckets (use map for sparse histogram)
        std::map<int64_t, long> hist;
        for (auto v : samples_) {
            int64_t bucket = (v >= 0) ? v / bucket_ns : -((-v + bucket_ns - 1) / bucket_ns);
            hist[bucket]++;
        }

        long max_count = 0;
        for (auto& [k, v] : hist) max_count = std::max(max_count, v);

        printf("\nJitter histogram (bucket = %ld μs):\n", bucket_us);
        printf("%-15s | %-40s  count\n", "range (μs)", "bar");
        printf("%s\n", std::string(70, '-').c_str());

        for (auto& [k, count] : hist) {
            int bar = (max_count > 0) ? (int)(40.0 * count / max_count) : 0;
            printf("[%+5ld, %+5ld) | %s %ld\n",
                   k * bucket_us, (k + 1) * bucket_us,
                   std::string(bar, '#').c_str(), count);
        }
    }

    // Export gnuplot format
    void export_gnuplot(const char* filename, int64_t bucket_us = 5) const {
        int64_t bucket_ns = bucket_us * 1000;
        std::map<int64_t, long> hist;
        for (auto v : samples_) {
            int64_t b = v / bucket_ns;
            hist[b]++;
        }

        std::ofstream f(filename);
        f << "# jitter_us count\n";
        for (auto& [k, count] : hist)
            f << k * bucket_us << " " << count << "\n";

        printf("Exported to %s\n", filename);
        printf("Plot: gnuplot -e \"set terminal png; set output 'jitter.png'; "
               "set xlabel 'Jitter (us)'; set ylabel 'Count'; "
               "plot '%s' with boxes\"\n", filename);
    }
};

// ─── Collect jitter trong RT thread ───────────────────────────────────────
struct CollectArg { JitterCollector* collector; int n; long period_ns; };

void* collect_jitter_thread(void* arg) {
    auto* a = static_cast<CollectArg*>(arg);

    mlockall(MCL_CURRENT | MCL_FUTURE);
    volatile char prefault[64 * 1024];
    for (size_t i = 0; i < sizeof(prefault); i += 4096) prefault[i] = 0;

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);
    int64_t start_ns = ((int64_t)next.tv_sec * 1'000'000'000LL + next.tv_nsec);

    for (int i = 0; i < a->n; ++i) {
        next.tv_nsec += a->period_ns;
        if (next.tv_nsec >= 1'000'000'000L) {
            next.tv_nsec -= 1'000'000'000L;
            next.tv_sec++;
        }

        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);

        struct timespec actual;
        clock_gettime(CLOCK_MONOTONIC, &actual);

        int64_t expected_ns = start_ns + (int64_t)(i + 1) * a->period_ns;
        int64_t actual_ns   = (int64_t)actual.tv_sec * 1'000'000'000LL + actual.tv_nsec;

        a->collector->add_sample(actual_ns, expected_ns);
    }
    return nullptr;
}

void example_rt_jitter() {
    std::cout << "\n=== RT Jitter Measurement (SCHED_FIFO, 1ms period, 5000 samples) ===\n";

    JitterCollector collector(1'000'000, 5000);
    CollectArg arg{ &collector, 5000, 1'000'000 };

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    sched_param sp{ .sched_priority = 80 };
    pthread_attr_setschedparam(&attr, &sp);

    pthread_t tid;
    int ret = pthread_create(&tid, &attr, collect_jitter_thread, &arg);
    if (ret != 0) {
        std::cerr << "RT thread failed: " << strerror(ret) << " (run with sudo)\n";
        pthread_attr_setschedpolicy(&attr, SCHED_OTHER);
        sp.sched_priority = 0;
        pthread_attr_setschedparam(&attr, &sp);
        pthread_create(&tid, &attr, collect_jitter_thread, &arg);
    }
    pthread_attr_destroy(&attr);
    pthread_join(tid, nullptr);

    collector.print_summary();
    collector.print_histogram(5);
    collector.export_gnuplot("jitter.dat", 5);
}

int main() {
    std::cout << "=== Phase1 Bài 08: Jitter Histogram ===\n";
    example_rt_jitter();
    return 0;
}
