# RT Linux & IPC — Tài liệu và Bài tập Thực hành

---

## Mục tiêu

Hiểu và áp dụng được:
- **Real-time scheduling** trên Linux: thread priority, CPU affinity, PREEMPT_RT
- **Memory management** cho realtime: mlockall, stack pre-fault, hugepages
- **High-resolution timing**: POSIX clocks, timerfd, clock_nanosleep
- **IPC mechanisms**: shared memory, UNIX sockets, pipes, message queues, epoll

---

## Yêu cầu Môi trường

### 1. Hệ điều hành

Ubuntu 22.04 LTS (khuyến nghị) hoặc bất kỳ distro Linux nào.

```bash
uname -r    # kiểm tra kernel version
```

### 2. PREEMPT_RT Kernel (quan trọng nhất)

Kernel thông thường **không phải** RT. Để học RT thực sự cần kernel với PREEMPT_RT patch.

**Kiểm tra kernel hiện tại có RT không:**
```bash
uname -v | grep PREEMPT_RT    # nếu có output → đang dùng RT kernel
cat /sys/kernel/realtime       # = 1 nếu RT
```

**Cài PREEMPT_RT kernel trên Ubuntu:**
```bash
# Cách 1: Cài sẵn từ Ubuntu (Ubuntu 22.04 có sẵn lowlatency)
sudo apt install linux-lowlatency    # lowlatency, không phải full RT nhưng dùng để học

# Cách 2: Cài linux-image với RT patch (Ubuntu Pro / mainline)
sudo apt install linux-image-lowlatency-hwe-22.04

# Cách 3: Build từ source (full PREEMPT_RT) — mất 30-60 phút
# Download: https://cdn.kernel.org/pub/linux/kernel/projects/rt/
# Xem hướng dẫn: https://wiki.linuxfoundation.org/realtime/documentation/howto/applications/preemptrt_setup
```

**Kiểm tra sau khi boot vào RT kernel:**
```bash
uname -r              # ví dụ: 5.15.0-91-lowlatency
cat /proc/sys/kernel/sched_rt_runtime_us   # = -1 nếu RT throttling tắt
```

**Tắt RT throttling (quan trọng khi dev):**
```bash
sudo sysctl -w kernel.sched_rt_runtime_us=-1
# hoặc vĩnh viễn:
echo 'kernel.sched_rt_runtime_us=-1' | sudo tee /etc/sysctl.d/99-rt.conf
sudo sysctl -p /etc/sysctl.d/99-rt.conf
```

### 3. Quyền chạy RT scheduling

Các bài tập dùng `SCHED_FIFO`/`SCHED_RR` yêu cầu quyền đặc biệt.

**Cách 1: Chạy với sudo (đơn giản khi học)**
```bash
sudo ./program
```

**Cách 2: Cấp capability (khuyến nghị khi deploy)**
```bash
sudo setcap cap_sys_nice=eip ./program
./program   # chạy được không cần sudo
```

**Cách 3: /etc/security/limits.conf (cho user cụ thể)**
```bash
sudo nano /etc/security/limits.conf
# Thêm dòng sau (thay 'youruser'):
youruser  -  rtprio   99
youruser  -  memlock  unlimited
# Logout và login lại để có hiệu lực
```

**Kiểm tra:**
```bash
ulimit -r    # RT priority limit (cần > 0)
ulimit -l    # memlock limit (cần unlimited hoặc lớn)
```

### 4. Cài đặt tools & packages

```bash
# Build tools
sudo apt install build-essential cmake

# RT testing tools
sudo apt install rt-tests     # cyclictest, hackbench, latency_histogram
sudo apt install linux-tools-generic  # perf

# IPC & system headers (thường đã có)
sudo apt install libc6-dev    # bao gồm pthread, POSIX headers

# Optional: stress test
sudo apt install stress-ng

# Optional: Trace tools
sudo apt install trace-cmd kernelshark
```

### 5. CPU Isolation (tuỳ chọn, cho bài nâng cao)

Dành riêng 1 CPU core cho RT thread, không bị OS scheduler chen vào.

```bash
# Chỉnh GRUB: /etc/default/grub
GRUB_CMDLINE_LINUX_DEFAULT="quiet splash isolcpus=3 rcu_nocbs=3 nohz_full=3"
sudo update-grub
sudo reboot

# Kiểm tra sau khi reboot
cat /sys/devices/system/cpu/isolated    # = 3
taskset -c 3 ./rt_program               # chạy program trên CPU 3
```

### 6. Compile flags

```bash
# Cơ bản
g++ -std=c++17 -pthread file.cpp -o out

# Với RT (link librt cho timer, mqueue)
g++ -std=c++17 -pthread -lrt file.cpp -o out

# Release với optimization (production)
g++ -std=c++17 -pthread -lrt -O2 -DNDEBUG file.cpp -o out

# Đo latency chính xác (tắt optimization)
g++ -std=c++17 -pthread -lrt -O0 file.cpp -o out
```

---

## Cấu trúc thư mục

