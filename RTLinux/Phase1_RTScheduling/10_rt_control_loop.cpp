/**
 * PHASE 1 - Bài 10: Complete RT Control Loop
 *
 * Mục tiêu:
 *  - Kết hợp tất cả: mlockall, SCHED_FIFO, CPU affinity, clock_nanosleep,
 *    timerfd, latency measurement, watchdog
 *  - Simulate robot control loop 3 threads:
 *      * Sensor thread (priority 70, 500Hz)
 *      * Control thread (priority 80, 1kHz)
 *      * Comm thread (priority 60, 100Hz, non-blocking)
 *  - In thống kê latency sau khi chạy
 *
 * Compile: g++ -std=c++17 -O2 10_rt_control_loop.cpp -o out -lrt -lpthread -lm
 * Run:     sudo ./out
 */

#include <iostream>
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <time.h>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <cerrno>
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cstdio>

// ─── Shared state (atomic) ────────────────────────────────────────────────
struct SensorData {
    double vel_left;
    double vel_right;
    double imu_yaw;
    int64_t timestamp_ns;
};

struct MotorCmd {
    double pwm_left;
    double pwm_right;
    int64_t timestamp_ns;
};

std::atomic<SensorData> g_sensor{};
std::atomic<MotorCmd>   g_cmd{};
std::atomic<bool>       g_running{ true };

// ─── Helpers ──────────────────────────────────────────────────────────────
inline int64_t mono_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1'000'000'000LL + ts.tv_nsec;
}

inline void timespec_add_ns(struct timespec& ts, long ns) {
    ts.tv_nsec += ns;
    if (ts.tv_nsec >= 1'000'000'000L) {
        ts.tv_nsec -= 1'000'000'000L;
        ts.tv_sec++;
    }
}

// ─── RT setup ─────────────────────────────────────────────────────────────
void rt_system_setup() {
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0)
        perror("mlockall (need sudo)");

    // Stack prefault
    volatile char stack[256 * 1024];
    for (size_t i = 0; i < sizeof(stack); i += 4096) stack[i] = 0;
}

bool create_rt_thread(pthread_t& tid, void*(*fn)(void*), void* arg,
                      int policy, int priority, int cpu = -1)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, policy);
    sched_param sp{ .sched_priority = priority };
    pthread_attr_setschedparam(&attr, &sp);

    if (cpu >= 0) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cpu, &cpuset);
        pthread_attr_setaffinity_np(&attr, sizeof(cpuset), &cpuset);
    }

    int ret = pthread_create(&tid, &attr, fn, arg);
    pthread_attr_destroy(&attr);
    return ret == 0;
}

// ─── Stats ────────────────────────────────────────────────────────────────
struct LoopStats {
    const char*         name;
    std::vector<int64_t> jitters_ns;
    int                 overruns{ 0 };
    long                period_ns;

    void record(int64_t actual_ns, int64_t expected_ns) {
        int64_t j = actual_ns - expected_ns;
        jitters_ns.push_back(j);
        if (j > period_ns / 2) overruns++;
    }

    void print() const {
        if (jitters_ns.empty()) return;
        std::vector<int64_t> s = jitters_ns;
        std::sort(s.begin(), s.end());
        double avg = std::accumulate(s.begin(), s.end(), 0.0) / s.size();

        printf("%-15s | iters=%5zu | min=%+5.1fμs avg=%+5.1fμs "
               "p99=%+5.1fμs max=%+5.1fμs | overruns=%d\n",
               name,
               s.size(),
               s.front() / 1000.0,
               avg / 1000.0,
               s[(size_t)(0.99 * s.size())] / 1000.0,
               s.back() / 1000.0,
               overruns);
    }
};

LoopStats g_sensor_stats{ "Sensor(500Hz)",  {}, 0, 2'000'000 };
LoopStats g_control_stats{ "Control(1kHz)", {}, 0, 1'000'000 };
LoopStats g_comm_stats{ "Comm(100Hz)",      {}, 0, 10'000'000 };

// ─── Sensor thread (500Hz, priority 70) ──────────────────────────────────
void* sensor_thread(void*) {
    const long PERIOD_NS = 2'000'000;  // 500 Hz

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);
    int64_t start_ns = mono_ns();

    int iter = 0;
    while (g_running) {
        timespec_add_ns(next, PERIOD_NS);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);

        int64_t now = mono_ns();
        int64_t expected = start_ns + (int64_t)(iter + 1) * PERIOD_NS;
        g_sensor_stats.record(now, expected);

        // Simulate sensor read
        SensorData d{};
        d.vel_left    = std::sin(iter * 0.01) * 1.5;
        d.vel_right   = std::cos(iter * 0.01) * 1.5;
        d.imu_yaw     = iter * 0.001;
        d.timestamp_ns = now;
        g_sensor.store(d, std::memory_order_release);

        iter++;
    }
    return nullptr;
}

