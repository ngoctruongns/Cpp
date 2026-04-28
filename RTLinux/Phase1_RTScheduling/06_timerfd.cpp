/**
 * PHASE 1 - Bài 06: timerfd — Timer như File Descriptor
 *
 * Mục tiêu:
 *  - Tạo timer với timerfd_create / timerfd_settime
 *  - Đọc timer expiry bằng read() (blocking hoặc non-blocking)
 *  - Phát hiện overrun (bỏ lỡ timer)
 *  - Kết hợp timerfd với epoll cho event-driven control loop
 *
 * Compile: g++ -std=c++17 -O2 06_timerfd.cpp -o out -lrt
 */

#include <iostream>
#include <sys/timerfd.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <time.h>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <atomic>

std::atomic<bool> g_running{ true };

// ─── Helper ───────────────────────────────────────────────────────────────
inline int64_t now_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1'000'000'000LL + ts.tv_nsec;
}

// ─── Ví dụ 1: Tạo và đọc timerfd ─────────────────────────────────────────
void example_basic_timerfd() {
    std::cout << "\n=== Ví dụ 1: Basic timerfd ===\n";

    // Tạo timer (CLOCK_MONOTONIC, không block khi close)
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (tfd < 0) { perror("timerfd_create"); return; }

    // Set timer: bắt đầu sau 10ms, lặp mỗi 10ms
    struct itimerspec spec{};
    spec.it_value.tv_nsec    = 10'000'000;   // first expiry: 10ms
    spec.it_interval.tv_nsec = 10'000'000;   // period: 10ms

    timerfd_settime(tfd, 0, &spec, nullptr);  // 0 = relative time
    // TFD_TIMER_ABSTIME = absolute (like TIMER_ABSTIME)

    std::cout << "Timer running (10ms period, reading 5 expirations)...\n";

    for (int i = 0; i < 5; ++i) {
        uint64_t expirations;
        // Blocking read — trả về số lần timer đã fire
        ssize_t n = read(tfd, &expirations, sizeof(expirations));
        if (n == sizeof(expirations)) {
            if (expirations > 1)
                printf("  iter %d: %lu expirations! (OVERRUN)\n", i, expirations);
            else
                printf("  iter %d: 1 expiration (ok)\n", i);
        }
    }

    // Dừng timer
    struct itimerspec stop{};
    timerfd_settime(tfd, 0, &stop, nullptr);

    close(tfd);
}

// ─── Ví dụ 2: Phát hiện overrun ──────────────────────────────────────────
void example_overrun_detection() {
    std::cout << "\n=== Ví dụ 2: Overrun detection ===\n";

    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);

    struct itimerspec spec{};
    spec.it_value.tv_nsec    = 1'000'000;   // 1ms
    spec.it_interval.tv_nsec = 1'000'000;   // 1ms period

    timerfd_settime(tfd, 0, &spec, nullptr);

    // Simulate slow handler — miss some timer expirations
    for (int i = 0; i < 5; ++i) {
        if (i == 2) {
            // Simulate slow processing (3ms > 1ms period → 2 overruns)
            struct timespec slow{ .tv_sec = 0, .tv_nsec = 3'000'000 };
            nanosleep(&slow, nullptr);
        }

        uint64_t expirations;
        read(tfd, &expirations, sizeof(expirations));

        if (expirations > 1)
            printf("  iter %d: OVERRUN! missed %lu periods\n", i, expirations - 1);
        else
            printf("  iter %d: on time\n", i);
    }

    close(tfd);
}

// ─── Ví dụ 3: timerfd + epoll — event loop ────────────────────────────────
void example_timerfd_epoll() {
    std::cout << "\n=== Ví dụ 3: timerfd + epoll event loop ===\n";

    int epfd = epoll_create1(EPOLL_CLOEXEC);
    int tfd  = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);

    // 5ms periodic timer
    struct itimerspec spec{};
    spec.it_value.tv_nsec    = 5'000'000;
    spec.it_interval.tv_nsec = 5'000'000;
    timerfd_settime(tfd, 0, &spec, nullptr);

    // Đăng ký tfd vào epoll
    struct epoll_event ev{};
    ev.events  = EPOLLIN;
    ev.data.fd = tfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev);

    int64_t t_start = now_ns();
    int iter = 0;
    const int MAX_ITER = 10;

    struct epoll_event events[4];
    while (iter < MAX_ITER) {
        // Wait cho event (timeout 100ms)
        int n = epoll_wait(epfd, events, 4, 100);

        if (n < 0) { perror("epoll_wait"); break; }
        if (n == 0) { std::cout << "epoll timeout!\n"; break; }

        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == tfd) {
                uint64_t expirations;
                read(tfd, &expirations, sizeof(expirations));

                int64_t elapsed_ms = (now_ns() - t_start) / 1'000'000;
                printf("  [%3ldms] Timer fired (iter=%d)\n", elapsed_ms, iter);
                iter++;
            }
        }
    }

    close(tfd);
    close(epfd);
}

// ─── Ví dụ 4: One-shot timer với timerfd ──────────────────────────────────
void example_oneshot_timer() {
    std::cout << "\n=== Ví dụ 4: One-shot timer ===\n";

    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);

    // One-shot: interval = 0 (không lặp)
    struct itimerspec spec{};
    spec.it_value.tv_nsec = 50'000'000;  // fire sau 50ms
    spec.it_interval     = {};           // không lặp

    timerfd_settime(tfd, 0, &spec, nullptr);

    int64_t t0 = now_ns();
    std::cout << "Waiting 50ms one-shot timer...\n";

    uint64_t expirations;
    read(tfd, &expirations, sizeof(expirations));  // blocking

    int64_t elapsed = (now_ns() - t0) / 1000;
    printf("Timer fired after %ld μs (expected 50000 μs)\n", elapsed);

    close(tfd);
}

int main() {
    std::cout << "=== Phase1 Bài 06: timerfd ===\n";

    example_basic_timerfd();
    example_overrun_detection();
    example_timerfd_epoll();
    example_oneshot_timer();

    return 0;
}
