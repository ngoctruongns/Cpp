# POSIX Message Queue

> **Bài tập liên quan:** Phase2 / 06

---

## 1. Tổng quan

POSIX mqueue: IPC với **typed messages** có **priority**. Khác pipe/socket ở chỗ:
- Giữ message boundaries (không phải byte stream)
- Mỗi message có priority — đọc theo priority cao trước
- Non-blocking và blocking mode
- Notification qua signal hoặc thread khi có message mới

```bash
# Xem message queues đang có
ls /dev/mqueue/
# Hoặc
cat /proc/sysvipc/msg    # System V (khác POSIX)

# Giới hạn hệ thống
cat /proc/sys/fs/mqueue/msg_max      # max messages per queue
cat /proc/sys/fs/mqueue/msgsize_max  # max message size
cat /proc/sys/fs/mqueue/queues_max   # max number of queues
```

---

## 2. API cơ bản

```cpp
#include <mqueue.h>
// Link với: -lrt

// Tạo/mở queue
struct mq_attr attr{};
attr.mq_flags   = 0;
attr.mq_maxmsg  = 10;         // số message tối đa
attr.mq_msgsize = sizeof(SensorMsg);  // kích thước message tối đa

// Sender: tạo queue
mqd_t mq = mq_open("/robot_sensor",  // tên: bắt đầu bằng /
                    O_CREAT | O_WRONLY,
                    0666,
                    &attr);

// Receiver: mở queue đã có
mqd_t mq = mq_open("/robot_sensor", O_RDONLY);

// Gửi message
SensorMsg msg{ .value = 3.14, .timestamp_ns = now_ns() };
unsigned int priority = 5;    // 0 = thấp nhất, 31 = cao nhất (Linux)
mq_send(mq, reinterpret_cast<const char*>(&msg), sizeof(msg), priority);

// Nhận message (blocking)
SensorMsg recv_msg;
unsigned int recv_prio;
ssize_t n = mq_receive(mq, reinterpret_cast<char*>(&recv_msg),
                        sizeof(recv_msg), &recv_prio);
// Nhận message có priority CAO NHẤT trước

// Đóng và xóa
mq_close(mq);
mq_unlink("/robot_sensor");   // xóa object (creator làm)
```

---

## 3. Non-blocking mode

```cpp
// Mở với O_NONBLOCK
mqd_t mq = mq_open("/robot_sensor", O_RDONLY | O_NONBLOCK);

// Nếu queue rỗng → return -1, errno = EAGAIN
SensorMsg msg;
ssize_t n = mq_receive(mq, (char*)&msg, sizeof(msg), nullptr);
if (n < 0) {
    if (errno == EAGAIN) {
        // queue empty — try later
    } else {
        perror("mq_receive");
    }
}
```

---

## 4. Timeout

```cpp
// mq_timedreceive / mq_timedsend
struct timespec timeout;
clock_gettime(CLOCK_REALTIME, &timeout);
timeout.tv_sec += 1;    // timeout = 1 giây từ bây giờ
// Lưu ý: CLOCK_REALTIME (không phải MONOTONIC) cho mq_timedreceive

ssize_t n = mq_timedreceive(mq, (char*)&msg, sizeof(msg), nullptr, &timeout);
if (n < 0 && errno == ETIMEDOUT) {
    // timeout!
}
```

---

## 5. Notification — Notify khi queue từ rỗng sang có message

```cpp
// Notify qua signal
struct sigevent sev{};
sev.sigev_notify = SIGEV_SIGNAL;
sev.sigev_signo  = SIGUSR1;
mq_notify(mq, &sev);
// Khi có message mới: SIGUSR1 được gửi đến process
// Phải gọi mq_notify lại sau mỗi lần được notify

// Notify qua thread
sev.sigev_notify            = SIGEV_THREAD;
sev.sigev_notify_function   = notification_callback;
sev.sigev_notify_attributes = nullptr;  // new thread sẽ được tạo
mq_notify(mq, &sev);
```

---

## 6. mqueue với epoll

```cpp
// mqd_t là file descriptor → có thể dùng với epoll!
epoll_event ev{};
ev.events  = EPOLLIN;
ev.data.fd = (int)mq;
epoll_ctl(epfd, EPOLL_CTL_ADD, (int)mq, &ev);

// Trong event loop:
// khi epoll_wait trả về với fd = (int)mq → mq_receive()
```

---

## 7. Priority-based Message Router

```cpp
// Robot command system với priorities
enum class Priority : unsigned int {
    EMERGENCY_STOP  = 31,   // Cao nhất
    VELOCITY_CMD    = 20,
    CONFIG_UPDATE   = 10,
    LOG_MESSAGE     =  1,
};

struct RobotCommand {
    uint8_t  type;
    double   value[4];
    uint64_t timestamp_ns;
};

class CommandQueue {
    mqd_t mq_;
public:
    void send_emergency_stop() {
        RobotCommand cmd{ .type = CMD_ESTOP };
        mq_send(mq_, (char*)&cmd, sizeof(cmd),
                static_cast<unsigned>(Priority::EMERGENCY_STOP));
        // Sẽ được đọc TRƯỚC mọi command khác dù gửi sau
    }
};
```

---

## 8. So sánh với các IPC khác

| | POSIX mqueue | UNIX socket | FIFO |
|-|-------------|-------------|------|
| Message boundary | ✅ | ❌ stream | ❌ stream |
| Priority | ✅ (0-31) | ❌ | ❌ |
| Persistent | ✅ (tồn tại đến unlink) | ❌ | ✅ (file) |
| epoll | ✅ | ✅ | ✅ |
| Latency | ~2μs | ~1μs | ~5μs |
| Dùng khi | Typed messages với priority | Low-latency bidirectional | Simple data stream |

---

## 9. Tóm tắt

```
mq_open()           → tạo/mở (O_CREAT, O_RDONLY/WRONLY/RDWR, O_NONBLOCK)
mq_send()           → gửi message với priority
mq_receive()        → nhận message priority cao nhất trước
mq_timedreceive()   → receive với timeout (CLOCK_REALTIME)
mq_notify()         → callback khi có message mới
mq_close() + mq_unlink() → cleanup
Link: -lrt
```
