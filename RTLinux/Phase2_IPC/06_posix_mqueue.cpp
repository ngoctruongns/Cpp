/**
 * PHASE 2 - Bài 06: POSIX Message Queue
 *
 * Mục tiêu:
 *  - Tạo mqueue với mq_open, gửi với mq_send, nhận với mq_receive
 *  - Priority-ordered delivery: message priority cao được nhận trước
 *  - Non-blocking mode (O_NONBLOCK) và timeout (mq_timedreceive)
 *  - Notification với mq_notify
 *  - Kết hợp mqd_t với epoll
 *
 * Compile: g++ -std=c++17 -O2 06_posix_mqueue.cpp -o out -lrt -lpthread
 */

#include <iostream>
#include <mqueue.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <sys/wait.h>
#include <signal.h>
#include <pthread.h>
#include <atomic>

constexpr const char* MQ_NAME = "/robot_mq_06";

// ─── Ví dụ 1: Cơ bản — gửi và nhận ──────────────────────────────────────
struct RobotCommand {
    uint8_t  type;       // CMD_VELOCITY=1, CMD_ESTOP=2, CMD_LED=3
    double   params[4];
    uint64_t timestamp;
};

constexpr uint8_t CMD_VELOCITY = 1;
constexpr uint8_t CMD_ESTOP    = 2;
constexpr uint8_t CMD_LED      = 3;

void example_basic_mqueue() {
    std::cout << "\n=== Ví dụ 1: Basic mq_send / mq_receive ===\n";

    mq_unlink(MQ_NAME);  // Cleanup trước

    struct mq_attr attr{};
    attr.mq_maxmsg  = 10;
    attr.mq_msgsize = sizeof(RobotCommand);

    // Tạo queue (O_CREAT | O_RDWR)
    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);
    if (mq == (mqd_t)-1) { perror("mq_open"); return; }

    // Gửi messages với các priority khác nhau
    RobotCommand cmd_led     { .type = CMD_LED };
    RobotCommand cmd_velocity{ .type = CMD_VELOCITY, .params = {1.5, 0.2} };
    RobotCommand cmd_estop   { .type = CMD_ESTOP };

    mq_send(mq, (char*)&cmd_led,      sizeof(cmd_led),      1);   // priority 1 (thấp)
    mq_send(mq, (char*)&cmd_velocity, sizeof(cmd_velocity),  20);  // priority 20
    mq_send(mq, (char*)&cmd_estop,    sizeof(cmd_estop),    31);  // priority 31 (cao nhất)

    printf("Sent 3 commands (LED=p1, VELOCITY=p20, ESTOP=p31)\n");

    // Nhận — sẽ theo thứ tự priority cao trước
    const char* type_names[] = { "UNKNOWN", "VELOCITY", "ESTOP", "LED" };
    for (int i = 0; i < 3; ++i) {
        RobotCommand recv;
        unsigned int prio;
        ssize_t n = mq_receive(mq, (char*)&recv, sizeof(recv), &prio);
        if (n > 0) {
            int t = recv.type < 4 ? recv.type : 0;
            printf("  Received type=%s priority=%u\n", type_names[t], prio);
        }
    }
    // Thứ tự nhận: ESTOP(31) → VELOCITY(20) → LED(1)

    mq_close(mq);
    mq_unlink(MQ_NAME);
}

// ─── Ví dụ 2: Non-blocking + timeout ─────────────────────────────────────
void example_nonblocking_timeout() {
    std::cout << "\n=== Ví dụ 2: Non-blocking + mq_timedreceive ===\n";

    mq_unlink(MQ_NAME);
    struct mq_attr attr{ .mq_maxmsg = 5, .mq_msgsize = sizeof(RobotCommand) };
    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDWR | O_NONBLOCK, 0666, &attr);

    // Non-blocking receive trên queue rỗng → EAGAIN
    RobotCommand recv;
    unsigned int prio;
    ssize_t n = mq_receive(mq, (char*)&recv, sizeof(recv), &prio);
    if (n < 0 && errno == EAGAIN)
        printf("Non-blocking receive on empty queue → EAGAIN (expected)\n");

    // Timeout receive
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);  // PHẢI dùng CLOCK_REALTIME
    timeout.tv_nsec += 200'000'000;           // 200ms timeout
    if (timeout.tv_nsec >= 1'000'000'000L) {
        timeout.tv_nsec -= 1'000'000'000L;
        timeout.tv_sec++;
    }

    n = mq_timedreceive(mq, (char*)&recv, sizeof(recv), &prio, &timeout);
    if (n < 0 && errno == ETIMEDOUT)
        printf("mq_timedreceive: ETIMEDOUT after 200ms (expected)\n");

    mq_close(mq);
    mq_unlink(MQ_NAME);
}

