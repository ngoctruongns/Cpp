# POSIX Clocks & Timers

> **Bài tập liên quan:** Phase1 / 04, 05, 06, 07, 08

---

## 1. POSIX Clocks

```cpp
#include <time.h>

struct timespec ts;
clock_gettime(CLOCK_ID, &ts);
// ts.tv_sec  = seconds
// ts.tv_nsec = nanoseconds (0-999,999,999)
```

| Clock ID | Ý nghĩa | Dùng khi |
|----------|---------|---------|
| `CLOCK_REALTIME` | Giờ thực (UTC), có thể nhảy khi NTP điều chỉnh | Timestamp log |
| `CLOCK_MONOTONIC` | Tăng đều từ boot, không nhảy | **Đo khoảng thời gian** |
| `CLOCK_MONOTONIC_RAW` | Như MONOTONIC nhưng không điều chỉnh NTP | Latency measurement chính xác nhất |
| `CLOCK_PROCESS_CPUTIME_ID` | CPU time của process | Profiling |
| `CLOCK_THREAD_CPUTIME_ID` | CPU time của thread | Profiling |
| `CLOCK_BOOTTIME` | Như MONOTONIC + thời gian suspend | |

**Quy tắc RT:** Luôn dùng `CLOCK_MONOTONIC` hoặc `CLOCK_MONOTONIC_RAW` cho timing.

---

## 2. Đo thời gian

```cpp
// Cách dùng
struct timespec start, end;
clock_gettime(CLOCK_MONOTONIC, &start);

do_something();

clock_gettime(CLOCK_MONOTONIC, &end);

// Tính hiệu
long elapsed_ns = (end.tv_sec - start.tv_sec) * 1'000'000'000L
                 + (end.tv_nsec - start.tv_nsec);

// Helper function
inline long timespec_diff_ns(const timespec& a, const timespec& b) {
    return (b.tv_sec - a.tv_sec) * 1'000'000'000L + (b.tv_nsec - a.tv_nsec);
}

// Cộng offset vào timespec
inline void timespec_add_ns(timespec& ts, long ns) {
    ts.tv_nsec += ns;
    while (ts.tv_nsec >= 1'000'000'000L) {
        ts.tv_nsec -= 1'000'000'000L;
        ts.tv_sec++;
    }
}
```

---

## 3. `clock_nanosleep` — Sleep chính xác

```cpp
// Relative sleep (từ bây giờ + duration) — không dùng trong periodic task!
struct timespec rel_ts{ .tv_sec = 0, .tv_nsec = 1'000'000 };  // 1ms
clock_nanosleep(CLOCK_MONOTONIC, 0, &rel_ts, nullptr);

// Absolute sleep (đến timestamp cụ thể) — DÙNG CHO PERIODIC TASK
struct timespec next_wakeup;
clock_gettime(CLOCK_MONOTONIC, &next_wakeup);

while (running) {
    // Xử lý
    do_work();

    // Tính thời điểm wakeup tiếp theo
    timespec_add_ns(next_wakeup, PERIOD_NS);   // next_wakeup += period

    // Sleep đến thời điểm tuyệt đối — không drift!
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_wakeup, nullptr);
}
```

**Tại sao dùng absolute time?**

```
Relative sleep:                       Absolute sleep:
t=0ms:  wakeup, work (3ms)            t=0ms:  wakeup, work (3ms)
        sleep(10ms)                           sleep until t=10ms
t=13ms: wakeup ← DRIFT!               t=10ms: wakeup ← NO DRIFT!
        work (2ms)
        sleep(10ms)
t=25ms: wakeup ← cumulative drift!
```

---

## 4. `timerfd` — Timer qua file descriptor

Lợi thế: timer FD có thể dùng với `epoll` — multiplexing timer + socket + UART trong một event loop.

```cpp
#include <sys/timerfd.h>
#include <unistd.h>

// Tạo timer
int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);

// Cấu hình: period 10ms, initial expiry 10ms
struct itimerspec its{};
its.it_interval.tv_nsec = 10'000'000;  // 10ms period
its.it_value.tv_nsec    = 10'000'000;  // 10ms initial expiry (không được = 0)
timerfd_settime(tfd, 0, &its, nullptr);

// Đọc timer (blocking đến khi timer fire)
uint64_t expirations;
read(tfd, &expirations, sizeof(expirations));
// expirations = số lần timer đã fire kể từ lần read trước
// > 1 → overrun! ta đã miss cycle

if (expirations > 1) {
    std::cerr << "OVERRUN: missed " << expirations - 1 << " cycles\n";
}

// Cleanup
close(tfd);
```

### timerfd với epoll

```cpp
// Thêm timer vào epoll set
epoll_event ev;
ev.events  = EPOLLIN;
ev.data.fd = tfd;
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tfd, &ev);

// Event loop — handle timer + other fds
epoll_event events[10];
int n = epoll_wait(epoll_fd, events, 10, -1);
for (int i = 0; i < n; ++i) {
    if (events[i].data.fd == tfd) {
        uint64_t expirations;
        read(tfd, &expirations, sizeof(expirations));
        control_loop();
    }
}
```

---

## 5. `clock_nanosleep` vs `timerfd` — Khi nào dùng gì?

| | `clock_nanosleep` | `timerfd` |
|-|-------------------|-----------|
| Dùng khi | Thread dedicated cho một task | Cần multiplex timer + I/O |
| epoll integration | ❌ | ✅ |
| Overhead | Thấp | Thấp (read syscall thêm) |
| Absolute time | ✅ TIMER_ABSTIME | ✅ TFD_TIMER_ABSTIME |
| Missed cycles detection | Manual | Tự động (expirations > 1) |

**Quy tắc:**
- Dedicated RT thread → `clock_nanosleep` với absolute time
- Event loop kết hợp nhiều nguồn → `timerfd` + `epoll`

---

## 6. Resolution của timer

```bash
# Xem timer resolution
cat /proc/timer_list | head -20

# Kiểm tra trong code
struct timespec res;
clock_getres(CLOCK_MONOTONIC, &res);
printf("Resolution: %ld ns\n", res.tv_nsec);
// Thường: 1ns (nhưng actual precision phụ thuộc hardware ~1-100μs)
```

---

## 7. Tóm tắt

```
Đo thời gian:    clock_gettime(CLOCK_MONOTONIC, &ts)
Periodic task:   clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, nullptr)
Timer + epoll:   timerfd_create() + timerfd_settime()
Đơn vị:         1ms = 1,000,000 ns = 1'000'000L
CLOCK_MONOTONIC: không drift, không nhảy → luôn dùng cho timing
```
