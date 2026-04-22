/**
 * PHASE 3 - Bài 09: Deadlock Prevention
 *
 * Mục tiêu:
 *  - Hiểu các nguyên nhân gây deadlock
 *  - std::lock() — lock nhiều mutex an toàn
 *  - scoped_lock (C++17) — lock nhiều mutex RAII
 *  - Lock ordering — quy ước thứ tự
 *  - std::try_lock + timeout — tránh chờ vô hạn
 *
 * Compile: g++ -std=c++17 -pthread 09_deadlock_prevention.cpp -o out
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <string>
#include <atomic>

using namespace std::chrono_literals;

// ─── Ví dụ 1: Classic deadlock ── commented out to avoid freeze ──────────
// Thread A: lock(ma) → lock(mb)
// Thread B: lock(mb) → lock(ma)
// → Both wait forever
void demo_deadlock_concept() {
    std::cout << "\n=== Deadlock Concept (NOT run — would hang) ===\n";
    std::cout << "Thread A acquires mutex_a, waits for mutex_b\n";
    std::cout << "Thread B acquires mutex_b, waits for mutex_a\n";
    std::cout << "→ circular wait → DEADLOCK\n";
    std::cout << "Conditions for deadlock (Coffman):\n";
    std::cout << "  1. Mutual exclusion\n";
    std::cout << "  2. Hold and wait\n";
    std::cout << "  3. No preemption\n";
    std::cout << "  4. Circular wait\n";
}

// ─── Ví dụ 2: Fix with std::lock() ──────────────────────────────────────
std::mutex mtx_bank_a, mtx_bank_b;
int bank_a = 1000, bank_b = 2000;

void transfer_with_std_lock(int amount, bool a_to_b) {
    // std::lock acquires ALL mutexes atomically using deadlock-avoidance
    std::lock(mtx_bank_a, mtx_bank_b);

    // adopt_lock: tell lock_guard we already own the mutex
    std::lock_guard ga(mtx_bank_a, std::adopt_lock);
    std::lock_guard gb(mtx_bank_b, std::adopt_lock);

    if (a_to_b) {
        bank_a -= amount;
        bank_b += amount;
    } else {
        bank_b -= amount;
        bank_a += amount;
    }
}

void demo_std_lock_fix() {
    std::cout << "\n=== Fix with std::lock() + adopt_lock ===\n";
    bank_a = 1000; bank_b = 2000;

    std::thread t1([]{ for (int i=0;i<50;++i) transfer_with_std_lock(10, true);  });
    std::thread t2([]{ for (int i=0;i<50;++i) transfer_with_std_lock(10, false); });
    t1.join(); t2.join();

    std::cout << "A=" << bank_a << " B=" << bank_b
              << " sum=" << bank_a+bank_b << " (expected 3000)\n";
}

// ─── Ví dụ 3: scoped_lock (C++17) — cleanest solution ────────────────────
std::mutex mtx_x, mtx_y;
int val_x = 100, val_y = 200;

void transfer_scoped(int amount, bool x_to_y) {
    std::scoped_lock lock(mtx_x, mtx_y);  // C++17: locks both atomically, RAII
    if (x_to_y) { val_x -= amount; val_y += amount; }
    else         { val_y -= amount; val_x += amount; }
}

void demo_scoped_lock() {
    std::cout << "\n=== scoped_lock (C++17) — simplest fix ===\n";
    val_x = 100; val_y = 200;

    std::thread t1([]{ for (int i=0;i<50;++i) transfer_scoped(5, true);  });
    std::thread t2([]{ for (int i=0;i<50;++i) transfer_scoped(5, false); });
    t1.join(); t2.join();

    std::cout << "X=" << val_x << " Y=" << val_y
              << " sum=" << val_x+val_y << " (expected 300)\n";
}

// ─── Ví dụ 4: Lock ordering — prevent circular wait ──────────────────────
// Strategy: always acquire mutexes in the same global order
// (e.g., by address, by ID, by name)

class Node {
public:
    int         id;
    std::string name;
    int         credits = 100;
    std::mutex  mtx;

    explicit Node(int i, const std::string& n) : id(i), name(n) {}
};

// Lock ordering: always lock lower id first
void transfer_ordered(Node& from, Node& to, int amount) {
    // Determine lock order by id
    Node* first  = (from.id < to.id) ? &from : &to;
    Node* second = (from.id < to.id) ? &to   : &from;

    std::lock_guard l1(first->mtx);
    std::lock_guard l2(second->mtx);

    from.credits -= amount;
    to.credits   += amount;
}

void demo_lock_ordering() {
    std::cout << "\n=== Lock Ordering Strategy ===\n";

    Node alice(1, "Alice");
    Node bob(2, "Bob");

    std::thread t1([&]() {
        for (int i = 0; i < 100; ++i)
            transfer_ordered(alice, bob, 1);
    });
    std::thread t2([&]() {
        for (int i = 0; i < 100; ++i)
            transfer_ordered(bob, alice, 1);  // reverse direction, same lock order!
    });
    t1.join(); t2.join();

    std::cout << "Alice: " << alice.credits
              << ", Bob: " << bob.credits
              << ", sum=" << alice.credits + bob.credits << " (expected 200)\n";
}

// ─── Ví dụ 5: try_lock — non-blocking, avoid waiting forever ─────────────
std::mutex resource_a, resource_b;

void demo_try_lock() {
    std::cout << "\n=== try_lock — non-blocking acquisition ===\n";
    std::atomic<int> success_count{0}, fail_count{0};

    auto try_both = [&](int thread_id) {
        for (int attempt = 0; attempt < 200; ++attempt) {
            // Try to lock both without blocking
            if (std::unique_lock la(resource_a, std::try_to_lock);
                la.owns_lock()) {
                if (std::unique_lock lb(resource_b, std::try_to_lock);
                    lb.owns_lock()) {
                    // Got both locks!
                    ++success_count;
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                } else {
                    ++fail_count;  // couldn't get b, la released at scope exit
                }
            } else {
                ++fail_count;  // couldn't get a
            }
            // Yield to let other thread progress
            std::this_thread::yield();
        }
    };

    std::thread t1(try_both, 1);
    std::thread t2(try_both, 2);
    t1.join(); t2.join();

    std::cout << "Success: " << success_count
              << ", Failed/retried: " << fail_count << "\n";
    std::cout << "(No deadlock — try_lock never waits)\n";
}

// ─── Ví dụ 6: timed_mutex — avoid deadlock with timeout ─────────────────
void demo_timed_mutex() {
    std::cout << "\n=== timed_mutex — lock with timeout ===\n";

    std::timed_mutex tm;
    std::atomic<bool> holder_done{false};

    // Thread that holds the lock for a while
    std::thread holder([&]() {
        std::lock_guard lock(tm);
        std::cout << "  [holder] acquired lock, sleeping...\n";
        std::this_thread::sleep_for(100ms);
        holder_done = true;
        std::cout << "  [holder] released lock\n";
    });

    std::this_thread::sleep_for(10ms);  // let holder acquire first

    // Thread that tries with timeout
    std::thread requester([&]() {
        std::cout << "  [requester] trying to acquire with 50ms timeout\n";
        if (tm.try_lock_for(50ms)) {
            std::cout << "  [requester] got lock!\n";
            tm.unlock();
        } else {
            std::cout << "  [requester] timed out — will retry or give up\n";
        }

        // Try again with longer timeout
        std::cout << "  [requester] retrying with 200ms timeout\n";
        if (tm.try_lock_for(200ms)) {
            std::cout << "  [requester] got lock on retry!\n";
            tm.unlock();
        }
    });

    holder.join();
    requester.join();
}

int main() {
    demo_deadlock_concept();
    demo_std_lock_fix();
    demo_scoped_lock();
    demo_lock_ordering();
    demo_try_lock();
    demo_timed_mutex();
    return 0;
}
