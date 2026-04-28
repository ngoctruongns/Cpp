/**
 * PHASE 1 - Bài 01: Thread Priority & Scheduling Policy
 *
 * Mục tiêu:
 *  - Tạo thread với SCHED_FIFO và SCHED_RR
 *  - Set priority khác nhau cho các thread
 *  - Quan sát thứ tự chạy theo priority
 *  - Thay đổi scheduling policy lúc runtime
 *
 * Compile: g++ -std=c++17 -O2 01_thread_priority.cpp -o out -lpthread
 * Run:     sudo ./out   (cần quyền root để set RT priority)
 */

#include <iostream>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <cstring>
#include <cassert>
#include <cerrno>

// ─── Ví dụ 1: Kiểm tra scheduling policy hiện tại ─────────────────────────
void example_current_policy() {
    std::cout << "\n=== Ví dụ 1: Current scheduling policy ===\n";

    int policy = sched_getscheduler(0);  // 0 = current process
    const char* name = "UNKNOWN";
    if      (policy == SCHED_OTHER)  name = "SCHED_OTHER (normal)";
    else if (policy == SCHED_FIFO)   name = "SCHED_FIFO  (RT)";
    else if (policy == SCHED_RR)     name = "SCHED_RR    (RT)";
    else if (policy == SCHED_BATCH)  name = "SCHED_BATCH";
    else if (policy == SCHED_IDLE)   name = "SCHED_IDLE";

    std::cout << "Current policy: " << name << "\n";

    // Priority range cho mỗi policy
    std::cout << "SCHED_OTHER priority range: "
              << sched_get_priority_min(SCHED_OTHER) << " - "
              << sched_get_priority_max(SCHED_OTHER) << "\n";
    std::cout << "SCHED_FIFO  priority range: "
              << sched_get_priority_min(SCHED_FIFO) << " - "
              << sched_get_priority_max(SCHED_FIFO) << "\n";
    std::cout << "SCHED_RR    priority range: "
              << sched_get_priority_min(SCHED_RR) << " - "
              << sched_get_priority_max(SCHED_RR) << "\n";
}

// ─── Ví dụ 2: Tạo thread với SCHED_FIFO ──────────────────────────────────
struct ThreadArg {
    int id;
    int priority;
};

void* fifo_thread_fn(void* arg) {
    auto* a = static_cast<ThreadArg*>(arg);

    // In policy và priority hiện tại của thread này
    int policy;
    sched_param sp;
    pthread_getschedparam(pthread_self(), &policy, &sp);

    std::cout << "[Thread " << a->id << "] "
              << "policy=" << (policy == SCHED_FIFO ? "FIFO" : "OTHER")
              << " priority=" << sp.sched_priority << "\n";

    // Simulate work
    volatile long x = 0;
    for (long i = 0; i < 50'000'000L; ++i) x += i;
    std::cout << "[Thread " << a->id << "] done (x=" << x % 100 << ")\n";
    return nullptr;
}

void example_create_rt_thread() {
    std::cout << "\n=== Ví dụ 2: Create SCHED_FIFO threads ===\n";

    pthread_t threads[3];
    ThreadArg args[3] = { {1, 60}, {2, 70}, {3, 80} };

    for (int i = 0; i < 3; ++i) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&attr, SCHED_FIFO);

        sched_param sp{ .sched_priority = args[i].priority };
        pthread_attr_setschedparam(&attr, &sp);

        int ret = pthread_create(&threads[i], &attr, fifo_thread_fn, &args[i]);
        if (ret != 0) {
            std::cerr << "[Thread " << args[i].id << "] create failed: "
                      << strerror(ret) << " (run with sudo?)\n";
        }
        pthread_attr_destroy(&attr);
    }

    for (auto& t : threads) pthread_join(t, nullptr);
}

// ─── Ví dụ 3: Thay đổi policy/priority lúc runtime ───────────────────────
void example_change_policy_runtime() {
    std::cout << "\n=== Ví dụ 3: Change policy at runtime ===\n";

    // Đổi process hiện tại sang SCHED_FIFO p=50
    sched_param sp{ .sched_priority = 50 };
    int ret = sched_setscheduler(0, SCHED_FIFO, &sp);
    if (ret != 0) {
        std::cerr << "sched_setscheduler failed: " << strerror(errno)
                  << " (run with sudo)\n";
        return;
    }

    int policy = sched_getscheduler(0);
    sched_getparam(0, &sp);
    std::cout << "After change: policy="
              << (policy == SCHED_FIFO ? "SCHED_FIFO" : "other")
              << " priority=" << sp.sched_priority << "\n";

    // Đổi lại SCHED_OTHER
    sp.sched_priority = 0;
    sched_setscheduler(0, SCHED_OTHER, &sp);
    std::cout << "Restored to SCHED_OTHER\n";
}

// ─── Ví dụ 4: SCHED_FIFO vs SCHED_RR so sánh ─────────────────────────────
void example_fifo_vs_rr() {
    std::cout << "\n=== Ví dụ 4: SCHED_FIFO vs SCHED_RR ===\n";
    std::cout <<
        "SCHED_FIFO:\n"
        "  - Thread chạy cho đến khi tự yield hoặc bị preempt bởi thread priority cao hơn\n"
        "  - KHÔNG có time slice — nếu không yield, thread chiếm CPU mãi\n"
        "  - Dùng: control loop, sensor reading (cần low-latency, ít context switch)\n\n"
        "SCHED_RR (Round Robin):\n"
        "  - Giống FIFO nhưng có time slice (thường 100ms)\n"
        "  - Threads cùng priority chia sẻ CPU theo round-robin\n"
        "  - Dùng: khi có nhiều threads cùng priority cần share CPU fairly\n\n"
        "Robot priority convention:\n"
        "  80 — control loop (SCHED_FIFO)\n"
        "  70 — sensor reader (SCHED_FIFO)\n"
        "  60 — comm/UART bridge (SCHED_FIFO)\n"
        "  30 — state machine (SCHED_OTHER ok)\n"
        "   1 — watchdog monitor\n";
}

int main() {
    std::cout << "=== Phase1 Bài 01: Thread Priority & Scheduling ===\n";
    std::cout << "(Một số ví dụ cần sudo để set RT priority)\n";

    example_current_policy();
    example_create_rt_thread();
    example_change_policy_runtime();
    example_fifo_vs_rr();

    return 0;
}
