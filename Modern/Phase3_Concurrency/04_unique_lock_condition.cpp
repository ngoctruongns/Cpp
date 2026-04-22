/**
 * PHASE 3 - Bài 04: unique_lock + condition_variable
 *
 * Mục tiêu:
 *  - Hiểu condition_variable để synchronize threads
 *  - Dùng unique_lock (flexible hơn lock_guard — có thể unlock tạm thời)
 *  - Pattern: wait / notify_one / notify_all
 *  - Tránh spurious wakeup bằng predicate
 *
 * Compile: g++ -std=c++17 -pthread 04_unique_lock_condition.cpp -o out
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <string>
#include <chrono>
#include <optional>
#include <atomic>

using namespace std::chrono_literals;

// ─── Phần 1: unique_lock vs lock_guard ────────────────────────────────────
void unique_lock_features_demo() {
    std::cout << "=== unique_lock Features ===\n";
    std::mutex mtx;

    {
        std::unique_lock<std::mutex> lock(mtx);
        std::cout << "Locked: " << lock.owns_lock() << "\n";

        // unique_lock có thể unlock tạm thời (lock_guard không làm được)
        lock.unlock();
        std::cout << "After unlock: " << lock.owns_lock() << "\n";

        // Làm việc không cần lock...

        lock.lock();
        std::cout << "Re-locked: " << lock.owns_lock() << "\n";
    }  // tự unlock khi ra scope

    // Defer lock: tạo lock nhưng chưa lock ngay
    std::unique_lock<std::mutex> deferred_lock(mtx, std::defer_lock);
    // ... làm setup ...
    deferred_lock.lock();  // lock sau
    std::cout << "Deferred lock acquired\n";
}

// ─── Phần 2: condition_variable — waking up waiting thread ────────────────
// Scenario: Robot controller đợi sensor data trước khi xử lý

struct SensorData {
    double distance;
    double angle;
    uint64_t timestamp;
};

class SensorDataBuffer {
public:
    // Producer: sensor thread gọi hàm này khi có data mới
    void push(SensorData data) {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            buffer_.push(std::move(data));
            std::cout << "[Sensor] pushed data, buffer size=" << buffer_.size() << "\n";
        }
        // Notify NGOÀI lock để tránh "notify và lock lại ngay" (spurious unlock)
        cv_.notify_one();
    }

    // Consumer: processing thread gọi hàm này để lấy data
    // Trả về nullopt nếu timeout
    std::optional<SensorData> pop(std::chrono::milliseconds timeout = 500ms) {
        std::unique_lock<std::mutex> lock(mtx_);

        // wait_for với predicate — tránh spurious wakeup!
        // Predicate: chỉ continue khi buffer không rỗng HOẶC timeout
        bool got_data = cv_.wait_for(lock, timeout, [this]() {
            return !buffer_.empty() || shutdown_;
        });

        if (!got_data || shutdown_) return std::nullopt;

        SensorData data = buffer_.front();
        buffer_.pop();
        return data;
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            shutdown_ = true;
        }
        cv_.notify_all();  // Wake up tất cả waiting threads
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return buffer_.size();
    }

private:
    mutable std::mutex mtx_;
    std::condition_variable cv_;
    std::queue<SensorData> buffer_;
    bool shutdown_ = false;
};

void sensor_processing_demo() {
    std::cout << "\n=== Sensor Processing Pipeline ===\n";

    SensorDataBuffer buffer;
    std::atomic<int> processed{0};

    // Producer thread: giả lập sensor
    auto sensor_thread = std::thread([&buffer]() {
        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(100ms);  // sensor sampling rate
            buffer.push({1.5 + i * 0.1, 45.0 + i * 5.0,
                        static_cast<uint64_t>(i * 100)});
        }
        std::this_thread::sleep_for(100ms);
        buffer.shutdown();  // signal done
    });

    // Consumer thread: processing node
    auto processing_thread = std::thread([&buffer, &processed]() {
        while (true) {
            auto data = buffer.pop(200ms);
            if (!data) {
                std::cout << "[Processor] no more data, exiting\n";
                break;
            }
            std::cout << "[Processor] got: dist=" << data->distance
                      << ", angle=" << data->angle << "\n";
            ++processed;
            std::this_thread::sleep_for(30ms);  // processing time
        }
    });

    sensor_thread.join();
    processing_thread.join();
    std::cout << "Total processed: " << processed << "\n";
}

// ─── Phần 3: Barrier — đồng bộ hóa nhiều threads ─────────────────────────
// Nhiều sensor threads hoàn thành một "epoch" → processing thread bắt đầu

class SimpleBarrier {
public:
    explicit SimpleBarrier(size_t count) : count_(count), waiting_(0), generation_(0) {}

    void wait() {
        std::unique_lock<std::mutex> lock(mtx_);
        size_t gen = generation_;
        ++waiting_;

        if (waiting_ == count_) {
            // Tất cả threads đã đến barrier
            ++generation_;
            waiting_ = 0;
            cv_.notify_all();
        } else {
            // Đợi các thread khác
            cv_.wait(lock, [this, gen]() { return generation_ != gen; });
        }
    }

private:
    std::mutex mtx_;
    std::condition_variable cv_;
    size_t count_;
    size_t waiting_;
    size_t generation_;
};

void barrier_demo() {
    std::cout << "\n=== Barrier Synchronization ===\n";

    constexpr int N_SENSORS = 4;
    SimpleBarrier barrier(N_SENSORS + 1);  // +1 cho main thread

    std::vector<std::thread> sensor_threads;
    std::vector<double> sensor_readings(N_SENSORS, 0.0);

    for (int i = 0; i < N_SENSORS; ++i) {
        sensor_threads.emplace_back([i, &barrier, &sensor_readings]() {
            // Phase 1: reading
            std::this_thread::sleep_for(std::chrono::milliseconds(50 + i * 20));
            sensor_readings[i] = 1.0 + i * 0.5;
            std::cout << "[Sensor " << i << "] reading done: " << sensor_readings[i] << "\n";

            barrier.wait();  // ← đợi tất cả sensors xong

            // Phase 2: sensors có thể làm gì khác sau barrier
            std::cout << "[Sensor " << i << "] post-barrier work\n";
        });
    }

    std::cout << "[Main] waiting for all sensors...\n";
    barrier.wait();  // Main thread cũng đợi

    std::cout << "[Main] all sensors done! Fusing data:\n";
    double sum = 0;
    for (int i = 0; i < N_SENSORS; ++i) {
        sum += sensor_readings[i];
    }
    std::cout << "[Main] fused reading = " << sum / N_SENSORS << "\n";

    for (auto& t : sensor_threads) t.join();
}

int main() {
    unique_lock_features_demo();
    sensor_processing_demo();
    barrier_demo();
    return 0;
}

/**
 * ═══ BÀI TẬP TỰ LÀM ═══════════════════════════════════════════════════════
 *
 * 1. Implement `BoundedQueue<T, MaxSize>`:
 *    - push() block nếu queue đầy (dùng condition_variable)
 *    - pop() block nếu queue rỗng
 *    - shutdown() wake up tất cả waiting threads
 *    Đây là building block của thread pool.
 *
 * 2. One-shot Event (giống std::promise/future nhưng reusable):
 *    - set() → đánh dấu event đã xảy ra, wake up tất cả waiter
 *    - wait() → block cho đến khi event được set
 *    - reset() → có thể dùng lại
 *    Ứng dụng: robot initialization completed signal.
 *
 * 3. ReadWriteLock: nhiều reader có thể đọc cùng lúc, nhưng writer
 *    phải độc quyền. Dùng condition_variable + counters.
 *    (std::shared_mutex C++17 làm điều này, hãy implement lại để học)
 *
 * ═══ LIÊN KẾT ROS2 ═════════════════════════════════════════════════════════
 *
 * Trong ROS2 rclcpp:
 *   - wait_set (rclcpp::WaitSet) dùng condition_variable bên trong
 *   - spin_until_future_complete() dùng future + condition_variable
 *   - Intra-process communication dùng lock-free queue
 *   - rclcpp::GuardCondition → notify executor thread tương tự cv.notify_one()
 */
