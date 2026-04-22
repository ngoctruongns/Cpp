/**
 * PHASE 3 - Bài 02: Race Condition Demo & Fix
 *
 * Mục tiêu:
 *  - Nhận biết race condition: khi 2+ thread đọc/ghi biến không đồng bộ
 *  - Minh họa output không xác định (non-deterministic)
 *  - Fix bằng std::mutex + lock_guard
 *
 * Compile: g++ -std=c++17 -pthread 02_race_condition_demo.cpp -o out
 */

#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>

using namespace std::chrono_literals;

// ─── Ví dụ 1: Race condition trên counter ────────────────────────────────
// Unsafe counter: multiple threads increment without synchronization
long long unsafe_counter = 0;

void unsafe_increment(int iterations) {
    for (int i = 0; i < iterations; ++i) {
        // NOT atomic: load → add → store → 3 separate operations
        // Another thread may interleave between these steps!
        ++unsafe_counter;
    }
}

void demo_race_condition() {
    std::cout << "\n=== RACE CONDITION (undefined behavior!) ===\n";
    unsafe_counter = 0;
    const int N = 100000;

    std::thread t1(unsafe_increment, N);
    std::thread t2(unsafe_increment, N);
    std::thread t3(unsafe_increment, N);

    t1.join(); t2.join(); t3.join();

    std::cout << "Expected: " << 3 * N << "\n";
    std::cout << "Got:      " << unsafe_counter
              << " (likely WRONG due to race)\n";
}

// ─── Ví dụ 2: Fix bằng mutex ─────────────────────────────────────────────
long long safe_counter = 0;
std::mutex counter_mutex;

void safe_increment(int iterations) {
    for (int i = 0; i < iterations; ++i) {
        std::lock_guard<std::mutex> lock(counter_mutex);  // RAII lock
        ++safe_counter;
        // lock released at end of scope
    }
}

void demo_mutex_fix() {
    std::cout << "\n=== FIXED WITH mutex ===\n";
    safe_counter = 0;
    const int N = 100000;

    std::thread t1(safe_increment, N);
    std::thread t2(safe_increment, N);
    std::thread t3(safe_increment, N);

    t1.join(); t2.join(); t3.join();

    std::cout << "Expected: " << 3 * N << "\n";
    std::cout << "Got:      " << safe_counter << " (CORRECT)\n";
}

// ─── Ví dụ 3: Data race on shared data structure ─────────────────────────
std::vector<int> shared_log;
std::mutex log_mutex;

void produce_log_unsafe(int producer_id, int count) {
    for (int i = 0; i < count; ++i) {
        // UNSAFE: vector realloc or iterator invalidation → crash
        shared_log.push_back(producer_id * 1000 + i);
    }
}

void produce_log_safe(int producer_id, int count) {
    for (int i = 0; i < count; ++i) {
        std::lock_guard<std::mutex> lk(log_mutex);
        shared_log.push_back(producer_id * 1000 + i);
    }
}

void demo_vector_race() {
    std::cout << "\n=== Vector Race → Fixed with mutex ===\n";
    shared_log.clear();

    // SAFE version
    std::thread t1(produce_log_safe, 1, 500);
    std::thread t2(produce_log_safe, 2, 500);
    t1.join(); t2.join();

    std::cout << "Total log entries: " << shared_log.size()
              << " (expected 1000)\n";
    std::cout << "Sample: ";
    for (size_t i = 0; i < std::min(shared_log.size(), size_t(5)); ++i)
        std::cout << shared_log[i] << " ";
    std::cout << "...\n";
}

// ─── Ví dụ 4: Race condition trên cout (output interleaving) ─────────────
std::mutex cout_mutex;

void print_safe(const std::string& prefix, int count) {
    for (int i = 0; i < count; ++i) {
        std::lock_guard<std::mutex> lk(cout_mutex);
        std::cout << prefix << " #" << i << "\n";
    }
}

void print_unsafe(const std::string& prefix, int count) {
    for (int i = 0; i < count; ++i) {
        // RACE: two threads may write simultaneously → garbled output
        std::cout << prefix << " #" << i << "\n";
    }
}

void demo_cout_race() {
    std::cout << "\n=== cout safe printing ===\n";

    // Safe version (output is clean, not interleaved)
    std::thread t1(print_safe, "[T1]", 3);
    std::thread t2(print_safe, "[T2]", 3);
    t1.join(); t2.join();
}

// ─── Ví dụ 5: Check-then-act race (TOCTOU) ──────────────────────────────
// Classic bug: check condition, then act — another thread may change state
// between check and act
class BankAccount {
    double balance_ = 1000.0;
    std::mutex mtx_;
public:
    // UNSAFE: check and withdraw are separate operations
    bool withdraw_unsafe(double amount) {
        if (balance_ >= amount) {       // check
            std::this_thread::sleep_for(1ms);  // simulate processing (race window)
            balance_ -= amount;         // act — another thread may have withdrawn!
            return true;
        }
        return false;
    }

    // SAFE: entire check-then-act is atomic under mutex
    bool withdraw_safe(double amount) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (balance_ >= amount) {
            balance_ -= amount;
            return true;
        }
        return false;
    }

    double balance() {
        std::lock_guard<std::mutex> lock(mtx_);
        return balance_;
    }
};

void demo_toctou() {
    std::cout << "\n=== TOCTOU fix with atomic check-then-act ===\n";

    BankAccount acct;
    std::atomic<int> success_count{0};

    auto try_withdraw = [&]() {
        for (int i = 0; i < 5; ++i) {
            if (acct.withdraw_safe(300.0)) {
                ++success_count;
            }
        }
    };

    std::thread t1(try_withdraw);
    std::thread t2(try_withdraw);
    std::thread t3(try_withdraw);
    t1.join(); t2.join(); t3.join();

    std::cout << "Successful withdrawals of 300: " << success_count.load() << "\n";
    std::cout << "Remaining balance: " << acct.balance()
              << " (started at 1000.0)\n";
    std::cout << "Math check: 1000 - " << success_count.load()
              << "×300 = " << (1000.0 - success_count.load() * 300.0) << "\n";
}

int main() {
    demo_race_condition();
    demo_mutex_fix();
    demo_vector_race();
    demo_cout_race();
    demo_toctou();
    return 0;
}
