# Latency Measurement & Profiling

> **Bài tập liên quan:** Phase1 / 07, 08

---

## 1. Đo Wakeup Latency trong Code

```cpp
struct LatencyStats {
    long min_ns  = LONG_MAX;
    long max_ns  = 0;
    long sum_ns  = 0;
    long count   = 0;
    std::vector<long> samples;

    void record(long ns) {
        min_ns  = std::min(min_ns, ns);
        max_ns  = std::max(max_ns, ns);
        sum_ns += ns;
        count++;
        samples.push_back(ns);
    }

    double mean_us() const { return (double)sum_ns / count / 1000.0; }
    double max_us()  const { return max_ns / 1000.0; }
    double min_us()  const { return min_ns / 1000.0; }

    double percentile_us(double p) const {
        if (samples.empty()) return 0;
        auto sorted = samples;
        std::sort(sorted.begin(), sorted.end());
        size_t idx = (size_t)(p / 100.0 * sorted.size());
        return sorted[std::min(idx, sorted.size()-1)] / 1000.0;
    }
};

// Đo latency của periodic wakeup
void measure_timer_latency(int periods = 10000) {
    const long PERIOD_NS = 1'000'000;  // 1ms
    LatencyStats stats;

    struct timespec next, actual;
    clock_gettime(CLOCK_MONOTONIC, &next);

    for (int i = 0; i < periods; ++i) {
        // Tính thời điểm wakeup mong muốn
        next.tv_nsec += PERIOD_NS;
        if (next.tv_nsec >= 1'000'000'000L) {
            next.tv_nsec -= 1'000'000'000L;
            next.tv_sec++;
        }

        // Sleep đến thời điểm tuyệt đối
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);

        // Đo thực tế wakeup lúc nào
        clock_gettime(CLOCK_MONOTONIC, &actual);

        // Latency = thực tế - mong muốn
        long latency_ns = (actual.tv_sec - next.tv_sec) * 1'000'000'000L
                        + (actual.tv_nsec - next.tv_nsec);
        if (latency_ns > 0) stats.record(latency_ns);
    }

    printf("Latency: min=%.1fμs mean=%.1fμs p95=%.1fμs p99=%.1fμs max=%.1fμs\n",
           stats.min_us(), stats.mean_us(),
           stats.percentile_us(95), stats.percentile_us(99),
           stats.max_us());
}
```

---

## 2. Histogram của Jitter

```cpp
class JitterHistogram {
    std::array<long, 100> buckets{};   // 0-99μs buckets
    long overflow_count{0};            // > 99μs
    long period_ns_;

public:
    explicit JitterHistogram(long period_ns) : period_ns_(period_ns) {}

    void record(long actual_ns, long expected_ns) {
        long jitter_us = std::abs(actual_ns - expected_ns) / 1000;
        if (jitter_us < (long)buckets.size())
            buckets[jitter_us]++;
        else
            overflow_count++;
    }

    void print() const {
        printf("\n=== Jitter Histogram ===\n");
        for (size_t i = 0; i < buckets.size(); ++i) {
            if (buckets[i] == 0) continue;
            printf("%3zu μs: %ld\n", i, buckets[i]);
        }
        if (overflow_count > 0)
            printf(">99 μs: %ld  ← OVERRUNS!\n", overflow_count);
    }
};
```

---

## 3. `cyclictest` — Công cụ đo latency chuẩn

```bash
# Cài đặt
sudo apt install rt-tests

# Chạy cơ bản (1ms timer, 10000 iterations)
sudo cyclictest --mlockall --smp --priority=80 --interval=1000 --loops=10000

# Chạy với histogram
sudo cyclictest -m -n -p80 -i1000 -l100000 -h200 --histfile=hist.txt

# Giải thích output:
# T: 0  (  P:80) I:1000 C: 10000 Min:      5 Act:    8 Avg:    9 Max:     47
# T = thread index
# P = priority
# I = interval (μs)
# C = count
# Min/Avg/Max = latency μs

# Plot histogram (cần gnuplot)
sudo apt install gnuplot
gnuplot -e "set terminal png; set output 'latency.png'; \
  plot 'hist.txt' using 1:2 with boxes"
```

---

## 4. Typical Latency Values

```
Standard kernel:    min=5μs  avg=20μs  max=1000μs  (không ổn định)
Lowlatency kernel:  min=5μs  avg=15μs  max=200μs
PREEMPT_RT kernel:  min=5μs  avg=12μs  max=50-100μs
PREEMPT_RT + isolcpus: min=3μs  avg=8μs  max=20-40μs
```

**Cho control loop:**
- 100Hz (10ms period): standard kernel đủ
- 500Hz (2ms period): cần lowlatency
- 1kHz (1ms period): cần PREEMPT_RT
- >4kHz (<250μs period): cần PREEMPT_RT + isolcpus + RT setup đầy đủ

---

## 5. `perf` — Performance Profiling

```bash
# Record CPU profile
sudo perf record -g ./rt_program

# Report (interactive)
sudo perf report

# Stat (summary)
sudo perf stat ./rt_program

# Measure cache miss
sudo perf stat -e cache-misses,cache-references ./rt_program

# Flamegraph
sudo perf record -F 99 -g ./rt_program
sudo perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg
```

---

## 6. `hackbench` — Stress Test Scheduler

Tạo nhiều task context switch → test worst-case latency khi hệ thống bận.

```bash
# Chạy cyclictest SONG SONG với hackbench để test worst case
sudo cyclictest -m -p80 -i1000 -l100000 &
sudo hackbench -l 1000   # tạo scheduler stress
wait
```

---

## 7. Latency Sources — Debug checklist

Khi thấy max latency cao bất thường:

```bash
# 1. Kiểm tra RT throttling
cat /proc/sys/kernel/sched_rt_runtime_us   # phải là -1

# 2. Kiểm tra IRQ affinity (không nên chạy trên CPU isolated)
cat /proc/irq/*/smp_affinity_list

# 3. Kiểm tra CPU frequency scaling
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
# Nếu là "powersave" → đổi sang "performance":
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# 4. Kiểm tra CPU idle states
cat /sys/devices/system/cpu/cpu*/cpuidle/state*/disable
# Disable C-states sâu:
sudo cpupower idle-set -D 1   # disable C-states sâu hơn C1

# 5. Check for SMI (System Management Interrupts)
sudo apt install msr-tools
sudo modprobe msr
sudo rdmsr 0x34  # SMI counter (tăng = có SMI → latency spike)
```

---

## 8. Tóm tắt

```
Đo latency trong code:  clock_gettime(CLOCK_MONOTONIC) + diff
Jitter histogram:       buckets theo μs, xem phân phối
cyclictest:             công cụ chuẩn, luôn chạy với sudo
Stress test:            cyclictest + hackbench song song
CPU freq:               scaling_governor = "performance"
C-states:               disable để giảm max latency
```
