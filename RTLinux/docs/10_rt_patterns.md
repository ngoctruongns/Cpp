# RT Patterns — Kết hợp tất cả trong Robot

> **Bài tập liên quan:** Phase1 / 10 · Phase2 / 10

---

## 1. RT Thread Lifecycle Chuẩn

```cpp
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <time.h>

// Bước 1: Lock memory (gọi một lần trong main)
inline void rt_setup() {
    mlockall(MCL_CURRENT | MCL_FUTURE);

    // Pre-fault stack
    volatile char stack[256 * 1024];
    for (size_t i = 0; i < sizeof(stack); i += 4096)
        stack[i] = 0;
}

// Bước 2: Tạo RT thread
struct RTThreadConfig {
    int  policy;       // SCHED_FIFO hoặc SCHED_RR
    int  priority;     // 1-99
    int  cpu;          // -1 = không pin, >=0 = pin vào CPU cụ thể
    long period_ns;    // chu kỳ; 0 = không periodic
};

pthread_t create_rt_thread(void*(*fn)(void*), void* arg,
                            const RTThreadConfig& cfg)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, cfg.policy);

    sched_param sp{ .sched_priority = cfg.priority };
    pthread_attr_setschedparam(&attr, &sp);

    if (cfg.cpu >= 0) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(cfg.cpu, &cpuset);
        pthread_attr_setaffinity_np(&attr, sizeof(cpuset), &cpuset);
    }

    pthread_t tid;
    pthread_create(&tid, &attr, fn, arg);
    pthread_attr_destroy(&attr);
    return tid;
}
```

---

## 2. Pattern: RT Control Loop

```
      Main Process
      ┌────────────┐
      │  rt_setup()│
      │            │
      │  SHM init  │─────────────── shared memory ─────────────────┐
      │            │                                                 │
      │  Threads:  │              ┌──────────────┐                  │
      │  sensor(70)│─── write ───>│ sensor buffer│                  │
      │  control(80)│── read  ───>│ cmd buffer   │<── write ────────┤
      │  comm(60)  │─── write ───>│ cmd buffer   │                  │
      └────────────┘              └──────────────┘        IPC to ROS2 node
```

```cpp
// Sensor thread (priority 70) — đọc sensor, ghi vào shared buffer
struct SensorData {
    double velocity_left;
    double velocity_right;
    double imu_yaw;
    uint64_t timestamp_ns;
};

std::atomic<SensorData> g_sensor{};    // atomic struct

void* sensor_thread(void*) {
    const long PERIOD_NS = 2'000'000;  // 500 Hz

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    while (g_running) {
        // Đọc encoder/IMU (thực tế từ UART/SPI fd)
        SensorData d;
        d.velocity_left  = read_encoder_left();
        d.velocity_right = read_encoder_right();
        d.imu_yaw        = read_imu();
        d.timestamp_ns   = timespec_to_ns(next);
        g_sensor.store(d, std::memory_order_release);

        // Advance + sleep absolute
        next.tv_nsec += PERIOD_NS;
        normalize_timespec(next);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);
    }
    return nullptr;
}

// Control thread (priority 80) — tính toán, ghi cmd
struct MotorCmd {
    double pwm_left;
    double pwm_right;
};

std::atomic<MotorCmd> g_cmd{};

void* control_thread(void*) {
    const long PERIOD_NS = 1'000'000;  // 1kHz

    struct timespec next;
    clock_gettime(CLOCK_MONOTONIC, &next);

    while (g_running) {
        SensorData sensor = g_sensor.load(std::memory_order_acquire);
        MotorCmd   cmd    = compute_control(sensor);
        g_cmd.store(cmd, std::memory_order_release);
        output_to_motor(cmd);

        next.tv_nsec += PERIOD_NS;
        normalize_timespec(next);
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr);
    }
    return nullptr;
}
```

---

## 3. Pattern: Inter-process Data Pipeline

```
ROS2 node (non-RT) ──── POSIX mqueue ────> RT Control Process
                         priority-ordered     (SCHED_FIFO 80)
                         typed messages
```

```cpp
// Non-RT side (ROS2 callback)
void on_cmd_vel(const Twist& msg) {
    RobotCommand cmd{
        .type  = CMD_VELOCITY,
        .v[0]  = msg.linear.x,
        .v[1]  = msg.angular.z,
        .stamp = now_ns()
    };
    mq_send(cmd_queue_,
            (char*)&cmd,
            sizeof(cmd),
            static_cast<unsigned>(Priority::VELOCITY_CMD));
}

// RT side — trong event loop (epoll + mqueue fd)
void on_mqueue_ready() {
    RobotCommand cmd;
    unsigned int prio;
    while (mq_receive(cmd_queue_, (char*)&cmd, sizeof(cmd), &prio) > 0) {
        if (cmd.type == CMD_ESTOP)    handle_estop();
        else if (cmd.type == CMD_VELOCITY) handle_velocity(cmd);
    }
}
```

---

## 4. Pattern: Watchdog

```cpp
class SoftwareWatchdog {
    std::atomic<uint64_t> last_feed_ns_{0};
    const uint64_t        timeout_ns_;

public:
    explicit SoftwareWatchdog(uint64_t timeout_ns) : timeout_ns_(timeout_ns) {}

    void feed() {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        last_feed_ns_.store((uint64_t)ts.tv_sec * 1'000'000'000ULL + ts.tv_nsec,
                            std::memory_order_relaxed);
    }

    bool is_alive() const {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        uint64_t now = (uint64_t)ts.tv_sec * 1'000'000'000ULL + ts.tv_nsec;
        return (now - last_feed_ns_.load(std::memory_order_relaxed)) < timeout_ns_;
    }
};

// Control thread feeds the watchdog every iteration
// Watchdog monitor thread (low priority) checks periodically
void* watchdog_monitor(void* arg) {
    auto* wd = static_cast<SoftwareWatchdog*>(arg);
    while (g_running) {
        if (!wd->is_alive()) {
            // Control thread hung → emergency stop
            trigger_emergency_stop();
        }
        struct timespec ts{ .tv_sec = 0, .tv_nsec = 50'000'000 };  // 50ms
        nanosleep(&ts, nullptr);
    }
    return nullptr;
}
```

---

## 5. Toàn bộ Priority Map — Robot

```
Priority 99: Reserved (hardware IRQ, never use)
Priority 80: Control loop      — motor PID, fastest
Priority 70: Sensor reader     — encoder, IMU
Priority 60: Communication     — UART bridge, mqueue reader
Priority 30: State machine     — mode switching, non-critical logic
Priority 10: Logging           — write to file, non-RT
Priority  1: Watchdog monitor  — check other threads alive
```

---

## 6. Tóm tắt Checklist RT Robot

```
□ mlockall(MCL_CURRENT | MCL_FUTURE) trong main()
□ stack_prefault() trước khi tạo thread
□ SCHED_FIFO + priority phù hợp cho từng thread
□ CPU affinity: pin RT threads vào isolated CPU
□ clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, ...) cho timing
□ Không gọi malloc/new trong RT loop — dùng object pool
□ Không gọi printf/std::cout trong RT thread — ghi vào ring buffer
□ Atomic cho shared data giữa RT và non-RT
□ POSIX mqueue hoặc lockfree ringbuffer cho IPC
□ Watchdog monitor để detect RT thread hang
□ sched_rt_runtime_us = -1 (disable RT throttling)
```