```
RTLinux/
├── docs/                         ← Tài liệu học (đọc trước khi code)
│   ├── README.md                 ← File này
│   ├── 01_rt_linux_concepts.md   ← RT Linux fundamentals
│   ├── 02_posix_threads.md       ← Scheduling, priority, affinity
│   ├── 03_memory_locking.md      ← mlockall, stack prefault
│   ├── 04_clocks_timers.md       ← POSIX clocks, timerfd, clock_nanosleep
│   ├── 05_shared_memory.md       ← POSIX shm, mmap, semaphore
│   ├── 06_sockets_pipes.md       ← UNIX sockets, pipes, FIFO
│   ├── 07_epoll_eventfd.md       ← epoll, eventfd, signalfd
│   ├── 08_posix_mqueue.md        ← POSIX message queues
│   ├── 09_latency_profiling.md   ← cyclictest, measurement, perf
│   └── 10_rt_patterns.md         ← RT patterns tổng hợp
│
├── Phase1_RTScheduling/          ← Bài tập RT scheduling & timing
│   ├── 01_thread_priority.cpp    ← SCHED_FIFO/SCHED_RR basics
│   ├── 02_cpu_affinity.cpp       ← CPU pinning
│   ├── 03_memory_locking.cpp     ← mlockall + stack prefault
│   ├── 04_posix_clocks.cpp       ← clock_gettime, CLOCK_MONOTONIC
│   ├── 05_periodic_task.cpp      ← clock_nanosleep periodic loop
│   ├── 06_timerfd.cpp            ← timerfd_create periodic task
│   ├── 07_latency_measure.cpp    ← wakeup latency measurement
│   ├── 08_jitter_histogram.cpp   ← jitter histogram + statistics
│   ├── 09_watchdog.cpp           ← software watchdog pattern
│   └── 10_rt_control_loop.cpp    ← complete RT control loop
│
└── Phase2_IPC/                   ← Bài tập IPC mechanisms
    ├── 01_shared_memory.cpp      ← POSIX shm_open + mmap
    ├── 02_shm_semaphore.cpp      ← shared memory + semaphore sync
    ├── 03_unix_socket_stream.cpp ← SOCK_STREAM local socket
    ├── 04_unix_socket_dgram.cpp  ← SOCK_DGRAM low-latency IPC
    ├── 05_named_pipe_fifo.cpp    ← mkfifo, named pipe
    ├── 06_posix_mqueue.cpp       ← mq_open, mq_send, mq_receive
    ├── 07_epoll_event_loop.cpp   ← epoll event-driven I/O
    ├── 08_eventfd_signalfd.cpp   ← eventfd, signalfd
    ├── 09_shm_ringbuffer.cpp     ← lock-free ring buffer qua shm
    └── 10_robot_ipc_bridge.cpp   ← robot inter-process comm sim
```

---

## Phương pháp học

### Quy trình cho mỗi bài tập
1. Đọc section tương ứng trong docs trước
2. Đọc comment trong file `.cpp`
3. Chạy thử code gốc, quan sát output
4. **Xóa implementation, tự viết lại**
5. So sánh với bản gốc

### Thứ tự học đề xuất
```
Tuần 1:  Phase1 (01-04)  →  Scheduling, affinity, memory
Tuần 2:  Phase1 (05-08)  →  Timing, periodic task, latency measurement
Tuần 3:  Phase1 (09-10)  →  Watchdog, RT control loop
Tuần 4:  Phase2 (01-04)  →  Shared memory, semaphore, UNIX socket
Tuần 5:  Phase2 (05-08)  →  FIFO, mqueue, epoll, eventfd
Tuần 6:  Phase2 (09-10)  →  Ring buffer, robot IPC bridge
```

### Áp dụng vào robot project
| Bài học | Áp dụng vào đâu |
|---------|----------------|
| SCHED_FIFO + CPU affinity | Control loop thread trên Pi/Laptop |
| mlockall + stack prefault | Trước khi bắt đầu RT section |
| clock_nanosleep periodic | UART read loop (10ms period) |
| timerfd | Timing control loop trong ros2_control |
| Shared memory | Zero-copy giữa process nhận UART và ROS2 node |
| UNIX socket DGRAM | IPC tốc độ cao giữa processes trên cùng Pi |
| epoll | Multiplexing nhiều fd: UART + socket + timer |
| eventfd | Notification từ IPC thread sang ROS2 callback |

### Lưu ý quan trọng
- Một số bài yêu cầu `sudo` để set RT priority — xem mục quyền ở trên
- Bài latency measurement nên chạy khi **không có load khác** trên máy
- Tắt screen saver, suspend trước khi đo latency
- Nên có **2 terminal**: 1 chạy RT program, 1 dùng `sudo cyclictest` song song để so sánh

---

## Kiểm tra môi trường nhanh

```bash
# Chạy script kiểm tra trước khi học
./docs/check_env.sh    # tạo script này sau bài 01

# Hoặc kiểm tra thủ công:
uname -v | grep -i "preempt\|rt"     # RT kernel?
ulimit -r                             # RT priority limit
ulimit -l                             # memlock limit
ls /dev/cpu_dma_latency               # RT latency hint device
cat /proc/sys/kernel/sched_rt_runtime_us  # RT throttling
```

---

## So sánh IPC mechanisms — Cheat sheet

| Mechanism | Latency | Throughput | Use case |
|-----------|---------|-----------|----------|
| Shared Memory | ~100ns | Rất cao | Zero-copy, large data |
| UNIX SOCK_DGRAM | ~1μs | Cao | Request/reply, small msgs |
| UNIX SOCK_STREAM | ~2μs | Cao | Streaming, reliable |
| eventfd | ~200ns | Cao | Lightweight notification |
| POSIX mqueue | ~2μs | Trung bình | Typed messages, priority |
| Named FIFO | ~5μs | Trung bình | Simple streaming |
| TCP loopback | ~20μs | Cao | Cross-host compatible |

> **Quy tắc robot:** Intra-host IPC → UNIX socket hoặc shared memory. **Không dùng TCP** cho IPC trong cùng máy.