// ─── Ví dụ 3: Multi-process — producer + consumer ─────────────────────────
void example_multiprocess_mqueue() {
    std::cout << "\n=== Ví dụ 3: Multi-process producer + consumer ===\n";

    mq_unlink(MQ_NAME);
    struct mq_attr attr{ .mq_maxmsg = 8, .mq_msgsize = sizeof(RobotCommand) };

    mqd_t mq_srv = mq_open(MQ_NAME, O_CREAT | O_RDONLY, 0666, &attr);

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }

    if (pid == 0) {
        // ─── Child: Producer (sends 5 commands)
        mqd_t mq = mq_open(MQ_NAME, O_WRONLY);

        for (int i = 0; i < 5; ++i) {
            RobotCommand cmd{ .type = CMD_VELOCITY, .params = {(double)i * 0.1} };
            mq_send(mq, (char*)&cmd, sizeof(cmd), (i == 2) ? 31 : 10);
            printf("[Producer] Sent velocity=%.1f (prio=%d)\n",
                   cmd.params[0], (i == 2) ? 31 : 10);

            struct timespec sleep{ .tv_sec = 0, .tv_nsec = 20'000'000 };
            nanosleep(&sleep, nullptr);
        }
        mq_close(mq);
        _exit(0);

    } else {
        // ─── Parent: Consumer
        for (int i = 0; i < 5; ++i) {
            RobotCommand recv;
            unsigned int prio;
            ssize_t n = mq_receive(mq_srv, (char*)&recv, sizeof(recv), &prio);
            if (n > 0)
                printf("[Consumer] Received velocity=%.1f prio=%u\n",
                       recv.params[0], prio);
        }
        wait(nullptr);
        mq_close(mq_srv);
        mq_unlink(MQ_NAME);
    }
}

// ─── Ví dụ 4: mqueue fd + epoll ──────────────────────────────────────────
void example_mqueue_epoll() {
    std::cout << "\n=== Ví dụ 4: mqd_t + epoll ===\n";

    mq_unlink(MQ_NAME);
    struct mq_attr attr{ .mq_maxmsg = 5, .mq_msgsize = sizeof(RobotCommand) };
    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDWR | O_NONBLOCK, 0666, &attr);

    int epfd = epoll_create1(EPOLL_CLOEXEC);

    // mqd_t cast to int để dùng với epoll
    struct epoll_event ev{};
    ev.events  = EPOLLIN;
    ev.data.fd = (int)mq;
    epoll_ctl(epfd, EPOLL_CTL_ADD, (int)mq, &ev);

    // Gửi một message
    RobotCommand cmd{ .type = CMD_VELOCITY, .params = {2.0} };
    mq_send(mq, (char*)&cmd, sizeof(cmd), 10);

    struct epoll_event events[4];
    int n = epoll_wait(epfd, events, 4, 100);  // 100ms timeout

    if (n > 0 && events[0].data.fd == (int)mq) {
        RobotCommand recv;
        unsigned int prio;
        mq_receive(mq, (char*)&recv, sizeof(recv), &prio);
        printf("epoll triggered → received velocity=%.1f\n", recv.params[0]);
    }

    close(epfd);
    mq_close(mq);
    mq_unlink(MQ_NAME);
}

int main() {
    std::cout << "=== Phase2 Bài 06: POSIX Message Queue ===\n";

    example_basic_mqueue();
    example_nonblocking_timeout();
    example_multiprocess_mqueue();
    example_mqueue_epoll();

    return 0;
}
