# POSIX Threads: Scheduling, Priority, CPU Affinity

> **Bài tập liên quan:** Phase1 / 01, 02

---

## 1. Tạo thread với RT scheduling

```cpp
#include <pthread.h>
#include <sched.h>

pthread_t thread;
pthread_attr_t attr;

pthread_attr_init(&attr);

// Set scheduling policy
pthread_attr_setschedpolicy(&attr, SCHED_FIFO);   // hoặc SCHED_RR

// Set priority
struct sched_param param;
param.sched_priority = 80;    // 1-99, 99 = cao nhất
pthread_attr_setschedparam(&attr, &param);

// QUAN TRỌNG: bắt buộc để policy/priority có hiệu lực
pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

pthread_create(&thread, &attr, thread_func, nullptr);
pthread_attr_destroy(&attr);
```

### Priority guidelines cho robot

```
Priority 99:  KHÔNG dùng (dành cho kernel/IRQ handlers)
Priority 90:  Hardware interrupt processing
Priority 80:  Control loop (1kHz servo, motor control)
Priority 70:  Sensor reading (IMU, encoder)
Priority 60:  Communication (UART, CAN)
Priority 50:  State estimation (sensor fusion)
Priority 30:  Path planning
Priority 20:  ROS2 callbacks (non-RT)
Priority  1:  Background tasks
```

---

## 2. SCHED_FIFO vs SCHED_RR

| | SCHED_FIFO | SCHED_RR |
|-|------------|----------|
| Preemption | Chỉ bị preempt bởi thread priority cao hơn | Như FIFO + time quantum (thường 100ms) |
| Cùng priority | Thread đang chạy không bị preempt | Thread được rotate sau time quantum |
| Dùng khi | Thread chạy đến khi tự yield/block | Nhiều thread cùng priority cần chia sẻ |
| Cho robotics | ✅ Thường dùng | ít phổ biến hơn |

---

## 3. Thay đổi scheduling tại runtime

```cpp
// Thay đổi policy của thread đang chạy (cần CAP_SYS_NICE)
struct sched_param param;
param.sched_priority = 80;

// Cho thread hiện tại
sched_setscheduler(0, SCHED_FIFO, &param);

// Cho thread cụ thể
pthread_setschedparam(thread_id, SCHED_FIFO, &param);

// Đọc scheduling info
int policy;
struct sched_param cur_param;
pthread_getschedparam(pthread_self(), &policy, &cur_param);
std::cout << "policy=" << policy << " prio=" << cur_param.sched_priority << "\n";
```

---

## 4. CPU Affinity

**Tại sao quan trọng?**
- Giảm cache miss (L1/L2 cache của mỗi core riêng biệt)
- Tránh migration overhead
- Khi kết hợp với `isolcpus` → loại bỏ jitter từ OS scheduler

```cpp
#include <sched.h>

// Pin thread hiện tại vào core 2 và 3
cpu_set_t cpuset;
CPU_ZERO(&cpuset);
CPU_SET(2, &cpuset);
CPU_SET(3, &cpuset);

// Cho thread hiện tại
sched_setaffinity(0, sizeof(cpuset), &cpuset);

// Cho pthread
pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);

// Đọc affinity
cpu_set_t cur_cpuset;
pthread_getaffinity_np(pthread_self(), sizeof(cur_cpuset), &cur_cpuset);
for (int i = 0; i < CPU_SETSIZE; ++i) {
    if (CPU_ISSET(i, &cur_cpuset))
        std::cout << "Running on CPU " << i << "\n";
}
```

**Số core trên máy:**
```cpp
int n_cpus = std::thread::hardware_concurrency();  // C++ way
int n_cpus = sysconf(_SC_NPROCESSORS_ONLN);        // POSIX way
```

---

## 5. RT Thread Wrapper (C++ RAII style)

```cpp
class RTThread {
    pthread_t thread_{};
    bool joined_{false};

public:
    struct Config {
        int    policy   = SCHED_FIFO;
        int    priority = 80;
        int    cpu      = -1;     // -1 = no affinity
        size_t stack_size = 8 * 1024 * 1024;  // 8MB
    };

    template<typename F, typename... Args>
    RTThread(const Config& cfg, F&& func, Args&&... args) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);

        // Stack size
        pthread_attr_setstacksize(&attr, cfg.stack_size);

        // Scheduling
        pthread_attr_setschedpolicy(&attr, cfg.policy);
        struct sched_param param{ .sched_priority = cfg.priority };
        pthread_attr_setschedparam(&attr, &param);
        pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);

        // Wrap function + args
        auto task = std::make_unique<std::function<void()>>(
            std::bind(std::forward<F>(func), std::forward<Args>(args)...));

        pthread_create(&thread_, &attr,
            [](void* arg) -> void* {
                auto fn = std::unique_ptr<std::function<void()>>(
                    static_cast<std::function<void()>*>(arg));
                (*fn)();
                return nullptr;
            }, task.release());

        pthread_attr_destroy(&attr);

        // CPU affinity
        if (cfg.cpu >= 0) {
            cpu_set_t cpuset;
            CPU_ZERO(&cpuset);
            CPU_SET(cfg.cpu, &cpuset);
            pthread_setaffinity_np(thread_, sizeof(cpuset), &cpuset);
        }
    }

    ~RTThread() { if (!joined_ && thread_) join(); }

    void join() {
        pthread_join(thread_, nullptr);
        joined_ = true;
    }

    RTThread(const RTThread&) = delete;
    RTThread& operator=(const RTThread&) = delete;
};
```

---

## 6. Yield và Sleep trong RT thread

```cpp
// Yield CPU (chuyển sang thread cùng priority)
sched_yield();

// Sleep chính xác — KHÔNG dùng usleep/sleep trong RT!
struct timespec ts{ .tv_sec = 0, .tv_nsec = 1'000'000 };  // 1ms
clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, nullptr);

// KHÔNG dùng:
// sleep(), usleep()      ← độ chính xác kém, dùng SIGALRM
// std::this_thread::sleep_for()  ← dùng được nhưng ít control hơn
```

---

## 7. Priority Inversion & rt_mutex

**Priority inversion:** Thread priority thấp (L) giữ lock → thread priority cao (H) phải chờ L → thread priority trung (M) chạy trước H.

**Giải pháp: Priority Inheritance**
- Linux hỗ trợ `PTHREAD_PRIO_INHERIT` cho futex
- Khi H chờ lock mà L đang giữ → priority của L tạm thời tăng lên priority của H

```cpp
pthread_mutex_t mtx;
pthread_mutexattr_t attr;
pthread_mutexattr_init(&attr);
pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);  // ← priority inheritance
pthread_mutex_init(&mtx, &attr);
pthread_mutexattr_destroy(&attr);
```

---

## 8. Tóm tắt

```
SCHED_FIFO:          RT, chạy đến khi tự yield/block/preempt by higher
SCHED_RR:            RT, như FIFO + time slice
Priority 1-99:       1 = thấp nhất, 99 = cao nhất
CPU affinity:        pthread_setaffinity_np() / sched_setaffinity()
Runtime change:      sched_setscheduler(0, SCHED_FIFO, &param)
Priority inversion:  PTHREAD_PRIO_INHERIT
```
