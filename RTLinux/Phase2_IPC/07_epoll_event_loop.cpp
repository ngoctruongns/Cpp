/**
 * PHASE 2 - Bài 07: epoll Event Loop với timerfd + eventfd
 *
 * Mục tiêu:
 *  - Xây dựng event loop hoàn chỉnh với epoll
 *  - Kết hợp timerfd (periodic timer) + eventfd (notification)
 *  - Level-triggered và edge-triggered epoll
 *  - Robot event loop pattern
 *
 * Compile: g++ -std=c++17 -O2 07_epoll_event_loop.cpp -o out -lrt -lpthread
 */

#include <iostream>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <sys/eventfd.h>
#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <atomic>
#include <time.h>

std::atomic<bool> g_running{ true };

inline int64_t mono_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1'000'000'000LL + ts.tv_nsec;
}

// ─── Ví dụ 1: epoll + timerfd cơ bản ─────────────────────────────────────
void example_epoll_timerfd_basic() {
    std::cout << "\n=== Ví dụ 1: epoll + timerfd (10ms) ===\n";

    int epfd = epoll_create1(EPOLL_CLOEXEC);
    int tfd  = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);

    // 10ms periodic timer
    struct itimerspec ts{};
    ts.it_value.tv_nsec    = 10'000'000;
    ts.it_interval.tv_nsec = 10'000'000;
    timerfd_settime(tfd, 0, &ts, nullptr);

    struct epoll_event ev{};
    ev.events  = EPOLLIN;
    ev.data.fd = tfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, tfd, &ev);

    struct epoll_event events[4];
    int iter = 0;
    int64_t t_start = mono_ns();

    while (iter < 5) {
        int n = epoll_wait(epfd, events, 4, 100);
        if (n <= 0) { printf("timeout\n"); break; }

        for (int i = 0; i < n; ++i) {
            if (events[i].data.fd == tfd) {
                uint64_t exp;
                read(tfd, &exp, sizeof(exp));
                int64_t ms = (mono_ns() - t_start) / 1'000'000;
                printf("  [%3ldms] Timer fired (iter=%d, exp=%lu)\n", ms, iter, exp);
                iter++;
            }
        }
    }

    close(tfd);
    close(epfd);
}

// ─── Ví dụ 2: eventfd — notify từ thread khác ─────────────────────────────
void example_epoll_eventfd() {
    std::cout << "\n=== Ví dụ 2: eventfd — notification từ thread ===\n";

    int epfd = epoll_create1(EPOLL_CLOEXEC);
    int efd  = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

    struct epoll_event ev{};
    ev.events  = EPOLLIN;
    ev.data.fd = efd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, efd, &ev);

    // Thread sẽ signal eventfd sau 100ms
    pthread_t tid;
    pthread_create(&tid, nullptr, [](void* arg) -> void* {
        int* fd = static_cast<int*>(arg);
        struct timespec sleep{ .tv_sec = 0, .tv_nsec = 100'000'000 };
        nanosleep(&sleep, nullptr);

        uint64_t val = 1;
        write(*fd, &val, sizeof(val));
        printf("[Thread] Sent eventfd notification\n");
        return nullptr;
    }, &efd);

    printf("[Main] Waiting for eventfd...\n");
    struct epoll_event events[4];
    int n = epoll_wait(epfd, events, 4, 500);  // 500ms timeout

    if (n > 0) {
        uint64_t count;
        read(efd, &count, sizeof(count));
        printf("[Main] eventfd triggered (count=%lu)\n", count);
    }

    pthread_join(tid, nullptr);
    close(efd);
    close(epfd);
}

// ─── Ví dụ 3: Robot event loop — timerfd + eventfd + UNIX socket ──────────
class RobotEventLoop {
    int epfd_;
    int timer_fd_;    // 10ms control timer
    int cmd_efd_;     // eventfd: new command from ROS2 thread
    int shutdown_efd_;// eventfd: shutdown signal

    struct Command { double v_left, v_right; };
    Command pending_cmd_{};
    std::atomic<Command> cmd_atomic_{};

    int iter_{ 0 };

public:
    RobotEventLoop() {
        epfd_         = epoll_create1(EPOLL_CLOEXEC);
        timer_fd_     = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
        cmd_efd_      = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
        shutdown_efd_ = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

        // 10ms timer
        struct itimerspec ts{};
        ts.it_value.tv_nsec    = 10'000'000;
        ts.it_interval.tv_nsec = 10'000'000;
        timerfd_settime(timer_fd_, 0, &ts, nullptr);

        add_fd(timer_fd_);
        add_fd(cmd_efd_);
        add_fd(shutdown_efd_);
    }

    ~RobotEventLoop() {
        close(timer_fd_);
        close(cmd_efd_);
        close(shutdown_efd_);
        close(epfd_);
    }

    void add_fd(int fd, uint32_t events = EPOLLIN) {
        struct epoll_event ev{ .events = events, .data = { .fd = fd } };
        epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev);
    }

    void send_command(double vl, double vr) {
        cmd_atomic_.store({ vl, vr });
        uint64_t one = 1;
        write(cmd_efd_, &one, sizeof(one));
    }

    void shutdown() {
        uint64_t one = 1;
        write(shutdown_efd_, &one, sizeof(one));
    }

    void run(int max_iters = 10) {
        struct epoll_event events[8];
        bool stop = false;

        while (!stop && iter_ < max_iters) {
            int n = epoll_wait(epfd_, events, 8, 100);
            if (n < 0) { perror("epoll_wait"); break; }

            for (int i = 0; i < n; ++i) {
                int fd = events[i].data.fd;

                if (fd == shutdown_efd_) {
                    uint64_t v; read(fd, &v, sizeof(v));
                    printf("[EventLoop] Shutdown requested\n");
                    stop = true;

                } else if (fd == cmd_efd_) {
                    uint64_t v; read(fd, &v, sizeof(v));
                    pending_cmd_ = cmd_atomic_.load();
                    printf("[EventLoop] New command: v_left=%.2f v_right=%.2f\n",
                           pending_cmd_.v_left, pending_cmd_.v_right);

                } else if (fd == timer_fd_) {
                    uint64_t exp; read(fd, &exp, sizeof(exp));
                    on_control_timer();
                }
            }
        }
    }

private:
    void on_control_timer() {
        // PID/motor control computation
        printf("[EventLoop] Control iter=%d cmd=(%.2f, %.2f)\n",
               iter_++, pending_cmd_.v_left, pending_cmd_.v_right);
    }
};

void example_robot_event_loop() {
    std::cout << "\n=== Ví dụ 3: Robot Event Loop ===\n";

    RobotEventLoop loop;

    // Simulate ROS2 thread sending commands
    pthread_t ros_thread;
    pthread_create(&ros_thread, nullptr, [](void* arg) -> void* {
        auto* loop = static_cast<RobotEventLoop*>(arg);
        for (int i = 0; i < 3; ++i) {
            struct timespec sleep{ .tv_sec = 0, .tv_nsec = 50'000'000 };
            nanosleep(&sleep, nullptr);
            loop->send_command(1.0 + i * 0.1, 0.9 + i * 0.1);
        }
        return nullptr;
    }, &loop);

    loop.run(8);

    pthread_join(ros_thread, nullptr);
    std::cout << "Robot event loop done\n";
}

int main() {
    std::cout << "=== Phase2 Bài 07: epoll Event Loop ===\n";

    example_epoll_timerfd_basic();
    example_epoll_eventfd();
    example_robot_event_loop();

    return 0;
}
