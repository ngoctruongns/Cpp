# Shared Memory & POSIX Semaphore

> **Bài tập liên quan:** Phase2 / 01, 02, 09

---

## 1. POSIX Shared Memory

Cho phép nhiều process chia sẻ vùng nhớ chung — **zero-copy** giữa processes.

```
Process A              Shared Memory (/dev/shm)     Process B
[write data] ──mmap──► [  shared page  ] ◄──mmap── [read data]
                        No copy! Same physical RAM
```

### API

```cpp
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>

// === WRITER (creator) ===
// 1. Tạo hoặc mở shared memory object
int shm_fd = shm_open("/robot_data",           // tên: phải bắt đầu bằng /
                       O_CREAT | O_RDWR,        // tạo nếu chưa có
                       0666);                   // permissions
if (shm_fd < 0) { perror("shm_open"); exit(1); }

// 2. Đặt kích thước
ftruncate(shm_fd, sizeof(SharedData));

// 3. Map vào address space
SharedData* ptr = static_cast<SharedData*>(
    mmap(nullptr, sizeof(SharedData),
         PROT_READ | PROT_WRITE,
         MAP_SHARED,                            // MAP_SHARED: thay đổi visible cho process khác
         shm_fd, 0));

close(shm_fd);                                  // fd không cần giữ sau khi mmap

// 4. Ghi dữ liệu
ptr->velocity = 1.5;
ptr->timestamp_ns = now_ns();

// 5. Cleanup
munmap(ptr, sizeof(SharedData));
shm_unlink("/robot_data");                      // xóa (thường do creator làm)

// === READER (consumer) ===
int shm_fd = shm_open("/robot_data", O_RDONLY, 0);
SharedData* ptr = static_cast<SharedData*>(
    mmap(nullptr, sizeof(SharedData), PROT_READ, MAP_SHARED, shm_fd, 0));
close(shm_fd);
// Đọc ptr->velocity, ptr->timestamp_ns
```

### Vị trí shared memory

```bash
ls /dev/shm/           # shared memory objects tồn tại ở đây
ipcs -m                # System V shared memory (khác với POSIX)
```

---

## 2. Synchronization với POSIX Semaphore

Shared memory không có sync → race condition. Dùng semaphore hoặc mutex trong shared memory.

### Named Semaphore (giữa processes)

```cpp
#include <semaphore.h>

// Writer
sem_t* sem = sem_open("/robot_sem",
                       O_CREAT,            // tạo nếu chưa có
                       0666,               // permissions
                       1);                 // initial value = 1 (unlocked)

sem_wait(sem);         // P() — decrement, block nếu = 0
// ... write to shared memory ...
sem_post(sem);         // V() — increment, wake waiters

sem_close(sem);        // close (process này)
sem_unlink("/robot_sem");  // xóa (thường creator làm khi shutdown)
```

### Process-shared Mutex (trong shared memory — hiệu quả hơn)

```cpp
// Đặt mutex VÀO shared memory
struct SharedRegion {
    pthread_mutex_t  mutex;
    pthread_cond_t   cond;
    SensorData       data;
    bool             data_ready;
};

SharedRegion* shm = /* mmap ... */;

// Init mutex với PROCESS-SHARED attribute
pthread_mutexattr_t mattr;
pthread_mutexattr_init(&mattr);
pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
pthread_mutexattr_setprotocol(&mattr, PTHREAD_PRIO_INHERIT);  // priority inheritance
pthread_mutex_init(&shm->mutex, &mattr);
pthread_mutexattr_destroy(&mattr);

// Init cond var với PROCESS-SHARED
pthread_condattr_t cattr;
pthread_condattr_init(&cattr);
pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
pthread_cond_init(&shm->cond, &cattr);
pthread_condattr_destroy(&cattr);

// Dùng bình thường
pthread_mutex_lock(&shm->mutex);
shm->data = new_data;
shm->data_ready = true;
pthread_cond_signal(&shm->cond);
pthread_mutex_unlock(&shm->mutex);
```

---

## 3. Lock-free Ring Buffer qua Shared Memory

Không cần mutex trong hot path — dùng atomic index.

```cpp
template<typename T, size_t N>
struct alignas(64) SharedRingBuffer {
    static_assert((N & (N-1)) == 0, "N must be power of 2");

    std::atomic<uint64_t> write_idx{0};
    char pad1[64 - sizeof(std::atomic<uint64_t>)];  // cache line padding

    std::atomic<uint64_t> read_idx{0};
    char pad2[64 - sizeof(std::atomic<uint64_t>)];  // cache line padding

    T slots[N];

    bool push(const T& item) {
        uint64_t wi = write_idx.load(std::memory_order_relaxed);
        uint64_t ri = read_idx.load(std::memory_order_acquire);
        if (wi - ri >= N) return false;  // full
        slots[wi & (N-1)] = item;
        write_idx.store(wi + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& item) {
        uint64_t ri = read_idx.load(std::memory_order_relaxed);
        uint64_t wi = write_idx.load(std::memory_order_acquire);
        if (ri == wi) return false;  // empty
        item = slots[ri & (N-1)];
        read_idx.store(ri + 1, std::memory_order_release);
        return true;
    }

    size_t size() const {
        return write_idx.load(std::memory_order_relaxed)
             - read_idx.load(std::memory_order_relaxed);
    }
};

// Layout trong shared memory:
// shm_open → ftruncate(sizeof(SharedRingBuffer<SensorMsg, 256>)) → mmap
```

---

## 4. Vị trí data trong shared memory

**Rule:** Chỉ đặt **plain old data (POD)** hoặc cẩn thận với data có pointer.

```cpp
// ✅ OK: POD types
struct SharedData {
    double  velocity;
    double  angular;
    int64_t timestamp_ns;
    uint8_t status;
};

// ❌ KHÔNG OK: std::string, std::vector, smart pointer (chứa pointer → process khác không hợp lệ)
struct BadSharedData {
    std::string name;    // pointer bên trong → invalid trong process khác!
    std::vector<int> v;  // heap pointer → invalid!
};

// Fixed-size string trong shared memory:
struct GoodSharedData {
    char    name[64];    // ✅
    int     data[100];   // ✅
    int32_t count;       // ✅
};
```

---

## 5. Tóm tắt

```
shm_open()   → tạo/mở object → fd
ftruncate()  → set size
mmap()       → map vào address space → ptr
munmap()     → unmap
shm_unlink() → xóa object

Sync options:
  Named semaphore:      sem_open/sem_wait/sem_post      ← đơn giản
  Process-shared mutex: PTHREAD_PROCESS_SHARED          ← hiệu năng tốt hơn
  Lock-free ring buf:   atomic index                    ← tốt nhất cho RT
```
