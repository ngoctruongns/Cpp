# Concurrency Tools: Atomic, Thread Pool, jthread, Deadlock Prevention

> **Bài tập liên quan:** Phase3 / 05, 06, 07, 08, 09, 10

---

## 1. `std::atomic` — Lock-free Operations

Dùng cho các phép toán đơn giản trên shared variable mà không cần mutex.

```cpp
#include <atomic>

std::atomic<int> counter{0};

// Atomic operations — đảm bảo không race
counter++;                          // fetch_add(1)
counter.fetch_add(5);               // cộng, trả về giá trị cũ
counter.fetch_sub(3);
int old = counter.exchange(100);    // set và trả về giá trị cũ
int val = counter.load();           // đọc
counter.store(42);                  // ghi

// Compare-and-Swap (CAS) — nền tảng của lock-free algorithm
int expected = 10;
bool ok = counter.compare_exchange_strong(expected, 20);
// Nếu counter == expected(10) → set 20, return true
// Nếu counter != expected    → load current vào expected, return false
```

### Memory Ordering

| Order | Ý nghĩa |
|-------|---------|
| `memory_order_relaxed` | Chỉ đảm bảo atomicity, không đảm bảo ordering |
| `memory_order_acquire` | Dùng cho load: thấy tất cả write trước release |
| `memory_order_release` | Dùng cho store: các write trước đây visible |
| `memory_order_seq_cst` | Mạnh nhất, mặc định — đảm bảo global ordering |

```cpp
// Pattern acquire/release phổ biến (producer-consumer flag)
std::atomic<bool> data_ready{false};
int data = 0;

// Producer
data = 42;
data_ready.store(true, std::memory_order_release);  // data visible trước flag

// Consumer
while (!data_ready.load(std::memory_order_acquire)) {}  // chờ flag
std::cout << data;  // đảm bảo thấy data = 42
```

**Quy tắc:** Dùng `seq_cst` (mặc định) trừ khi profiling chứng minh cần optimize.

---

## 2. Producer-Consumer Pattern

```cpp
template<typename T>
class ThreadSafeQueue {
    std::queue<T>           queue_;
    std::mutex              mtx_;
    std::condition_variable cv_;
    bool                    stopped_{false};

public:
    void push(T item) {
        {
            std::lock_guard lock(mtx_);
            queue_.push(std::move(item));
        }
        cv_.notify_one();
    }

    // Blocking pop — chờ đến khi có item hoặc stopped
    std::optional<T> pop() {
        std::unique_lock lock(mtx_);
        cv_.wait(lock, [this]{ return !queue_.empty() || stopped_; });
        if (queue_.empty()) return std::nullopt;
        T item = std::move(queue_.front());
        queue_.pop();
        return item;
    }

    void stop() {
        { std::lock_guard lock(mtx_); stopped_ = true; }
        cv_.notify_all();
    }
};
```

---

## 3. Thread Pool

```cpp
class ThreadPool {
    std::vector<std::thread>          workers_;
    ThreadSafeQueue<std::function<void()>> tasks_;

public:
    explicit ThreadPool(size_t n = std::thread::hardware_concurrency()) {
        for (size_t i = 0; i < n; ++i) {
            workers_.emplace_back([this] {
                while (auto task = tasks_.pop()) {
                    (*task)();
                }
            });
        }
    }

    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) {
        using R = std::invoke_result_t<F, Args...>;
        auto promise = std::make_shared<std::promise<R>>();
        auto future  = promise->get_future();

        tasks_.push([p = promise, f = std::forward<F>(f),
                     ...a = std::forward<Args>(args)]() mutable {
            try { p->set_value(f(a...)); }
            catch (...) { p->set_exception(std::current_exception()); }
        });
        return future;
    }

    ~ThreadPool() { tasks_.stop(); for (auto& t : workers_) t.join(); }
};

// Dùng
ThreadPool pool(4);
auto f1 = pool.submit([](int x){ return x * x; }, 7);
std::cout << f1.get();  // 49
```

