/**
 * PHASE 1 - Bài 09: Software Watchdog
 *
 * Mục tiêu:
 *  - Implement watchdog: monitor thread kiểm tra thread khác còn sống
 *  - Dùng atomic timestamp để thread feed watchdog
 *  - Timeout handler: emergency stop
 *  - RAII watchdog guard để tự động feed
 *
 * Compile: g++ -std=c++17 -O2 09_watchdog.cpp -o out -lrt -lpthread
 */

#include <iostream>
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <functional>
#include <unistd.h>

// ─── SoftwareWatchdog ─────────────────────────────────────────────────────
class SoftwareWatchdog {
    std::atomic<int64_t>   last_feed_ns_{ 0 };
    const int64_t          timeout_ns_;
    std::function<void()>  on_timeout_;
    std::atomic<bool>      armed_{ false };

    static int64_t mono_ns() {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (int64_t)ts.tv_sec * 1'000'000'000LL + ts.tv_nsec;
    }

public:
    explicit SoftwareWatchdog(int64_t timeout_ns, std::function<void()> cb = {})
        : timeout_ns_(timeout_ns), on_timeout_(std::move(cb)) {}

    // Arm watchdog (call before starting monitored thread)
    void arm() {
        last_feed_ns_.store(mono_ns(), std::memory_order_relaxed);
        armed_.store(true, std::memory_order_release);
    }

    // Disarm (call after monitored thread exits cleanly)
    void disarm() {
        armed_.store(false, std::memory_order_release);
    }

    // Feed — monitored thread calls this each iteration
    void feed() {
        last_feed_ns_.store(mono_ns(), std::memory_order_relaxed);
    }

    // Check — monitor thread calls this periodically
    bool is_alive() const {
        if (!armed_.load(std::memory_order_acquire)) return true;
        int64_t now  = mono_ns();
        int64_t last = last_feed_ns_.load(std::memory_order_relaxed);
        return (now - last) < timeout_ns_;
    }

    void check_and_fire() {
        if (!is_alive()) {
            if (on_timeout_) on_timeout_();
        }
    }

    int64_t time_since_feed_us() const {
        return (mono_ns() - last_feed_ns_.load(std::memory_order_relaxed)) / 1000;
    }
};

// ─── RAII WatchdogFeeder ─────────────────────────────────────────────────
// Đặt ở đầu mỗi iteration của RT loop — tự động feed khi xây dựng,
// hoặc gọi feed() thủ công
class WatchdogFeeder {
    SoftwareWatchdog& wd_;
public:
    explicit WatchdogFeeder(SoftwareWatchdog& wd) : wd_(wd) { wd_.feed(); }
};

// ─── Ví dụ 1: Watchdog bình thường ───────────────────────────────────────
std::atomic<bool> g_running{ true };
std::atomic<bool> g_emergency_stop{ false };

SoftwareWatchdog g_wd(
    200'000'000LL,  // 200ms timeout
    []() {
        printf("[WATCHDOG] Control thread hung! Emergency stop!\n");
        g_emergency_stop.store(true);
        g_running.store(false);
    }
);

void* control_thread(void* arg) {
    bool* simulate_hang = static_cast<bool*>(arg);

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    int iter = 0;
    while (g_running) {
        // Feed watchdog mỗi iteration
        g_wd.feed();
        iter++;

        // Simulate hang sau iteration 10
        if (*simulate_hang && iter == 10) {
            printf("[Control] Simulating hang at iter %d...\n", iter);
            struct timespec hang{ .tv_sec = 1, .tv_nsec = 0 };
            nanosleep(&hang, nullptr);  // 1s hang → watchdog sẽ trigger
        }

        printf("[Control] iter %d running\n", iter);

        next.tv_nsec += 50'000'000;  // 50ms period
        if (next.tv_nsec >= 1'000'000'000L) {
            next.tv_nsec -= 1'000'000'000L;
            next.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);
    }
    return nullptr;
}

void* watchdog_monitor_thread(void*) {
    while (g_running || !g_emergency_stop) {
        g_wd.check_and_fire();

        if (!g_wd.is_alive()) {
            printf("[Monitor] Last feed: %ld μs ago\n", g_wd.time_since_feed_us());
        }

        struct timespec sleep{ .tv_sec = 0, .tv_nsec = 50'000'000 };  // check every 50ms
        nanosleep(&sleep, nullptr);

        if (g_emergency_stop) break;
    }
    printf("[Monitor] Watchdog monitor exiting\n");
    return nullptr;
}

void example_watchdog_normal() {
    std::cout << "\n=== Ví dụ 1: Watchdog — normal operation (no hang) ===\n";

    g_running        = true;
    g_emergency_stop = false;
    g_wd.arm();

    bool simulate_hang = false;

    pthread_t control, monitor;
    pthread_create(&control, nullptr, control_thread,        &simulate_hang);
    pthread_create(&monitor, nullptr, watchdog_monitor_thread, nullptr);

    // Let it run for 500ms then stop cleanly
    struct timespec sleep{ .tv_sec = 0, .tv_nsec = 500'000'000 };
    nanosleep(&sleep, nullptr);

    g_running = false;
    g_wd.disarm();

    pthread_join(control, nullptr);
    pthread_join(monitor, nullptr);
    std::cout << "Normal operation: OK\n";
}

void example_watchdog_hang() {
    std::cout << "\n=== Ví dụ 2: Watchdog — simulate hang ===\n";

    g_running        = true;
    g_emergency_stop = false;
    g_wd.arm();

    bool simulate_hang = true;

    pthread_t control, monitor;
    pthread_create(&control, nullptr, control_thread,         &simulate_hang);
    pthread_create(&monitor, nullptr, watchdog_monitor_thread, nullptr);

    pthread_join(control, nullptr);
    pthread_join(monitor, nullptr);
    std::cout << "Hang detected and handled: OK\n";
}

// ─── Ví dụ 3: Watchdog với RAII Feeder ───────────────────────────────────
void example_raii_feeder() {
    std::cout << "\n=== Ví dụ 3: RAII WatchdogFeeder ===\n";

    SoftwareWatchdog wd(100'000'000LL, []() {
        printf("[WD] RAII example: timeout!\n");
    });
    wd.arm();

    for (int i = 0; i < 5; ++i) {
        WatchdogFeeder feeder(wd);  // auto feed

        printf("[Loop] iter %d — WD fed automatically\n", i);

        struct timespec sleep{ .tv_sec = 0, .tv_nsec = 20'000'000 };
        nanosleep(&sleep, nullptr);
    }

    wd.disarm();
    printf("RAII feeder demo done\n");
}

int main() {
    std::cout << "=== Phase1 Bài 09: Software Watchdog ===\n";

    example_watchdog_normal();
    example_watchdog_hang();
    example_raii_feeder();

    return 0;
}
