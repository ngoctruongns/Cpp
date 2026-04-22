/**
 * PHASE 4 - Bài 09: Lock-free Ring Buffer (Real-time safe)
 *
 * Mục tiêu:
 *  - Single-producer Single-consumer (SPSC) ring buffer
 *  - Lock-free sử dụng atomic với acquire/release ordering
 *  - Real-time safe: không có dynamic allocation, không có mutex
 *  - Ứng dụng: sensor data pipeline cho real-time robotics
 *
 * Compile: g++ -std=c++17 -pthread 09_realtime_buffer.cpp -o out
 */

#include <iostream>
#include <atomic>
#include <thread>
#include <array>
#include <optional>
#include <chrono>
#include <vector>
#include <string>
#include <cassert>
#include <queue>
#include <mutex>

using namespace std::chrono_literals;

// ──────────────────────────────────────────────────────────────────────────
// SPSC Lock-free Ring Buffer
// Size must be power of 2 for efficient modulo via bit masking
// ──────────────────────────────────────────────────────────────────────────
template<typename T, size_t Capacity>
class SPSCRingBuffer {
    static_assert((Capacity & (Capacity - 1)) == 0,
                  "Capacity must be a power of 2");

    std::array<T, Capacity> buffer_{};

    // head: producer writes here (only written by producer)
    // tail: consumer reads here  (only written by consumer)
    alignas(64) std::atomic<size_t> head_{0};  // cache line isolation
    alignas(64) std::atomic<size_t> tail_{0};

    static constexpr size_t MASK = Capacity - 1;

public:
    // Push — called by producer only
    // Returns false if full
    bool push(const T& item) {
        size_t h = head_.load(std::memory_order_relaxed);
        size_t next_h = (h + 1) & MASK;

        // Check if full: next write position == tail (consumer's position)
        if (next_h == tail_.load(std::memory_order_acquire)) {
            return false;  // buffer full
        }

        buffer_[h] = item;
        head_.store(next_h, std::memory_order_release);
        return true;
    }

    bool push(T&& item) {
        size_t h = head_.load(std::memory_order_relaxed);
        size_t next_h = (h + 1) & MASK;

        if (next_h == tail_.load(std::memory_order_acquire)) {
            return false;
        }

        buffer_[h] = std::move(item);
        head_.store(next_h, std::memory_order_release);
        return true;
    }

    // Pop — called by consumer only
    // Returns nullopt if empty
    std::optional<T> pop() {
        size_t t = tail_.load(std::memory_order_relaxed);
        if (t == head_.load(std::memory_order_acquire)) {
            return std::nullopt;  // empty
        }

        T item = std::move(buffer_[t]);
        tail_.store((t + 1) & MASK, std::memory_order_release);
        return item;
    }

    // Non-destructive peek at next item
    const T* peek() const {
        size_t t = tail_.load(std::memory_order_relaxed);
        if (t == head_.load(std::memory_order_acquire)) return nullptr;
        return &buffer_[t];
    }

    bool empty() const {
        return head_.load(std::memory_order_acquire) ==
               tail_.load(std::memory_order_acquire);
    }

    // Available items (approximate — safe to call from either side)
    size_t size() const {
        size_t h = head_.load(std::memory_order_acquire);
        size_t t = tail_.load(std::memory_order_acquire);
        return (h - t + Capacity) & MASK;
    }

    static constexpr size_t capacity() { return Capacity - 1; }  // usable capacity
};

// ──────────────────────────────────────────────────────────────────────────
// Demo 1: Basic push/pop
// ──────────────────────────────────────────────────────────────────────────
struct SensorSample {
    uint64_t timestamp_ns;
    float    value;
    int      sensor_id;
};

void demo_basic() {
    std::cout << "\n=== Basic Ring Buffer ===\n";
    SPSCRingBuffer<int, 8> buf;  // capacity=7 (one slot reserved)

    std::cout << "Empty: " << buf.empty() << "\n";

    for (int i = 1; i <= 7; ++i) {
        bool ok = buf.push(i * 10);
        std::cout << "push(" << i*10 << ")=" << ok << " size=" << buf.size() << "\n";
    }

    // Full: one more push should fail
    bool ok = buf.push(999);
    std::cout << "push(999) when full: " << ok << "\n";

    while (auto v = buf.pop()) {
        std::cout << "pop → " << *v << "\n";
    }

    std::cout << "Empty after drain: " << buf.empty() << "\n";
}

// ──────────────────────────────────────────────────────────────────────────
// Demo 2: SPSC producer-consumer (different threads)
// ──────────────────────────────────────────────────────────────────────────
void demo_spsc_threads() {
    std::cout << "\n=== SPSC Producer-Consumer (lock-free) ===\n";

    SPSCRingBuffer<SensorSample, 64> ring;
    std::atomic<bool> done{false};
    std::atomic<int>  produced{0}, consumed{0}, dropped{0};

    // Producer thread (simulates LiDAR at 1kHz)
    std::thread producer([&]() {
        for (int i = 0; i < 200; ++i) {
            SensorSample s{static_cast<uint64_t>(i) * 1'000'000,
                           static_cast<float>(i) * 0.01f, 1};
            if (!ring.push(s)) {
                ++dropped;  // ring full — RT system would log this
            } else {
                ++produced;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(500));  // 2kHz
        }
        done = true;
    });

    // Consumer thread (simulates processing at ~1kHz)
    std::thread consumer([&]() {
        while (!done || !ring.empty()) {
            if (auto sample = ring.pop()) {
                ++consumed;
            } else {
                std::this_thread::yield();  // empty — yield (spin in real RT)
            }
            std::this_thread::sleep_for(std::chrono::microseconds(600));
        }
    });

    producer.join();
    consumer.join();

    std::cout << "Produced: " << produced
              << ", Consumed: " << consumed
              << ", Dropped: " << dropped << "\n";
    std::cout << "Ring empty: " << ring.empty() << "\n";
}

