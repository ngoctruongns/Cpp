/**
 * PHASE 2 - Bài 08: eventfd + signalfd
 *
 * Mục tiêu:
 *  - eventfd: counter notification, semaphore mode
 *  - signalfd: handle SIGINT/SIGTERM synchronously trong event loop
 *  - Kết hợp eventfd + signalfd + epoll
 *  - Pattern: graceful shutdown bằng signal
 *
 * Compile: g++ -std=c++17 -O2 08_eventfd_signalfd.cpp -o out -lpthread
 */

#include <iostream>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/signalfd.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <atomic>
#include <time.h>

// ─── Ví dụ 1: eventfd — counter mode ─────────────────────────────────────
void example_eventfd_counter() {
    std::cout << "\n=== Ví dụ 1: eventfd — counter mode ===\n";

    int efd = eventfd(0, EFD_CLOEXEC);

    // Ghi nhiều lần (cộng dồn vào counter)
    uint64_t val = 3;
    write(efd, &val, sizeof(val));
    val = 5;
    write(efd, &val, sizeof(val));

    // Đọc một lần → nhận tổng = 8
    uint64_t total;
    read(efd, &total, sizeof(total));
    printf("eventfd counter: read=%lu (expected 8 = 3+5)\n", total);

    // Counter reset về 0 sau khi read
    // Thử read lần nữa sẽ block (hoặc EAGAIN nếu O_NONBLOCK)
    close(efd);
}

// ─── Ví dụ 2: eventfd — semaphore mode (EFD_SEMAPHORE) ───────────────────
void example_eventfd_semaphore() {
    std::cout << "\n=== Ví dụ 2: eventfd — semaphore mode ===\n";

    int efd = eventfd(0, EFD_CLOEXEC | EFD_SEMAPHORE | EFD_NONBLOCK);

    // Write 3 lần (như sem_post 3 lần)
    uint64_t one = 1;
    write(efd, &one, sizeof(one));  // counter = 1
    write(efd, &one, sizeof(one));  // counter = 2
    write(efd, &one, sizeof(one));  // counter = 3

    // Mỗi read chỉ decrement 1 (không phải đọc hết)
    for (int i = 0; i < 3; ++i) {
        uint64_t v;
        ssize_t n = read(efd, &v, sizeof(v));
        if (n > 0)
            printf("  semaphore read %d: got %lu (should be 1)\n", i, v);
        else if (errno == EAGAIN)
            printf("  semaphore read %d: EAGAIN (empty)\n", i);
    }

    // Lần thứ 4 → EAGAIN
    uint64_t v;
    ssize_t n = read(efd, &v, sizeof(v));
    if (n < 0 && errno == EAGAIN)
        printf("  semaphore read 3: EAGAIN (exhausted — expected)\n");

    close(efd);
}

// ─── Ví dụ 3: signalfd — sync signal handling ────────────────────────────
void example_signalfd() {
    std::cout << "\n=== Ví dụ 3: signalfd — handle signals synchronously ===\n";

    // Block SIGUSR1 và SIGUSR2 trước (không để kernel raise signal handler)
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigprocmask(SIG_BLOCK, &mask, nullptr);

    int sfd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
    if (sfd < 0) { perror("signalfd"); return; }

    int epfd = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event ev{ .events = EPOLLIN, .data = { .fd = sfd } };
    epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev);

    // Gửi signals cho chính mình
    pthread_t tid;
    pthread_create(&tid, nullptr, [](void*) -> void* {
        struct timespec sleep{ .tv_sec = 0, .tv_nsec = 100'000'000 };
        nanosleep(&sleep, nullptr);
        kill(getpid(), SIGUSR1);
        nanosleep(&sleep, nullptr);
        kill(getpid(), SIGUSR2);
        return nullptr;
    }, nullptr);

    struct epoll_event events[4];
    int received = 0;

    while (received < 2) {
        int n = epoll_wait(epfd, events, 4, 500);
        if (n <= 0) break;

        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == sfd) {
                struct signalfd_siginfo siginfo;
                ssize_t sz = read(sfd, &siginfo, sizeof(siginfo));
                if (sz == sizeof(siginfo)) {
                    const char* sig_name = (siginfo.ssi_signo == SIGUSR1) ? "SIGUSR1" : "SIGUSR2";
                    printf("  Received signal %s synchronously (pid=%u)\n",
                           sig_name, siginfo.ssi_pid);
                    received++;
                }
            }
        }
    }

    pthread_join(tid, nullptr);

    // Restore default signal mask
    sigprocmask(SIG_UNBLOCK, &mask, nullptr);

    close(sfd);
    close(epfd);
}

// ─── Ví dụ 4: Graceful shutdown pattern ──────────────────────────────────
void example_graceful_shutdown() {
    std::cout << "\n=== Ví dụ 4: Graceful shutdown với SIGINT/SIGTERM ===\n";
    std::cout << "(Simulate: gửi SIGTERM sau 200ms)\n";

    // Block SIGINT và SIGTERM
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, nullptr);

    int sfd  = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
    int epfd = epoll_create1(EPOLL_CLOEXEC);

    struct epoll_event ev{ .events = EPOLLIN, .data = { .fd = sfd } };
    epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev);

    // Simulate shutdown signal
    pthread_t killer;
    pthread_create(&killer, nullptr, [](void*) -> void* {
        struct timespec sleep{ .tv_sec = 0, .tv_nsec = 200'000'000 };
        nanosleep(&sleep, nullptr);
        kill(getpid(), SIGTERM);
        printf("[Killer] Sent SIGTERM\n");
        return nullptr;
    }, nullptr);

    // Event loop
    bool running = true;
    int tick = 0;
    struct epoll_event events[4];

    while (running) {
        int n = epoll_wait(epfd, events, 4, 50);  // 50ms timeout

        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == sfd) {
                struct signalfd_siginfo info;
                read(sfd, &info, sizeof(info));
                printf("[EventLoop] Received signal %u — shutting down\n",
                       info.ssi_signo);
                running = false;
            }
        }

        if (running) {
            printf("[EventLoop] tick %d\n", tick++);
        }
    }

    pthread_join(killer, nullptr);

    sigprocmask(SIG_UNBLOCK, &mask, nullptr);
    close(sfd);
    close(epfd);
    std::cout << "Graceful shutdown complete\n";
}

int main() {
    std::cout << "=== Phase2 Bài 08: eventfd + signalfd ===\n";

    example_eventfd_counter();
    example_eventfd_semaphore();
    example_signalfd();
    example_graceful_shutdown();

    return 0;
}
