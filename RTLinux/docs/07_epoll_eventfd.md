# epoll, eventfd & signalfd

> **Bài tập liên quan:** Phase2 / 07, 08

---

## 1. epoll — Event-driven I/O Multiplexing

`epoll` theo dõi nhiều file descriptor cùng lúc, notify khi có sự kiện — không cần busy-wait hay tạo thread riêng cho mỗi fd.

```
Không dùng epoll:
  while(true) {
    read(fd1);   // block!
    read(fd2);   // không bao giờ đến đây khi fd1 blocked
  }

Dùng epoll:
  epoll_wait() → block cho đến khi fd1 hoặc fd2 có data
  → xử lý fd nào có data → quay lại wait
```

### API cơ bản

```cpp
#include <sys/epoll.h>

// 1. Tạo epoll instance
int epfd = epoll_create1(EPOLL_CLOEXEC);

// 2. Thêm fd vào watch list
epoll_event ev{};
ev.events  = EPOLLIN;            // notify khi có data để đọc
ev.data.fd = socket_fd;          // user data (fd, ptr, u64...)

epoll_ctl(epfd, EPOLL_CTL_ADD, socket_fd, &ev);
// EPOLL_CTL_ADD    — thêm fd
// EPOLL_CTL_MOD    — thay đổi events
// EPOLL_CTL_DEL    — xóa fd

// 3. Event loop
epoll_event events[16];          // buffer cho events
while (running) {
    int n = epoll_wait(epfd, events, 16, -1);    // -1 = block forever
    // int n = epoll_wait(epfd, events, 16, 10); // timeout 10ms

    for (int i = 0; i < n; ++i) {
        int fd = events[i].data.fd;
        if (events[i].events & EPOLLIN)  handle_read(fd);
        if (events[i].events & EPOLLOUT) handle_write(fd);
        if (events[i].events & EPOLLHUP) handle_disconnect(fd);
    }
}

close(epfd);
```

### Events flags

| Flag | Ý nghĩa |
|------|---------|
| `EPOLLIN` | Data available to read |
| `EPOLLOUT` | Ready to write (buffer not full) |
| `EPOLLERR` | Error on fd |
| `EPOLLHUP` | Hangup (connection closed) |
| `EPOLLET` | Edge-triggered (default: level-triggered) |
| `EPOLLONESHOT` | Notify chỉ một lần, phải arm lại |

### Level-triggered vs Edge-triggered

```
Level-triggered (mặc định):
  epoll_wait trả về khi buffer có data.
  Nếu không đọc hết → tiếp tục trả về ở lần gọi tiếp theo.
  → An toàn hơn, dễ dùng hơn.

Edge-triggered (EPOLLET):
  epoll_wait chỉ trả về khi state THAY ĐỔI (buffer trống → có data).
  Phải đọc hết data ngay (non-blocking read loop).
  → Hiệu năng cao hơn, dùng cho high-throughput.
```

### Robot event loop với timerfd + socket + UART

```cpp
class RobotEventLoop {
    int epfd_;
    int timer_fd_;      // periodic control loop
    int socket_fd_;     // control commands từ ROS2 node
    int uart_fd_;       // serial port (dùng tty fd)

public:
    void run() {
        while (running_) {
            epoll_event events[8];
            int n = epoll_wait(epfd_, events, 8, 100);  // 100ms timeout

            for (int i = 0; i < n; ++i) {
                int fd = events[i].data.fd;
                if      (fd == timer_fd_)  on_timer();
                else if (fd == socket_fd_) on_command();
                else if (fd == uart_fd_)   on_uart_data();
            }
        }
    }
};
```

---

## 2. eventfd — Lightweight Notification

`eventfd` là file descriptor đặc biệt: chỉ dùng để gửi nhận **số nguyên 64-bit** — cực nhẹ, dùng để notify giữa threads hoặc processes.