// ──────────────────────────────────────────────────────────────────────────
// Demo 3: Sensor pipeline with multiple buffers
// ──────────────────────────────────────────────────────────────────────────
struct ProcessedData {
    uint64_t timestamp_ns;
    double   filtered_value;
    int      source_id;
};

void demo_pipeline() {
    std::cout << "\n=== Sensor Pipeline (lock-free stages) ===\n";

    // Stage 1→2: raw sensor data
    SPSCRingBuffer<SensorSample,  32> raw_buf;
    // Stage 2→3: processed/filtered data
    SPSCRingBuffer<ProcessedData, 32> proc_buf;

    std::atomic<bool> raw_done{false}, proc_done{false};
    std::atomic<int>  raw_count{0}, proc_count{0};

    // Stage 1: Sensor driver (producer of raw data)
    std::thread sensor_driver([&]() {
        for (int i = 0; i < 50; ++i) {
            SensorSample s{static_cast<uint64_t>(i), static_cast<float>(i) + 0.5f, 2};
            while (!raw_buf.push(s)) std::this_thread::yield();  // spin until space
            ++raw_count;
            std::this_thread::sleep_for(1ms);
        }
        raw_done = true;
    });

    // Stage 2: Filter (consumer of raw → producer of processed)
    std::thread filter([&]() {
        double prev = 0.0;
        while (!raw_done || !raw_buf.empty()) {
            if (auto s = raw_buf.pop()) {
                // Simple low-pass filter: y[n] = 0.7*y[n-1] + 0.3*x[n]
                double filtered = 0.7 * prev + 0.3 * s->value;
                prev = filtered;
                ProcessedData pd{s->timestamp_ns, filtered, s->sensor_id};
                while (!proc_buf.push(pd)) std::this_thread::yield();
                ++proc_count;
            } else {
                std::this_thread::yield();
            }
        }
        proc_done = true;
    });

    // Stage 3: Logger (consumer of processed data)
    std::atomic<int> logged{0};
    std::thread logger([&]() {
        while (!proc_done || !proc_buf.empty()) {
            if (auto pd = proc_buf.pop()) {
                ++logged;
                if (logged % 10 == 0) {
                    std::cout << "  [Logger] #" << logged
                              << " filtered=" << pd->filtered_value << "\n";
                }
            } else {
                std::this_thread::yield();
            }
        }
    });

    sensor_driver.join();
    filter.join();
    logger.join();

    std::cout << "Pipeline complete: raw=" << raw_count
              << " proc=" << proc_count
              << " logged=" << logged << "\n";
}

// ──────────────────────────────────────────────────────────────────────────
// Demo 4: Compare lock-free vs mutex-based throughput
// ──────────────────────────────────────────────────────────────────────────
void demo_throughput_comparison() {
    std::cout << "\n=== Throughput: Lock-free vs Mutex ===\n";
    const int N = 100000;

    // Lock-free
    SPSCRingBuffer<int, 512> ring;
    std::atomic<int> lf_consumed{0};

    auto t0 = std::chrono::high_resolution_clock::now();
    std::thread lf_prod([&]() {
        for (int i = 0; i < N; ++i) {
            while (!ring.push(i)) std::this_thread::yield();
        }
    });
    std::thread lf_cons([&]() {
        while (lf_consumed < N) {
            if (ring.pop()) ++lf_consumed;
            else std::this_thread::yield();
        }
    });
    lf_prod.join(); lf_cons.join();
    auto t1 = std::chrono::high_resolution_clock::now();
    auto lf_ms = std::chrono::duration_cast<std::chrono::microseconds>(t1-t0).count();

    // Mutex-based
    std::queue<int> q;
    std::mutex q_mtx;
    std::atomic<int> m_consumed{0};

    auto t2 = std::chrono::high_resolution_clock::now();
    std::thread m_prod([&]() {
        for (int i = 0; i < N; ++i) {
            std::lock_guard<std::mutex> lk(q_mtx);
            q.push(i);
        }
    });
    std::thread m_cons([&]() {
        while (m_consumed < N) {
            std::lock_guard<std::mutex> lk(q_mtx);
            if (!q.empty()) { q.pop(); ++m_consumed; }
        }
    });
    m_prod.join(); m_cons.join();
    auto t3 = std::chrono::high_resolution_clock::now();
    auto m_ms = std::chrono::duration_cast<std::chrono::microseconds>(t3-t2).count();

    std::cout << "Items: " << N << "\n";
    std::cout << "Lock-free:   " << lf_ms << " μs\n";
    std::cout << "Mutex-based: " << m_ms  << " μs\n";
    if (m_ms > 0)
        std::cout << "Speedup: " << static_cast<double>(m_ms)/lf_ms << "x\n";
}

int main() {
    demo_basic();
    demo_spsc_threads();
    demo_pipeline();
    demo_throughput_comparison();
    return 0;
}
