# Thread, Mutex & Condition Variable

> **Bài tập liên quan:** Phase3 / 01, 02, 03, 04

---

## 1. `std::thread` Basics

```cpp
#include <thread>
#include <chrono>
using namespace std::chrono_literals;

// Tạo thread với function
void worker(const std::string& name, int n) {
    for (int i = 0; i < n; ++i) {
        std::cout << "[" << name << "] " << i << "\n";
        std::this_thread::sleep_for(100ms);
    }
}
std::thread t(worker, "t1", 5);
t.join();   // chờ thread kết thúc

// Tạo thread với lambda
std::thread t2([](int x) { std::cout << x * 2; }, 21);
t2.join();
```

### join vs detach

| | `join()` | `detach()` |
|-|----------|------------|
| Ý nghĩa | Chờ thread kết thúc | Thread chạy độc lập (daemon) |
| Sau khi gọi | thread joinable = false | thread joinable = false |
| Nguy hiểm | Block nếu thread không bao giờ kết thúc | Thread tiếp cận biến đã bị destroy |
| Dùng khi | Cần kết quả / cleanup | Background task không cần kiểm soát |

**Quan trọng:** Nếu `std::thread` bị destroy mà chưa `join()` hay `detach()` → `std::terminate()`.

### RAII Thread Wrapper

```cpp
class ScopedThread {
    std::thread t_;
public:
    template<typename F, typename... Args>
    explicit ScopedThread(F&& f, Args&&... args)
        : t_(std::forward<F>(f), std::forward<Args>(args)...) {}

    ~ScopedThread() { if (t_.joinable()) t_.join(); }

    ScopedThread(const ScopedThread&) = delete;
    ScopedThread(ScopedThread&&) = default;
};
```

### Truyền argument bằng reference

```cpp
void fill(std::vector<int>& out) { out.push_back(42); }

std::vector<int> v;
// std::thread t(fill, v);        // ❌ copy v, không phải ref
std::thread t(fill, std::ref(v)); // ✅ truyền reference
t.join();
```

---

## 2. Race Condition

Xảy ra khi nhiều thread cùng đọc/ghi shared data mà không đồng bộ.

```cpp
int counter = 0;

// ❌ Data race: undefined behavior
auto inc = [&]() { for (int i = 0; i < 100000; ++i) ++counter; };
std::thread t1(inc), t2(inc);
t1.join(); t2.join();
// counter thường != 200000
```

**Nguyên nhân:** `++counter` không phải atomic — gồm load, increment, store.

---

## 3. `std::mutex` & Lock Guards

```cpp
#include <mutex>

std::mutex mtx;
int counter = 0;

// lock_guard: RAII, tự unlock khi ra scope
auto safe_inc = [&]() {
    for (int i = 0; i < 100000; ++i) {
        std::lock_guard<std::mutex> lock(mtx);  // lock
        ++counter;
    }   // unlock tự động
};

// C++17: class template argument deduction
std::lock_guard lock(mtx);   // không cần <std::mutex>
```

### Các loại mutex

| Mutex | Đặc điểm |
|-------|----------|
| `std::mutex` | Basic, không đệ quy |
| `std::recursive_mutex` | Cùng thread có thể lock nhiều lần |
| `std::timed_mutex` | Hỗ trợ `try_lock_for()` timeout |
| `std::shared_mutex` (C++17) | Multiple readers / single writer |

---

## 4. `std::unique_lock` — Linh hoạt hơn lock_guard

```cpp
std::mutex mtx;
std::unique_lock<std::mutex> lock(mtx);   // lock ngay

// Có thể unlock/relock thủ công
lock.unlock();
// ... làm việc không cần lock ...
lock.lock();

// Deferred locking
std::unique_lock<std::mutex> lock2(mtx, std::defer_lock);
// ... chuẩn bị ...
lock2.lock();  // lock sau

// Try lock (không block)
std::unique_lock<std::mutex> lock3(mtx, std::try_to_lock);
if (lock3.owns_lock()) { /* got lock */ }
```

---

## 5. `std::condition_variable`

Cho phép thread **chờ** đến khi một điều kiện được thỏa mãn, thay vì busy-waiting.

```cpp
#include <condition_variable>

std::mutex mtx;
std::condition_variable cv;
bool ready = false;

// Thread producer
void producer() {
    {
        std::lock_guard lock(mtx);
        // ... chuẩn bị data ...
        ready = true;
    }
    cv.notify_one();   // đánh thức một thread đang chờ
    // cv.notify_all() // đánh thức tất cả
}

// Thread consumer
void consumer() {
    std::unique_lock lock(mtx);
    cv.wait(lock, []{ return ready; });  // unlock + chờ, relock khi được notify
    // Khi đây: lock đã được giữ lại, ready == true
    // ... xử lý data ...
}
```

**Quan trọng:**
- `wait()` bắt buộc dùng `unique_lock`, không phải `lock_guard`
- Luôn dùng predicate (`[]{ return condition; }`) để tránh **spurious wakeups**
- Spurious wakeup: thread bị đánh thức không có lý do → predicate bảo vệ khỏi điều này

---

## 6. Shared Mutex (Reader-Writer Lock)

```cpp
#include <shared_mutex>

std::shared_mutex rw_mtx;
std::map<std::string, int> data;

// Nhiều reader đồng thời
void reader() {
    std::shared_lock lock(rw_mtx);   // shared (read) lock
    auto it = data.find("key");
}

// Chỉ một writer tại một thời điểm
void writer() {
    std::unique_lock lock(rw_mtx);   // exclusive (write) lock
    data["key"] = 42;
}
```

---

## 7. Tóm tắt

```
Thread:               std::thread(func, args...)
Join/Detach:          t.join() / t.detach() — PHẢI gọi một trong hai
RAII thread:          Tự tạo ScopedThread hoặc dùng std::jthread (C++20)
Mutex:                std::mutex + std::lock_guard (đơn giản nhất)
Flexible lock:        std::unique_lock (cần cho condition_variable)
Condition variable:   std::condition_variable + predicate lambda
Reader-writer:        std::shared_mutex (C++17)
```
