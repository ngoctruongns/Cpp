# UNIX Sockets & Pipes

> **Bài tập liên quan:** Phase2 / 03, 04, 05

---

## 1. UNIX Domain Sockets

Giống TCP socket nhưng **không qua network stack** — nhanh hơn nhiều, chỉ hoạt động intra-host.

```
UNIX SOCK_STREAM:   reliable, connection-oriented  (giống TCP)
UNIX SOCK_DGRAM:    unreliable, connectionless      (giống UDP, nhưng reliable vì kernel!)
UNIX SOCK_SEQPACKET: reliable + message boundaries (ít dùng)
```

### SOCK_STREAM — Client/Server

```cpp
// === SERVER ===
#include <sys/socket.h>
#include <sys/un.h>

const char* SOCKET_PATH = "/tmp/robot_ctrl.sock";

int server_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);

struct sockaddr_un addr{};
addr.sun_family = AF_UNIX;
strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

unlink(SOCKET_PATH);                   // xóa nếu tồn tại từ lần trước
bind(server_fd, (sockaddr*)&addr, sizeof(addr));
listen(server_fd, 5);                  // backlog = 5

int client_fd = accept(server_fd, nullptr, nullptr);

// Giao tiếp
char buf[256];
ssize_t n = recv(client_fd, buf, sizeof(buf), 0);
send(client_fd, "ack", 3, 0);

close(client_fd);
close(server_fd);
unlink(SOCKET_PATH);

// === CLIENT ===
int sock_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
struct sockaddr_un addr{};
addr.sun_family = AF_UNIX;
strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

connect(sock_fd, (sockaddr*)&addr, sizeof(addr));
send(sock_fd, "hello", 5, 0);
recv(sock_fd, buf, sizeof(buf), 0);
close(sock_fd);
```

### SOCK_DGRAM — Low latency IPC

Không cần connect/accept — đơn giản hơn, latency thấp hơn, giữ message boundaries.

```cpp
// === SERVER (receiver) ===
const char* SERVER_PATH = "/tmp/robot_sensor.sock";

int server_fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
struct sockaddr_un addr{};
addr.sun_family = AF_UNIX;
strncpy(addr.sun_path, SERVER_PATH, sizeof(addr.sun_path) - 1);

unlink(SERVER_PATH);
bind(server_fd, (sockaddr*)&addr, sizeof(addr));

// Nhận — biết sender qua src_addr
struct sockaddr_un src_addr{};
socklen_t src_len = sizeof(src_addr);
SensorMsg msg;
recvfrom(server_fd, &msg, sizeof(msg), 0,
         (sockaddr*)&src_addr, &src_len);

// === CLIENT (sender) ===
const char* CLIENT_PATH = "/tmp/robot_sensor_client.sock";

int client_fd = socket(AF_UNIX, SOCK_DGRAM | SOCK_CLOEXEC, 0);
struct sockaddr_un client_addr{};
client_addr.sun_family = AF_UNIX;
strncpy(client_addr.sun_path, CLIENT_PATH, sizeof(client_addr.sun_path) - 1);
unlink(CLIENT_PATH);
bind(client_fd, (sockaddr*)&client_addr, sizeof(client_addr));  // cần bind để nhận reply

struct sockaddr_un server_addr{};
server_addr.sun_family = AF_UNIX;
strncpy(server_addr.sun_path, SERVER_PATH, sizeof(server_addr.sun_path) - 1);

SensorMsg msg{.value = 3.14};
sendto(client_fd, &msg, sizeof(msg), 0,
       (sockaddr*)&server_addr, sizeof(server_addr));
```

### Abstract namespace (Linux-specific)

Không tạo file trong filesystem → tự động cleanup khi process kết thúc.

