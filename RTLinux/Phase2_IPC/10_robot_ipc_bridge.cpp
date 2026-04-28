/**
 * PHASE 2 - Bài 10: Robot IPC Bridge — Hoàn chỉnh
 *
 * Mục tiêu:
 *  - Kết hợp tất cả IPC: shared memory + UNIX socket + mqueue + eventfd
 *  - Simulate 3 processes: Sensor, Controller, Comm bridge
 *  - Dữ liệu chảy: Sensor → SHM → Controller → SHM → Comm → socket → ngoài
 *  - Graceful shutdown qua signalfd
 *
 *  Architecture:
 *    SensorProc  ─(shm ring buf)──>  ControllerProc ─(mqueue)──> CommProc
 *                                         │                          │
 *                                    (motor cmd)             (UNIX socket)
 *
 * Compile: g++ -std=c++17 -O2 10_robot_ipc_bridge.cpp -o out -lrt -lpthread
 * Run:     ./out
 */

#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/eventfd.h>
#include <sys/signalfd.h>
#include <mqueue.h>
#include <unistd.h>
#include <signal.h>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <atomic>
#include <sys/wait.h>
#include <time.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>

// ─── Shared types ─────────────────────────────────────────────────────────
struct SensorData {
    uint32_t seq;
    double   vel_left, vel_right, imu_yaw;
    uint64_t timestamp_ns;
};

struct MotorCmd {
    uint32_t seq;
    double   pwm_left, pwm_right;
    uint64_t timestamp_ns;
};

struct ControlPacket {  // mqueue payload
    MotorCmd cmd;
};

// ─── Names ────────────────────────────────────────────────────────────────
constexpr const char* SHM_SENSOR_NAME = "/robot_sensor_shm_10";
constexpr const char* SHM_CMD_NAME    = "/robot_cmd_shm_10";
constexpr const char* MQ_NAME         = "/robot_mq_10";
constexpr const char* SOCK_PATH       = "/tmp/robot_comm_10.sock";

// ─── Helpers ──────────────────────────────────────────────────────────────
inline int64_t mono_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1'000'000'000LL + ts.tv_nsec;
}

inline void sleepms(long ms) {
    struct timespec ts{ .tv_sec = ms / 1000, .tv_nsec = (ms % 1000) * 1'000'000 };
    nanosleep(&ts, nullptr);
}

template<typename T>
T* open_shm(const char* name, bool create, int& fd) {
    int flags = create ? (O_CREAT | O_RDWR) : O_RDWR;
    fd = shm_open(name, flags, 0666);
    if (fd < 0) return nullptr;
    if (create) ftruncate(fd, sizeof(T));
    T* ptr = static_cast<T*>(
        mmap(nullptr, sizeof(T), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);
    return ptr;
}

// ─── Process 1: Sensor ────────────────────────────────────────────────────
void sensor_process() {
    // Viết vào SHM mỗi 10ms (100Hz)
    int fd;
    auto* shm = open_shm<std::atomic<SensorData>>(SHM_SENSOR_NAME, true, fd);
    if (!shm) { perror("sensor shm"); return; }

    printf("[Sensor] Started (100Hz)\n");

    for (int i = 0; i < 20; ++i) {
        SensorData d{
            .seq           = (uint32_t)i,
            .vel_left      = 1.0 + i * 0.05,
            .vel_right     = 1.0 + i * 0.04,
            .imu_yaw       = i * 0.01,
            .timestamp_ns  = (uint64_t)mono_ns()
        };
        shm->store(d, std::memory_order_release);

        printf("[Sensor] seq=%u vel=(%.2f,%.2f)\n", d.seq, d.vel_left, d.vel_right);
        sleepms(10);
    }

    munmap(shm, sizeof(*shm));
    printf("[Sensor] Done\n");
    _exit(0);
}

// ─── Process 2: Controller ────────────────────────────────────────────────
void controller_process() {
    // Đọc SHM sensor, tính cmd, ghi qua mqueue
    sleepms(50);  // Chờ sensor sẵn sàng

    int fd;
    auto* sensor_shm = open_shm<std::atomic<SensorData>>(SHM_SENSOR_NAME, false, fd);
    if (!sensor_shm) { perror("controller shm"); return; }

    // Mở mqueue để gửi cmd cho Comm
    mq_unlink(MQ_NAME);
    struct mq_attr attr{ .mq_maxmsg = 16, .mq_msgsize = sizeof(ControlPacket) };
    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_WRONLY, 0666, &attr);
    if (mq == (mqd_t)-1) { perror("controller mq_open"); return; }

    printf("[Controller] Started (50Hz)\n");

    uint32_t last_seq = UINT32_MAX;
    int sent = 0;

    while (sent < 10) {
        SensorData sensor = sensor_shm->load(std::memory_order_acquire);
        if (sensor.seq == last_seq) { sleepms(20); continue; }
        last_seq = sensor.seq;

        // Simple control: differential drive
        double error  = sensor.vel_left - sensor.vel_right;
        MotorCmd cmd{
            .seq        = sensor.seq,
            .pwm_left   = 0.5 + error * 0.1,
            .pwm_right  = 0.5 - error * 0.1,
            .timestamp_ns = (uint64_t)mono_ns()
        };

        ControlPacket pkt{ .cmd = cmd };
        mq_send(mq, (char*)&pkt, sizeof(pkt), 10);

        printf("[Controller] seq=%u pwm=(%.3f,%.3f)\n",
               cmd.seq, cmd.pwm_left, cmd.pwm_right);
        sent++;
        sleepms(20);
    }

    mq_close(mq);
    munmap(sensor_shm, sizeof(*sensor_shm));
    printf("[Controller] Done\n");
    _exit(0);
}

