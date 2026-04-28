# Roadmap — Robotics System Engineer

> Những gì cần học ngoài Modern C++ & Design Patterns để master level.
> Phạm vi: **System Engineer** — không cần AI/ML, không cần sâu firmware/RTOS.

---

## Tổng quan các Layer

```
┌─────────────────────────────────────────────┐
│  Layer 6 — Architecture & System Design     │  ← Expert
├─────────────────────────────────────────────┤
│  Layer 5 — Testing, Debugging, Profiling    │
├─────────────────────────────────────────────┤
│  Layer 4 — Build System & DevOps            │
├─────────────────────────────────────────────┤
│  Layer 3 — Communication & Protocols        │
├─────────────────────────────────────────────┤
│  Layer 2 — Real-time Linux & IPC            │
├─────────────────────────────────────────────┤
│  Layer 1 — ROS2 Deep Dive                   │  ← Bắt đầu từ đây
├─────────────────────────────────────────────┤
│  [Đang học] Modern C++ + Design Patterns    │
└─────────────────────────────────────────────┘
```

---

## Layer 1 — ROS2 Deep Dive (Ưu tiên cao nhất)

Biết dùng ROS2 ≠ hiểu ROS2. System engineer cần hiểu internals.

### 1.1 Executor & Threading Model
```
- SingleThreadedExecutor vs MultiThreadedExecutor vs StaticSingleThreadedExecutor
- Callback groups: MutuallyExclusive vs Reentrant
- Spin strategies: spin(), spin_some(), spin_until_future_complete()
- Tại sao callback group quan trọng cho realtime
```

### 1.2 DDS & QoS
```
- DDS là gì: domain, participant, publisher, subscriber, topic
- FastDDS vs CycloneDDS — chọn khi nào
- QoS profiles: Reliability, Durability, History, Deadline, Liveliness
- QoS cho realtime: BEST_EFFORT + VOLATILE
- QoS cho critical data: RELIABLE + TRANSIENT_LOCAL
- Discovery mechanism: shared memory vs UDP multicast
```

### 1.3 ROS2 Communication Types
```
- Topic: pub/sub, one-to-many, fire-and-forget
- Service: request/response, synchronous (tránh trong realtime callback)
- Action: long-running task với feedback và cancellation
- Parameter: typed key-value, callback khi thay đổi
```

### 1.4 TF2 — Coordinate Frames
```
- Transform tree: world → odom → base_link → sensor_frame
- Broadcasting: StaticTransformBroadcaster vs TransformBroadcaster
- Listening: lookupTransform(), waitForTransform()
- Time-stamped transforms (tf buffer)
- Diff-drive: publish odom frame từ encoder data
```

### 1.5 ros2_control
```
- Hardware Interface: read() và write() lifecycle
- Controller Manager: load, configure, activate controllers
- Joint State Broadcaster
- Diff Drive Controller
- Controller Interface (CommandInterface, StateInterface)
- Realtime safe: tránh dynamic allocation trong update()
```

### 1.6 Bag Files (rosbag2)
```
- Record: ros2 bag record -a
- Play: ros2 bag play với rate control
- Inspect: ros2 bag info
- C++ API: để test node với recorded data (không cần robot thật)
```

