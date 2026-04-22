/**
 * PHASE 3 - Bài 03: mutex, lock_guard, scoped_lock
 *
 * Mục tiêu:
 *  - Hiểu các loại mutex: mutex, recursive_mutex, timed_mutex
 *  - lock_guard: RAII, không flexiblily
 *  - scoped_lock (C++17): lock nhiều mutex cùng lúc, tránh deadlock
 *  - unique_lock: flexible lock/unlock, cần thiết cho condition_variable
 *
 * Compile: g++ -std=c++17 -pthread 03_mutex_and_lockguard.cpp -o out
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <string>
#include <chrono>
#include <unordered_map>

using namespace std::chrono_literals;

// ─── Ví dụ 1: std::mutex + lock_guard (RAII) ─────────────────────────────
class SafeCounter {
    int       count_ = 0;
    mutable std::mutex mtx_;

public:
    void increment() {
        std::lock_guard<std::mutex> lock(mtx_);
        // lock acquired here
        ++count_;
        // lock released when lock_guard is destroyed (scope exit)
    }

    // C++17: class template argument deduction (CTAD)
    void decrement() {
        std::lock_guard lock(mtx_);  // CTAD: no need for <std::mutex>
        --count_;
    }

    int get() const {
        std::lock_guard lock(mtx_);
        return count_;
        // Note: mutable mutex needed for const method (omitted for clarity)
    }
};

void demo_lock_guard() {
    std::cout << "\n=== lock_guard Demo ===\n";
    SafeCounter c;

    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&c]() {
            for (int j = 0; j < 1000; ++j) c.increment();
        });
    }
    for (auto& t : threads) t.join();
    std::cout << "Counter = " << c.get() << " (expected 5000)\n";
}

// ─── Ví dụ 2: recursive_mutex — cùng thread lock nhiều lần ───────────────
class RecursiveProcessor {
    int                    depth_ = 0;
    int                    result_ = 0;
    std::recursive_mutex   mtx_;

public:
    int process(int n) {
        std::lock_guard lock(mtx_);  // OK to call again from same thread
        ++depth_;
        std::cout << "  process depth=" << depth_ << " n=" << n << "\n";

        if (n <= 0) {
            --depth_;
            return result_;
        }

        result_ += n;
        process(n - 1);  // recursive call → same thread locks again

        --depth_;
        return result_;
    }
};

void demo_recursive_mutex() {
    std::cout << "\n=== recursive_mutex Demo ===\n";
    RecursiveProcessor p;
    std::cout << "Result: " << p.process(5) << " (expected 15)\n";
}

// ─── Ví dụ 3: scoped_lock (C++17) — lock nhiều mutex, tránh deadlock ──────
// Classic deadlock: Thread A locks m1 then m2, Thread B locks m2 then m1
// scoped_lock uses deadlock-avoidance algorithm (std::lock internally)

std::mutex mtx_a, mtx_b;
int account_a = 1000, account_b = 2000;

// UNSAFE: order matters → could deadlock
void transfer_unsafe(int amount, bool a_to_b) {
    if (a_to_b) {
        std::lock_guard la(mtx_a);
        std::this_thread::sleep_for(1ms);  // make race more likely
        std::lock_guard lb(mtx_b);
        account_a -= amount;
        account_b += amount;
    } else {
        std::lock_guard lb(mtx_b);
        std::this_thread::sleep_for(1ms);
        std::lock_guard la(mtx_a);
        account_b -= amount;
        account_a += amount;
    }
}

// SAFE: scoped_lock locks both at once with deadlock prevention
void transfer_safe(int amount, bool a_to_b) {
    std::scoped_lock lock(mtx_a, mtx_b);  // C++17: locks both atomically
    if (a_to_b) {
        account_a -= amount;
        account_b += amount;
    } else {
        account_b -= amount;
        account_a += amount;
    }
}

void demo_scoped_lock() {
    std::cout << "\n=== scoped_lock — no deadlock ===\n";
    account_a = 1000; account_b = 2000;
    std::cout << "Before: A=" << account_a << " B=" << account_b << "\n";

    std::thread t1([]{ for (int i = 0; i < 100; ++i) transfer_safe(10, true);  });
    std::thread t2([]{ for (int i = 0; i < 100; ++i) transfer_safe(10, false); });
    t1.join(); t2.join();

    std::cout << "After:  A=" << account_a << " B=" << account_b
              << " (sum should be " << 1000+2000 << ")\n";
}

// ─── Ví dụ 4: unique_lock — flexible locking ─────────────────────────────
// unique_lock can: defer locking, try_lock, unlock/relock, move, timed_lock

std::mutex data_mutex;
std::vector<int> shared_data;

void demo_unique_lock() {
    std::cout << "\n=== unique_lock — flexible locking ===\n";

    // Deferred lock — acquire later
    std::unique_lock<std::mutex> lock(data_mutex, std::defer_lock);
    std::cout << "Lock owns? " << lock.owns_lock() << "\n";

    lock.lock();  // manually acquire
    shared_data.push_back(42);
    std::cout << "Lock owns? " << lock.owns_lock() << "\n";

    lock.unlock();  // manually release (while lock is still in scope)
    std::cout << "Lock owns after unlock? " << lock.owns_lock() << "\n";

    // Try lock (non-blocking)
    bool got_it = lock.try_lock();
    std::cout << "try_lock success? " << got_it << "\n";
    if (got_it) {
        shared_data.push_back(100);
        lock.unlock();
    }

    // Can move unique_lock (unlike lock_guard)
    auto take_lock = [](std::unique_lock<std::mutex> ul) {
        std::cout << "Moved lock owns? " << ul.owns_lock() << "\n";
        shared_data.push_back(999);
        // ul released at end of lambda
    };

    std::unique_lock<std::mutex> ul(data_mutex);
    take_lock(std::move(ul));  // transfer ownership
    std::cout << "Original lock owns after move? " << ul.owns_lock() << "\n";

    std::cout << "shared_data size: " << shared_data.size() << "\n";
}

// ─── Ví dụ 5: shared_mutex (C++17) — multiple readers, single writer ──────
// Useful for read-heavy data structures (config, sensor cache)
class SensorCache {
    std::unordered_map<std::string, double> data_;
    mutable std::shared_mutex               rw_mutex_;

public:
    // Multiple readers can read simultaneously
    double read(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(rw_mutex_);  // shared (read) lock
        auto it = data_.find(key);
        return (it != data_.end()) ? it->second : 0.0;
    }

    // Only one writer at a time, blocks all readers
    void write(const std::string& key, double value) {
        std::unique_lock<std::shared_mutex> lock(rw_mutex_);  // exclusive (write) lock
        data_[key] = value;
    }
};

void demo_shared_mutex() {
    std::cout << "\n=== shared_mutex (readers-writer lock) ===\n";
    SensorCache cache;
    cache.write("lidar/range", 3.14);
    cache.write("imu/accel",   9.81);

    std::vector<std::thread> readers;
    for (int i = 0; i < 4; ++i) {
        readers.emplace_back([&cache, i]() {
            double v = cache.read("lidar/range");
            std::cout << "  Reader " << i << " got lidar=" << v << "\n";
        });
    }

    std::thread writer([&cache]() {
        std::this_thread::sleep_for(2ms);
        cache.write("lidar/range", 5.0);
        std::cout << "  Writer updated lidar/range=5.0\n";
    });

    for (auto& t : readers) t.join();
    writer.join();

    std::cout << "Final lidar/range = " << cache.read("lidar/range") << "\n";
}

int main() {
    demo_lock_guard();
    demo_recursive_mutex();
    demo_scoped_lock();
    demo_unique_lock();
    demo_shared_mutex();
    return 0;
}