---

## 4. `std::jthread` & `stop_token` (C++20)

`jthread` = `thread` + auto join trong destructor + cooperative cancellation.

```cpp
#include <thread>   // C++20: std::jthread

// jthread tự join() khi hết scope — không cần wrapper thủ công
{
    std::jthread t([](std::stop_token stoken) {
        while (!stoken.stop_requested()) {
            // ... làm việc ...
            std::this_thread::sleep_for(10ms);
        }
        std::cout << "Thread stopped cleanly\n";
    });

    std::this_thread::sleep_for(100ms);
    // t hết scope → gọi t.request_stop() rồi t.join() tự động
}

// request_stop() thủ công
std::jthread t(worker_func);
t.request_stop();   // yêu cầu dừng (cooperative)
t.join();           // chờ
// Hoặc để destructor lo
```

---

## 5. Deadlock — Nguyên Nhân & Phòng Tránh

**Deadlock** xảy ra khi 2+ thread chờ nhau giải phóng lock.

```cpp
// ❌ Deadlock
std::mutex mtx_a, mtx_b;

void thread1() {
    std::lock_guard la(mtx_a);   // lock A
    std::lock_guard lb(mtx_b);   // lock B
}
void thread2() {
    std::lock_guard lb(mtx_b);   // lock B
    std::lock_guard la(mtx_a);   // chờ A ← deadlock!
}
```

### Cách sửa 1: `std::lock()` — lock nhiều mutex đồng thời

```cpp
// std::lock dùng deadlock-avoidance algorithm (không quan tâm thứ tự)
std::lock(mtx_a, mtx_b);                            // acquire cả hai
std::lock_guard la(mtx_a, std::adopt_lock);         // RAII, không lock lại
std::lock_guard lb(mtx_b, std::adopt_lock);
```

### Cách sửa 2: `std::scoped_lock` (C++17) — cách hiện đại

```cpp
std::scoped_lock lock(mtx_a, mtx_b);  // lock cả hai atomic, tự unlock khi ra scope
```

### Cách sửa 3: Thứ tự lock nhất quán

```cpp
// Luôn lock theo thứ tự: A → B ở tất cả thread
void any_thread() {
    std::lock_guard la(mtx_a);
    std::lock_guard lb(mtx_b);
}
```

### 4 điều kiện Coffman (deadlock xảy ra khi cả 4 thỏa):

1. **Mutual exclusion** — resource chỉ dùng được bởi 1 thread
2. **Hold and wait** — thread giữ resource trong khi chờ resource khác
3. **No preemption** — resource không thể bị lấy cưỡng bức
4. **Circular wait** — vòng tròn chờ nhau

Phá bất kỳ điều kiện nào → phá deadlock.

---

## 6. Sensor Fusion Simulation (Pattern tổng hợp)

```cpp
class SensorFusion {
    ThreadSafeQueue<ImuData>   imu_queue_;
    ThreadSafeQueue<OdomData>  odom_queue_;
    ThreadPool                 pool_;
    std::atomic<bool>          running_{true};

public:
    void start() {
        pool_.submit([this]{ process_imu(); });
        pool_.submit([this]{ process_odom(); });
        pool_.submit([this]{ fuse_data(); });
    }

    void stop() {
        running_.store(false, std::memory_order_release);
        imu_queue_.stop();
        odom_queue_.stop();
    }
};
```

---

## 7. Tóm tắt

```
atomic:         Lock-free cho simple ops (counter, flag)
                Dùng memory_order_seq_cst mặc định
Producer-consumer: ThreadSafeQueue = queue + mutex + condition_variable
Thread pool:    Reuse threads, submit tasks, get futures
jthread (C++20): auto join + stop_token cooperative cancellation
Deadlock fix:   std::scoped_lock (C++17) — cách ưu tiên
```