// ─── Control thread (1kHz, priority 80) ──────────────────────────────────
void* control_thread(void*) {
    const long PERIOD_NS = 1'000'000;  // 1kHz

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);
    int64_t start_ns = mono_ns();

    int iter = 0;
    while (g_running) {
        timespec_add_ns(next, PERIOD_NS);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);

        int64_t now = mono_ns();
        int64_t expected = start_ns + (int64_t)(iter + 1) * PERIOD_NS;
        g_control_stats.record(now, expected);

        SensorData sensor = g_sensor.load(std::memory_order_acquire);

        // Simple differential drive control (PID-like)
        double error = sensor.vel_left - sensor.vel_right;
        MotorCmd cmd{};
        cmd.pwm_left       = 0.5 + error * 0.1;
        cmd.pwm_right      = 0.5 - error * 0.1;
        cmd.timestamp_ns   = now;
        g_cmd.store(cmd, std::memory_order_release);

        iter++;
    }
    return nullptr;
}

// ─── Comm thread (100Hz, priority 60) ────────────────────────────────────
void* comm_thread(void*) {
    const long PERIOD_NS = 10'000'000;  // 100 Hz

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);
    int64_t start_ns = mono_ns();

    int iter = 0;
    while (g_running) {
        timespec_add_ns(next, PERIOD_NS);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);

        int64_t now = mono_ns();
        int64_t expected = start_ns + (int64_t)(iter + 1) * PERIOD_NS;
        g_comm_stats.record(now, expected);

        // Simulate UART/socket send (non-blocking in real code)
        MotorCmd cmd = g_cmd.load(std::memory_order_acquire);
        (void)cmd;  // simulate send

        iter++;
    }
    return nullptr;
}

int main() {
    std::cout << "=== Phase1 Bài 10: Complete RT Control Loop ===\n";
    std::cout << "(Cần sudo để set RT priority)\n\n";

    rt_system_setup();

    int n_cpu = (int)sysconf(_SC_NPROCESSORS_ONLN);
    printf("System: %d CPUs online\n", n_cpu);

    // Tạo RT threads (pin vào CPU riêng nếu có đủ CPU)
    pthread_t t_sensor, t_control, t_comm;

    bool ok_c = create_rt_thread(t_control, control_thread, nullptr,
                                  SCHED_FIFO, 80, n_cpu > 2 ? 2 : -1);
    bool ok_s = create_rt_thread(t_sensor,  sensor_thread,  nullptr,
                                  SCHED_FIFO, 70, n_cpu > 1 ? 1 : -1);
    bool ok_m = create_rt_thread(t_comm,    comm_thread,    nullptr,
                                  SCHED_FIFO, 60, -1);

    if (!ok_c || !ok_s || !ok_m) {
        std::cerr << "Warning: some threads couldn't use RT scheduling (run with sudo)\n";
        // Fallback already done inside create_rt_thread would fail silently
        // Just create normal threads
        if (!ok_c) pthread_create(&t_control, nullptr, control_thread, nullptr);
        if (!ok_s) pthread_create(&t_sensor,  nullptr, sensor_thread,  nullptr);
        if (!ok_m) pthread_create(&t_comm,    nullptr, comm_thread,    nullptr);
    }

    printf("Control loop running for 2 seconds...\n");
    struct timespec run_time{ .tv_sec = 2, .tv_nsec = 0 };
    nanosleep(&run_time, nullptr);

    g_running = false;

    pthread_join(t_control, nullptr);
    pthread_join(t_sensor,  nullptr);
    pthread_join(t_comm,    nullptr);

    printf("\n=== Latency Statistics ===\n");
    printf("%-15s | %-5s | %-15s %-15s %-15s %-15s | %s\n",
           "Thread", "iters", "min", "avg", "p99", "max", "overruns");
    printf("%s\n", std::string(100, '-').c_str());
    g_control_stats.print();
    g_sensor_stats.print();
    g_comm_stats.print();

    printf("\nRun with PREEMPT_RT kernel + sudo for best results.\n");
    printf("Check with: uname -a | grep PREEMPT_RT\n");

    return 0;
}