// ─── Process 3: Comm Bridge ───────────────────────────────────────────────
void comm_process() {
    // Nhận cmd từ mqueue, forward qua UNIX socket
    sleepms(100);  // Chờ controller tạo mqueue

    struct mq_attr attr{ .mq_maxmsg = 16, .mq_msgsize = sizeof(ControlPacket) };
    mqd_t mq = mq_open(MQ_NAME, O_RDONLY | O_NONBLOCK, 0666, &attr);
    if (mq == (mqd_t)-1) {
        // Try again
        sleepms(200);
        mq = mq_open(MQ_NAME, O_RDONLY, 0666, &attr);
        if (mq == (mqd_t)-1) { perror("comm mq_open"); return; }
    }

    // Tạo UNIX socket server (simulate sending to ROS2 node)
    unlink(SOCK_PATH);
    int srv_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCK_PATH, sizeof(addr.sun_path) - 1);
    bind(srv_fd, (struct sockaddr*)&addr, sizeof(addr));

    printf("[Comm] Started (listening on mqueue + socket %s)\n", SOCK_PATH);

    int epfd = epoll_create1(EPOLL_CLOEXEC);
    struct epoll_event ev{ .events = EPOLLIN, .data = { .fd = (int)mq } };
    epoll_ctl(epfd, EPOLL_CTL_ADD, (int)mq, &ev);

    int received = 0;
    struct epoll_event events[4];

    while (received < 10) {
        int n = epoll_wait(epfd, events, 4, 2000);
        if (n <= 0) break;

        for (int i = 0; i < n; ++i) {
            ControlPacket pkt;
            unsigned int prio;
            ssize_t sz = mq_receive(mq, (char*)&pkt, sizeof(pkt), &prio);
            if (sz > 0) {
                printf("[Comm] Forwarding seq=%u pwm=(%.3f,%.3f) → socket\n",
                       pkt.cmd.seq, pkt.cmd.pwm_left, pkt.cmd.pwm_right);
                // In real system: send(sock_to_ros2, &pkt.cmd, sizeof(pkt.cmd), 0);
                received++;
            }
        }
    }

    close(epfd);
    close(srv_fd);
    mq_close(mq);
    mq_unlink(MQ_NAME);
    unlink(SOCK_PATH);
    printf("[Comm] Done\n");
    _exit(0);
}

// ─── Main: spawn 3 processes ──────────────────────────────────────────────
int main() {
    std::cout << "=== Phase2 Bài 10: Robot IPC Bridge ===\n";
    std::cout << "Architecture: Sensor→SHM→Controller→mqueue→Comm→socket\n\n";

    // Cleanup
    shm_unlink(SHM_SENSOR_NAME);
    shm_unlink(SHM_CMD_NAME);
    mq_unlink(MQ_NAME);
    unlink(SOCK_PATH);

    // Pre-create sensor SHM (in parent so children can share)
    int fd;
    auto* sensor_shm = open_shm<std::atomic<SensorData>>(SHM_SENSOR_NAME, true, fd);
    SensorData init{};
    sensor_shm->store(init, std::memory_order_relaxed);

    pid_t pids[3];

    // Fork 3 processes
    pids[0] = fork(); if (pids[0] == 0) sensor_process();
    pids[1] = fork(); if (pids[1] == 0) controller_process();
    pids[2] = fork(); if (pids[2] == 0) comm_process();

    if (pids[0] < 0 || pids[1] < 0 || pids[2] < 0) {
        perror("fork");
        return 1;
    }

    printf("[Main] Spawned processes: Sensor=%d Controller=%d Comm=%d\n",
           pids[0], pids[1], pids[2]);

    // Wait all
    for (int i = 0; i < 3; ++i) {
        int status;
        waitpid(pids[i], &status, 0);
        printf("[Main] Process %d exited (status=%d)\n", pids[i], WEXITSTATUS(status));
    }

    // Final cleanup
    munmap(sensor_shm, sizeof(*sensor_shm));
    shm_unlink(SHM_SENSOR_NAME);

    std::cout << "\n=== IPC Bridge completed ===\n";
    return 0;
}