```cpp
#include <sys/eventfd.h>

// Tạo eventfd
// initval: giá trị ban đầu
// EFD_SEMAPHORE: mỗi read chỉ decrement 1 (semaphore mode)
// EFD_NONBLOCK:  non-blocking read/write
int efd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

// Gửi signal (writer thread)
uint64_t value = 1;
write(efd, &value, sizeof(value));   // tăng counter
// write có thể block nếu counter sẽ vượt UINT64_MAX - 1

// Nhận signal (reader thread) — blocking read
uint64_t count;
read(efd, &count, sizeof(count));    // đọc + reset counter về 0
// count = số lần signal được gửi kể từ lần đọc trước

// Kết hợp với epoll
epoll_event ev{};
ev.events  = EPOLLIN;
ev.data.fd = efd;
epoll_ctl(epfd, EPOLL_CTL_ADD, efd, &ev);
```

### Ứng dụng: Notify consumer thread

```cpp
class DataPipeline {
    int       notify_fd_;    // eventfd
    std::deque<Data> queue_;
    std::mutex queue_mtx_;

public:
    void produce(Data d) {
        { std::lock_guard lock(queue_mtx_); queue_.push_back(std::move(d)); }
        uint64_t one = 1;
        write(notify_fd_, &one, sizeof(one));  // signal consumer
    }

    // Consumer loop trong epoll:
    void on_notify() {
        uint64_t count;
        read(notify_fd_, &count, sizeof(count));   // clear
        std::lock_guard lock(queue_mtx_);
        while (!queue_.empty()) {
            process(queue_.front());
            queue_.pop_front();
        }
    }
};
```

---

## 3. signalfd — Handle signals như file descriptor

Thay vì signal handler (async, unsafe), dùng `signalfd` để đọc signals trong event loop.

```cpp
#include <sys/signalfd.h>
#include <signal.h>

// Block signals trước (không để kernel xử lý)
sigset_t mask;
sigemptyset(&mask);
sigaddset(&mask, SIGINT);
sigaddset(&mask, SIGTERM);
sigprocmask(SIG_BLOCK, &mask, nullptr);   // block ở tất cả threads

// Tạo signalfd
int sfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);

// Đọc signal
struct signalfd_siginfo siginfo;
ssize_t n = read(sfd, &siginfo, sizeof(siginfo));
if (n == sizeof(siginfo)) {
    if (siginfo.ssi_signo == SIGINT)  handle_interrupt();
    if (siginfo.ssi_signo == SIGTERM) handle_terminate();
}

// Kết hợp với epoll
epoll_event ev{};
ev.events  = EPOLLIN;
ev.data.fd = sfd;
epoll_ctl(epfd, EPOLL_CTL_ADD, sfd, &ev);
```

**Tại sao dùng signalfd thay signal handler?**
- Signal handler chạy async, interrupt bất kỳ code nào → race condition
- `signalfd` synchronous, đọc trong event loop → an toàn
- Có thể dùng cùng epoll → không cần thread riêng cho signal handling

---

## 4. Hoàn chỉnh: Event Loop Robot

```cpp
class RobotEventLoop {
    int epfd_;
    int signal_fd_;   // SIGINT, SIGTERM
    int timer_fd_;    // 10ms control timer
    int uart_fd_;     // serial port
    int cmd_efd_;     // eventfd from ROS2 callback thread

    void run() {
        epoll_event events[8];
        while (true) {
            int n = epoll_wait(epfd_, events, 8, -1);
            for (int i = 0; i < n; ++i) {
                int fd = events[i].data.fd;
                if      (fd == signal_fd_) { on_signal(); return; }
                else if (fd == timer_fd_)  on_control_timer();
                else if (fd == uart_fd_)   on_uart_receive();
                else if (fd == cmd_efd_)   on_new_command();
            }
        }
    }
};
```

---

## 5. Tóm tắt

```
epoll:     Multiplexing nhiều fd, event-driven, không poll
           epoll_create1 → epoll_ctl (ADD/MOD/DEL) → epoll_wait

eventfd:   Lightweight notification (uint64 counter)
           write() → tăng; read() → đọc + reset
           Dùng: notify giữa threads, cho phép kết hợp với epoll

signalfd:  Handle UNIX signals synchronously trong event loop
           sigprocmask(BLOCK) → signalfd → epoll_wait → read
```
