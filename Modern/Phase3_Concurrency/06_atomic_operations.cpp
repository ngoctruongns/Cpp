/**
 * PHASE 3 - Bài 06: std::atomic Operations
 *
 * Mục tiêu:
 *  - Hiểu atomic: lock-free, không cần mutex cho simple operations
 *  - Compare-and-swap (CAS): foundation of lock-free algorithms
 *  - Memory ordering: relaxed, acquire/release, seq_cst
 *  - Ứng dụng: counter, flag, spin-lock, reference counting
 *
 * Compile: g++ -std=c++17 -pthread 06_atomic_operations.cpp -o out
 */

#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>
#include <cassert>

using namespace std::chrono_literals;

// ─── Ví dụ 1: Atomic counter vs non-atomic ───────────────────────────────
void demo_atomic_counter() {
    std::cout << "\n=== Atomic Counter ===\n";

    std::atomic<int> counter{0};
    const int N = 100000;

    auto increment = [&]() {
        for (int i = 0; i < N; ++i)
            counter.fetch_add(1, std::memory_order_relaxed);
            // OR simply: ++counter; (defaults to seq_cst)
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) threads.emplace_back(increment);
    for (auto& t : threads) t.join();

    std::cout << "counter = " << counter << " (expected " << 4 * N << ")\n";

    // fetch_add returns OLD value
    int old = counter.fetch_add(10);
    std::cout << "fetch_add(10): old=" << old << ", new=" << counter.load() << "\n";

    // fetch_sub, fetch_and, fetch_or, fetch_xor also available
    counter.fetch_sub(5);
    std::cout << "After fetch_sub(5): " << counter.load() << "\n";
}

// ─── Ví dụ 2: Compare-and-Swap (CAS) ─────────────────────────────────────
// CAS: if (*this == expected) { *this = desired; return true; }
//      else { expected = *this; return false; }
void demo_cas() {
    std::cout << "\n=== Compare-And-Swap (CAS) ===\n";

    std::atomic<int> val{10};

    // Strong CAS: no spurious failure
    int expected = 10;
    bool ok = val.compare_exchange_strong(expected, 20);
    std::cout << "CAS(10→20): ok=" << ok << " val=" << val << "\n";

    // CAS fails: expected doesn't match
    expected = 10;  // wrong expected (val is now 20)
    ok = val.compare_exchange_strong(expected, 30);
    std::cout << "CAS(10→30) fail: ok=" << ok
              << " expected updated to=" << expected  // updated to current value
              << " val=" << val << "\n";

    // Retry loop pattern (lock-free max update)
    auto update_max = [&](std::atomic<int>& m, int new_val) {
        int current = m.load(std::memory_order_relaxed);
        while (new_val > current &&
               !m.compare_exchange_weak(current, new_val,
                                        std::memory_order_release,
                                        std::memory_order_relaxed)) {
            // current was refreshed by CAS → retry
        }
    };

    std::atomic<int> max_val{0};
    std::vector<std::thread> threads;
    for (int i = 1; i <= 8; ++i) {
        threads.emplace_back([&, i]() { update_max(max_val, i * 10); });
    }
    for (auto& t : threads) t.join();
    std::cout << "Lock-free max: " << max_val << " (expected 80)\n";
}

// ─── Ví dụ 3: Memory Ordering ────────────────────────────────────────────
// relaxed:         no ordering constraints (only atomicity)
// release/acquire: synchronize-with between producer and consumer
// seq_cst:         total global order (slowest, safest)

std::atomic<bool> data_ready{false};
int produced_value = 0;  // non-atomic, protected by release/acquire

void producer_thread() {
    produced_value = 42;  // write data
    // release: ensures produced_value write is visible BEFORE data_ready=true
    data_ready.store(true, std::memory_order_release);
    std::cout << "[Producer] stored data_ready=true\n";
}

void consumer_thread() {
    // acquire: ensures we see produced_value AFTER seeing data_ready=true
    while (!data_ready.load(std::memory_order_acquire)) {
        std::this_thread::yield();
    }
    std::cout << "[Consumer] saw data_ready, value=" << produced_value
              << " (should be 42)\n";
}

void demo_memory_ordering() {
    std::cout << "\n=== Memory Ordering (acquire-release) ===\n";
    data_ready.store(false);
    produced_value = 0;

    std::thread p(producer_thread);
    std::thread c(consumer_thread);
    p.join(); c.join();
}

// ─── Ví dụ 4: Spinlock using atomic_flag ─────────────────────────────────
// atomic_flag: simplest atomic (guaranteed lock-free), only test_and_set/clear
class SpinLock {
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
public:
    void lock() {
        // Spin until we acquire the lock
        while (flag_.test_and_set(std::memory_order_acquire)) {
            // busy-wait (bad for long waits, OK for very short critical sections)
            std::this_thread::yield();
        }
    }
    void unlock() {
        flag_.clear(std::memory_order_release);
    }
};

void demo_spinlock() {
    std::cout << "\n=== SpinLock ===\n";
    SpinLock sl;
    int counter = 0;

    auto inc = [&]() {
        for (int i = 0; i < 50000; ++i) {
            sl.lock();
            ++counter;
            sl.unlock();
        }
    };

    std::thread t1(inc), t2(inc);
    t1.join(); t2.join();
    std::cout << "counter = " << counter << " (expected 100000)\n";
}

// ─── Ví dụ 5: Atomic in ROS2 context — stop flag ─────────────────────────
class ROSSensorNode {
    std::atomic<bool> running_{false};
    std::atomic<int>  message_count_{0};

public:
    void start() {
        running_.store(true);
        std::cout << "[Node] started\n";
    }

    void stop() {
        running_.store(false);
        std::cout << "[Node] stop requested\n";
    }

    // Spin loop — checks stop flag atomically
    void spin() {
        while (running_.load(std::memory_order_acquire)) {
            // Simulate processing a message
            int n = message_count_.fetch_add(1, std::memory_order_relaxed);
            if (n % 100 == 0) {
                std::cout << "[Node] processed " << n << " messages\n";
            }
            std::this_thread::sleep_for(1ms);
        }
        std::cout << "[Node] spin exited, total messages: "
                  << message_count_.load() << "\n";
    }

    int message_count() const { return message_count_.load(); }
};

void demo_ros2_stop_flag() {
    std::cout << "\n=== ROS2-style Atomic Stop Flag ===\n";
    ROSSensorNode node;
    node.start();

    std::thread spin_thread([&node]() { node.spin(); });

    std::this_thread::sleep_for(50ms);  // let it run
    node.stop();

    spin_thread.join();
}

int main() {
    demo_atomic_counter();
    demo_cas();
    demo_memory_ordering();
    demo_spinlock();
    demo_ros2_stop_flag();
    return 0;
}
