# Memory Locking & Stack Pre-fault

> **Bài tập liên quan:** Phase1 / 03

---

## 1. Tại sao cần Memory Locking?

Linux dùng **demand paging**: page của process không load vào RAM ngay, chỉ load khi truy cập lần đầu → **page fault** → kernel interrupt → load từ disk/swap → ~1-10ms.

Trong RT loop, page fault là **thảm họa** — làm trễ deadline.

```
Truy cập memory lần đầu:
  → Page fault exception
  → Kernel xử lý (có thể bị preempt, vào scheduler)
  → Load page từ swap/file
  → Resume thread
  → Tổng: 100μs - 10ms

Sau mlockall():
  → Tất cả pages đã locked trong RAM
  → Truy cập: ~5-50ns (cache hit/miss)
```

---

## 2. `mlockall()` — Lock tất cả pages vào RAM

```cpp
#include <sys/mman.h>

// MCL_CURRENT: lock các pages hiện đang được map
// MCL_FUTURE:  lock tất cả pages được map trong tương lai
// MCL_ONFAULT: (Linux 4.4+) lock page khi first fault (lazy)

int ret = mlockall(MCL_CURRENT | MCL_FUTURE);
if (ret != 0) {
    perror("mlockall failed");
    // Thường do không có quyền → cần sudo hoặc CAP_IPC_LOCK
}

// Unlock khi không cần nữa (optional, process exit tự unlock)
munlockall();
```

### Yêu cầu quyền

```bash
# Kiểm tra memlock limit
ulimit -l    # unlimited hoặc giá trị đủ lớn (KB)

# Cấp quyền trong /etc/security/limits.conf:
# user - memlock unlimited

# Hoặc dùng sudo / capability:
sudo setcap cap_ipc_lock=eip ./program
```

---

## 3. Stack Pre-fault

`MCL_FUTURE` lock pages **khi được map**, nhưng stack expansion vẫn có thể gây fault.

**Nguyên nhân:** Stack tăng trưởng downward. Khi hàm được gọi sâu hơn → stack pointer vượt ra ngoài page đã được map → page fault dù đã `mlockall`.

**Giải pháp:** "Touch" (đọc/ghi) toàn bộ stack size dự kiến trước khi vào RT loop.

```cpp
// Đặt ở đầu RT thread, TRƯỚC khi bắt đầu RT operation
void stack_prefault(size_t size_bytes = 8 * 1024 * 1024) {
    // Allocate trên stack và touch từng page
    volatile char dummy[size_bytes];   // volatile: ngăn compiler optimize away
    for (size_t i = 0; i < size_bytes; i += 4096) {
        dummy[i] = 0;                   // write → ensure page is faulted in
    }
    // dummy sẽ bị destroy khi ra scope — OK, pages đã được locked
}

void rt_thread_func() {
    // Bước 1: Lock memory
    mlockall(MCL_CURRENT | MCL_FUTURE);

    // Bước 2: Pre-fault stack
    stack_prefault(8 * 1024 * 1024);   // 8MB — đủ cho deep call stack

    // Bước 3: Bắt đầu RT operation
    while (running) {
        // ... RT code, không còn page fault
    }
}
```

---

## 4. Heap Pre-allocation

Tương tự stack, heap allocation (`new`/`malloc`) trong RT loop là forbidden.

```cpp
// Pre-allocate trước RT loop
std::vector<double> data;
data.reserve(1024);           // allocate memory ngay

// Object pool pre-allocated
constexpr int POOL_SIZE = 100;
std::array<SensorMessage, POOL_SIZE> msg_pool;  // stack allocated
// hoặc
auto msg_pool = std::make_unique<SensorMessage[]>(POOL_SIZE);  // heap, nhưng trước RT

// RT loop: chỉ dùng pool, không new/delete
SensorMessage* get_from_pool() {
    // O(1), không gọi allocator
    return &msg_pool[next_idx_++ % POOL_SIZE];
}
```

---

## 5. Hugepages (nâng cao)

Thay vì 4KB pages thông thường, dùng 2MB/1GB huge pages → ít TLB miss hơn cho large data.

```bash
# Kiểm tra hugepages
grep -i huge /proc/meminfo

# Cấp hugepages
echo 64 | sudo tee /proc/sys/vm/nr_hugepages   # 64 × 2MB = 128MB

# Dùng trong C++
#include <sys/mman.h>
void* mem = mmap(nullptr, 2*1024*1024,
                 PROT_READ | PROT_WRITE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB,
                 -1, 0);
```

---

## 6. `/dev/cpu_dma_latency`

CPU C-states (power saving): tắt các CPU units khi idle → tiết kiệm điện nhưng wakeup tốn 100-500μs.

```cpp
#include <fcntl.h>
#include <unistd.h>

// Yêu cầu system không vào C-state sâu
int latency_fd = open("/dev/cpu_dma_latency", O_RDWR);
if (latency_fd >= 0) {
    int32_t latency_us = 0;    // 0 = yêu cầu C0 state (no sleep)
    write(latency_fd, &latency_us, sizeof(latency_us));
    // Giữ fd mở trong suốt RT session
    // Đóng fd → system có thể vào C-state lại
}
// Đóng khi không cần nữa
close(latency_fd);
```

---

## 7. Checklist hoàn chỉnh cho RT thread

```cpp
void setup_rt_thread() {
    // 1. RT scheduling
    struct sched_param param{ .sched_priority = 80 };
    sched_setscheduler(0, SCHED_FIFO, &param);

    // 2. CPU affinity
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(ISOLATED_CPU, &cpuset);
    sched_setaffinity(0, sizeof(cpuset), &cpuset);

    // 3. Memory locking
    if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0)
        perror("mlockall");

    // 4. Stack prefault
    stack_prefault(8 * 1024 * 1024);

    // 5. DMA latency hint (cần sudo/capability)
    int dma_fd = open("/dev/cpu_dma_latency", O_RDWR);
    int32_t zero = 0;
    if (dma_fd >= 0) write(dma_fd, &zero, sizeof(zero));

    // Từ đây: RT code bắt đầu
}
```

---

## 8. Tóm tắt

```
mlockall(MCL_CURRENT | MCL_FUTURE)  — Lock tất cả pages vào RAM
stack_prefault(8MB)                 — Touch stack trước RT loop
reserve() / pre-allocate            — Không new/delete trong RT
/dev/cpu_dma_latency = 0            — Tránh CPU C-state
Yêu cầu: sudo, CAP_IPC_LOCK, ulimit -l unlimited
```