**Tài nguyên:** [docs.ros.org/en/humble](https://docs.ros.org/en/humble), [ros2_control docs](https://control.ros.org)

---

## Layer 2 — Real-time Linux & IPC

Để viết code chạy đúng deadline (control loop 1kHz, 10kHz).

### 2.1 Real-time Linux
```
- PREEMPT_RT patch: biến Linux kernel thành near-realtime
- Thread priority: SCHED_FIFO, SCHED_RR vs SCHED_OTHER
  pthread_setschedparam() hoặc <pthread.h>
- CPU isolation: isolcpus, rcu_nocbs (dành CPU riêng cho RT thread)
- Memory locking: mlockall(MCL_CURRENT | MCL_FUTURE) — tránh page fault
- Stack pre-faulting: char stack[64KB]; memset(stack, 0, sizeof(stack));
```

### 2.2 Latency Measurement
```
- cyclictest: đo worst-case latency của RT thread
- /proc/sys/kernel/sched_rt_runtime_us: RT throttling
- Histogram of latency — phân tích jitter
```

### 2.3 Linux IPC (Inter-Process Communication)
```
- Shared Memory: POSIX shm_open/mmap — zero-copy giữa processes
- UNIX Domain Sockets: fast local IPC
- Pipes & FIFOs: đơn giản, reliable
- Message Queues: POSIX mq_open
- Epoll: event-driven I/O multiplexing (thay cho busy-wait)
```

### 2.4 POSIX Timers
```
- timer_create / timerfd_create: periodic callback chính xác
- clock_gettime(CLOCK_MONOTONIC): high-resolution timestamp
- clock_nanosleep: sleep đến absolute time (tránh drift)
```

**Tài nguyên:** *"Programming with POSIX Threads"* - Butenhof, `man 7 sched`

---

## Layer 3 — Communication Protocols

### 3.1 Serial Protocol Design (đang áp dụng với STM32)
```
Framing:
  - Start/end markers (0xAA 0x55)
  - Length field + CRC/checksum
  - Escape bytes cho binary data

Versioning:
  - Byte 0 = protocol version
  - Tương thích ngược

Flow Control:
  - ACK/NACK
  - Sequence numbers + timeout + retry
  - Sliding window (cho throughput cao)

Tham khảo: COBS encoding (Consistent Overhead Byte Stuffing)
```

### 3.2 CAN Bus
```
- Frame format: ID (11/29 bit), DLC, Data (0-8 byte), CRC
- Arbitration: priority by ID (lower = higher priority)
- CANopen / CiA 402: protocol cho motor drives (rất phổ biến)
- SocketCAN: Linux CAN interface (socket-based như TCP)
- Tools: candump, cansend, cananalyzer
```

### 3.3 DDS Configuration (FastDDS)
```
- XML profile: QoS, transport, discovery
- Shared memory transport: SHMT cho intra-host (zero-copy)
- UDP unicast vs multicast
- Security: RTPS DDS Security plugin
```

### 3.4 Ethernet Fieldbus (nếu làm robot công nghiệp)
```
- EtherCAT: deterministic Ethernet, <1μs jitter
- SOEM (Simple Open EtherCAT Master): C library
- Profinet, POWERLINK (ít phổ biến hơn)
```

---

## Layer 4 — Build System & DevOps

### 4.1 CMake Advanced
```cmake
# Target-based modern CMake (không dùng include_directories, link_libraries)
add_library(my_lib src/foo.cpp)
target_include_directories(my_lib PUBLIC include/)
target_link_libraries(my_lib PUBLIC Eigen3::Eigen)
target_compile_features(my_lib PUBLIC cxx_std_17)

# find_package với config mode
find_package(rclcpp REQUIRED)

# Export targets cho người dùng khác
install(TARGETS my_lib EXPORT my_lib-targets)
install(EXPORT my_lib-targets FILE my_lib-targets.cmake
        DESTINATION lib/cmake/my_lib)
```

### 4.2 Colcon & ament
```
colcon build --packages-select my_pkg --cmake-args -DCMAKE_BUILD_TYPE=Release
colcon test  --packages-select my_pkg
colcon graph                    # visualize dependency graph

ament_cmake vs ament_python
ament_lint_auto, ament_copyright
```

### 4.3 Docker cho Robotics
```dockerfile
FROM ros:humble
# Multi-stage: builder + runtime
# Không chạy ROS với root user
# Volume mount workspace

# Useful: rocker (run Docker with display, devices)
rocker --nvidia --x11 my_image
```

### 4.4 CI/CD cho Robot Software
```yaml
# GitHub Actions
- Build & test với colcon
- Lint (ament_lint, clang-tidy)
- Static analysis (clang-tidy, cppcheck)
- Integration test với Gazebo headless
- Bag file replay test
```

---

## Layer 5 — Testing, Debugging & Profiling

### 5.1 Unit Testing với Google Test
```cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>

// Mock hardware interface
class MockSerial : public ISerial {
    MOCK_METHOD(bool, write, (const std::vector<uint8_t>&), (override));
    MOCK_METHOD(std::vector<uint8_t>, read, (size_t), (override));
};

TEST(PacketEncoderTest, EncodeDecodeRoundtrip) {
    MockSerial serial;
    EXPECT_CALL(serial, write).Times(1);
    // ...
}
```

### 5.2 ROS2 Integration Testing
```python
# launch_testing: test node behavior end-to-end
from launch_testing import asserts
# Spin node, pub/sub topics, verify behavior
```

### 5.3 Debugging Tools
```
gdb:
  - Thread-aware debugging: info threads, thread N
  - Watchpoints: watch variable
  - Core dumps: ulimit -c unlimited

AddressSanitizer (ASAN):
  g++ -fsanitize=address,undefined
  Phát hiện: buffer overflow, use-after-free, memory leak

ThreadSanitizer (TSAN):
  g++ -fsanitize=thread
  Phát hiện: data race, deadlock

Valgrind Helgrind:
  valgrind --tool=helgrind ./program
```

### 5.4 Profiling
```
perf:
  perf record -g ./program
  perf report

Flamegraph:
  perf script | stackcollapse-perf.pl | flamegraph.pl > out.svg

ROS2 tracing (ros2_tracing):
  Trace DDS, node callbacks, scheduling latency
  Tool: tracetools, Trace Compass
```

---

## Layer 6 — System Architecture & Design

### 6.1 Robot Software Architecture Styles

```
Layered Architecture:
  Task Layer (planning) → Behavior Layer → Execution Layer → Hardware
  Ưu: rõ ràng, dễ test từng layer
  Nhược: latency qua nhiều layer

Component-based (như ROS2):
  Nodes giao tiếp qua messages
  Ưu: loose coupling, hot-swap
  Nhược: serialization overhead (có thể tránh với intra-process)

Behavior Trees (BT):
  Thay thế State Machine cho complex behaviors
  BehaviorTree.CPP library — chuẩn trong robotics hiện đại
```

### 6.2 Interface Design
```
Nguyên tắc:
  - Tách interface khỏi implementation (Dependency Inversion)
  - Narrow interfaces (ISP — Interface Segregation)
  - Versioned message types
  - Không expose internal state qua topic

ROS2 Message Design:
  - Dùng std_msgs types khi có thể
  - Header với stamp + frame_id cho mọi sensor data
  - Tránh string trong hot-path messages
```

### 6.3 Fault Tolerance
```
Levels:
  1. Component restart (supervisor pattern)
  2. Graceful degradation (fallback behavior)
  3. Safe stop (hardware e-stop integration)

Watchdog:
  - Software watchdog: thread không feed → restart node
  - Hardware watchdog: MCU reset nếu host mất kết nối

Diagnostics (ros2 diagnostics):
  - /diagnostics topic
  - DiagnosticUpdater: report status của mỗi subsystem
  - rqt_robot_monitor
```

### 6.4 Observability
```
Logging:
  - Structured logging (JSON format cho parsing)
  - Log levels, rate limiting (tránh spam)
  - RCUTILS_LOG_SEVERITY env var

Metrics:
  - Publish heartbeat topic
  - CPU/memory usage monitoring
  - Control loop frequency monitoring

Visualization:
  - RViz2: TF, markers, point clouds
  - PlotJuggler: real-time time series
  - Foxglove Studio: web-based ROS2 viz
```

---

## Thứ tự học đề xuất

```
Hiện tại:   Modern C++ + Design Patterns    ← đang làm
Tiếp theo:
  [1] ROS2 Deep Dive (Layer 1)              ← liên quan trực tiếp project
  [2] Serial Protocol Design (Layer 3.1)   ← bạn đang dùng UART
  [3] RT Linux basics (Layer 2.1-2.2)      ← control loop của robot
  [4] CMake + Colcon (Layer 4.1-4.2)       ← build system hàng ngày
  [5] GTest + ASAN + TSAN (Layer 5.1-5.3)  ← chất lượng code
  [6] ros2_control (Layer 1.5)             ← architecture chuẩn
  [7] CAN bus (Layer 3.2)                  ← mở rộng hardware
  [8] Architecture & BT (Layer 6)          ← level senior/expert
```

---

## Sách & Tài nguyên

| Chủ đề | Tài nguyên |
|--------|-----------|
| ROS2 | *"Programming Robots with ROS2"* (O'Reilly) |
| Real-time Linux | *"Linux Device Drivers"*, `cyclictest` docs |
| CMake | *"Professional CMake"* - Craig Scott |
| Testing | *"Modern C++ Testing with Catch2"* |
| Serial Protocol | *"Making Embedded Systems"* - White |
| Architecture | *"A Handbook of Software and Systems Engineering"* |
| Concurrency | *"C++ Concurrency in Action"* - Williams |
| Design Patterns | *"Design Patterns"* - GoF, *"Game Programming Patterns"* - Nystrom (free online, rất thực tế) |