```cpp
struct sockaddr_un addr{};
addr.sun_family = AF_UNIX;
addr.sun_path[0] = '\0';   // byte đầu = 0 → abstract namespace
strncpy(addr.sun_path + 1, "robot_ctrl", sizeof(addr.sun_path) - 2);
socklen_t addrlen = offsetof(sockaddr_un, sun_path) + 1 + strlen("robot_ctrl");

bind(fd, (sockaddr*)&addr, addrlen);
```

---

## 2. Pipes — Anonymous & Named

### Anonymous Pipe (giữa parent-child process)

```cpp
#include <unistd.h>

int pipefd[2];
pipe(pipefd);
// pipefd[0] = read end
// pipefd[1] = write end

pid_t pid = fork();
if (pid == 0) {
    // Child: writer
    close(pipefd[0]);   // đóng read end không dùng
    write(pipefd[1], "hello", 5);
    close(pipefd[1]);
} else {
    // Parent: reader
    close(pipefd[1]);   // đóng write end không dùng
    char buf[64];
    ssize_t n = read(pipefd[0], buf, sizeof(buf));
    close(pipefd[0]);
}
```

### Named FIFO (giữa processes không có quan hệ)

```cpp
#include <sys/stat.h>
#include <fcntl.h>

const char* FIFO_PATH = "/tmp/robot_fifo";

// Tạo FIFO (1 lần)
mkfifo(FIFO_PATH, 0666);

// Writer
int wr_fd = open(FIFO_PATH, O_WRONLY);  // block đến khi có reader
write(wr_fd, &data, sizeof(data));
close(wr_fd);

// Reader
int rd_fd = open(FIFO_PATH, O_RDONLY);  // block đến khi có writer
read(rd_fd, &data, sizeof(data));
close(rd_fd);

// Cleanup
unlink(FIFO_PATH);
```

### Non-blocking FIFO

```cpp
// Mở với O_NONBLOCK: không block nếu chưa có writer/reader
int rd_fd = open(FIFO_PATH, O_RDONLY | O_NONBLOCK);
// Nếu chưa có writer: ENXIO error (writer phải mở trước)
// Hoặc: O_RDWR trick để tránh block
int fd = open(FIFO_PATH, O_RDWR | O_NONBLOCK);
```

---

## 3. So sánh nhanh

| | UNIX SOCK_STREAM | UNIX SOCK_DGRAM | Named FIFO |
|-|-----------------|-----------------|-----------|
| Latency | ~2μs | ~1μs | ~5μs |
| Message boundary | ❌ (stream) | ✅ | ❌ (stream) |
| Connection | Required | Not required | Not required |
| Bidirectional | ✅ | ✅ (với 2 sockets) | ❌ (1 chiều) |
| Dùng khi | Command/control | Sensor data, events | Simple log/data pipe |

---

## 4. Truyền file descriptor qua UNIX socket (`SCM_RIGHTS`)

Linux cho phép gửi file descriptor từ process này sang process khác — dùng để share timerfd, eventfd, hoặc hardware fd.

```cpp
// Gửi fd qua UNIX socket
void send_fd(int sock, int fd_to_send) {
    char buf = 0;
    struct iovec iov{ .iov_base = &buf, .iov_len = 1 };
    char cmsg_buf[CMSG_SPACE(sizeof(int))];

    struct msghdr msg{};
    msg.msg_iov    = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control    = cmsg_buf;
    msg.msg_controllen = sizeof(cmsg_buf);

    struct cmsghdr* cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type  = SCM_RIGHTS;
    cmsg->cmsg_len   = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &fd_to_send, sizeof(int));

    sendmsg(sock, &msg, 0);
}
```

---

## 5. Tóm tắt

```
SOCK_STREAM:  reliable stream, cần connect/accept, không giữ boundaries
SOCK_DGRAM:   message-oriented, no connect, tự cleanup với abstract namespace
Named FIFO:   đơn giản, unidirectional, dùng cho log/event stream
Anonymous:    chỉ giữa parent-child processes
SCM_RIGHTS:   share file descriptor qua socket
```
