/**
 * PHASE 3 - Bài 01: std::thread Basics
 *
 * Mục tiêu:
 *  - Tạo, join, detach thread
 *  - Truyền arguments vào thread
 *  - std::this_thread utilities
 *  - Thread với lambda (quan trọng trong ROS2)
 *
 * Compile: g++ -std=c++17 -pthread 01_thread_basics.cpp -o out
 */

#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <string>
#include <functional>
#include <atomic>

using namespace std::chrono_literals;

// ─── Ví dụ 1: Thread cơ bản ───────────────────────────────────────────────
void worker_function(const std::string& name, int iterations) {
    for (int i = 0; i < iterations; ++i) {
        std::cout << "[" << name << "] step " << i
                  << " (thread id: " << std::this_thread::get_id() << ")\n";
        std::this_thread::sleep_for(100ms);
    }
}

// ─── Ví dụ 2: RAII Thread wrapper (quan trọng — tránh terminate!) ─────────
// Thread PHẢI được join() hoặc detach() trước khi destructor, nếu không → terminate()
class ScopedThread {
    std::thread thread_;
public:
    template<typename F, typename... Args>
    explicit ScopedThread(F&& f, Args&&... args)
        : thread_(std::forward<F>(f), std::forward<Args>(args)...) {}

    // Không copy
    ScopedThread(const ScopedThread&) = delete;
    ScopedThread& operator=(const ScopedThread&) = delete;

    // Di chuyển được
    ScopedThread(ScopedThread&&) = default;
    ScopedThread& operator=(ScopedThread&&) = default;

    ~ScopedThread() {
        if (thread_.joinable()) {
            thread_.join();  // Tự động join khi ra scope
        }
    }
    std::thread::id get_id() const { return thread_.get_id(); }
};

// ─── Ví dụ 3: Truyền argument by reference dùng std::ref ─────────────────
void fill_data(std::vector<int>& out, int start, int count) {
    for (int i = 0; i < count; ++i) {
        out.push_back(start + i);
    }
}

// ─── Ví dụ 4: Nhiều thread làm việc song song ────────────────────────────
void parallel_sensor_read() {
    std::cout << "\n=== Parallel Sensor Reading ===\n";

    std::atomic<int> lidar_count{0}, imu_count{0}, camera_count{0};

    auto lidar_thread = std::thread([&lidar_count]() {
        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(50ms);
            ++lidar_count;
            std::cout << "[lidar] reading #" << lidar_count << "\n";
        }
    });

    auto imu_thread = std::thread([&imu_count]() {
        for (int i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(20ms);  // IMU faster than LiDAR
            ++imu_count;
        }
    });

    auto camera_thread = std::thread([&camera_count]() {
        for (int i = 0; i < 3; ++i) {
            std::this_thread::sleep_for(100ms);  // Camera slower
            ++camera_count;
            std::cout << "[camera] frame #" << camera_count << "\n";
        }
    });

    // Quan trọng: phải join tất cả threads trước khi atomic variables bị destroy
    lidar_thread.join();
    imu_thread.join();
    camera_thread.join();

    std::cout << "Total: lidar=" << lidar_count
              << ", imu=" << imu_count
              << ", camera=" << camera_count << "\n";
}

// ─── Ví dụ 5: Hardware concurrency ───────────────────────────────────────
void hardware_concurrency_demo() {
    std::cout << "\n=== Hardware Concurrency ===\n";
    unsigned int n = std::thread::hardware_concurrency();
    std::cout << "Available CPU cores/hyper-threads: " << n << "\n";
    std::cout << "Main thread id: " << std::this_thread::get_id() << "\n";

    // Tạo một thread cho mỗi core (pattern dùng trong thread pool)
    std::vector<std::thread> pool;
    pool.reserve(n);

    std::atomic<int> tasks_done{0};
    for (unsigned int i = 0; i < n; ++i) {
        pool.emplace_back([i, &tasks_done]() {
            // Giả lập xử lý task
            std::this_thread::sleep_for(std::chrono::milliseconds(50 + i * 10));
            ++tasks_done;
        });
    }

    for (auto& t : pool) t.join();
    std::cout << "All " << tasks_done << " tasks done on " << n << " threads\n";
}

// ─── Ví dụ 6: Thread với member function ────────────────────────────────
class RobotController {
public:
    void control_loop() {
        std::cout << "[RobotController] control loop started on thread "
                  << std::this_thread::get_id() << "\n";
        for (int i = 0; i < 5 && running_; ++i) {
            std::this_thread::sleep_for(30ms);
            std::cout << "[RobotController] tick " << i << "\n";
        }
    }

    void start() {
        running_ = true;
        // Truyền member function vào thread
        thread_ = std::thread(&RobotController::control_loop, this);
    }

    void stop() {
        running_ = false;
        if (thread_.joinable()) thread_.join();
    }

    ~RobotController() { stop(); }

private:
    std::thread thread_;
    std::atomic<bool> running_{false};
};

int main() {
    std::cout << "=== Thread Basics ===\n";
    std::cout << "Main thread: " << std::this_thread::get_id() << "\n";

    // === Ví dụ 1: Basic thread ===
    std::cout << "\n=== Basic Thread ===\n";
    std::thread t1(worker_function, "worker1", 3);
    std::thread t2(worker_function, "worker2", 3);
    t1.join();
    t2.join();

    // === Ví dụ 2: ScopedThread ===
    std::cout << "\n=== ScopedThread (RAII) ===\n";
    {
        ScopedThread st([]() {
            std::cout << "[ScopedThread] running, will auto-join on scope exit\n";
            std::this_thread::sleep_for(50ms);
        });
        std::cout << "ScopedThread id: " << st.get_id() << "\n";
    }  // ← auto join here

    // === Ví dụ 3: Pass by reference ===
    std::cout << "\n=== Pass by Reference (std::ref) ===\n";
    std::vector<int> data;
    std::thread t3(fill_data, std::ref(data), 10, 5);
    t3.join();
    std::cout << "data: ";
    for (auto v : data) std::cout << v << " ";
    std::cout << "\n";

    // === Ví dụ 4: Parallel sensors ===
    parallel_sensor_read();

    // === Ví dụ 5: Hardware concurrency ===
    hardware_concurrency_demo();

    // === Ví dụ 6: Member function thread ===
    std::cout << "\n=== RobotController Thread ===\n";
    RobotController controller;
    controller.start();
    std::this_thread::sleep_for(200ms);
    controller.stop();

    return 0;
}

/**
 * ═══ BÀI TẬP TỰ LÀM ═══════════════════════════════════════════════════════
 *
 * 1. Viết `parallel_transform(input, output, func)` xử lý vector song song:
 *    - Chia input thành N phần (N = hardware_concurrency)
 *    - Mỗi thread xử lý một phần
 *    - Join tất cả threads
 *    Test với: double mỗi element, sqrt mỗi element.
 *
 * 2. Implement `jthread_example`: Dùng C++20 std::jthread với stop_token.
 *    Robot sensor thread chạy vô tận cho đến khi nhận stop signal.
 *
 * 3. Viết class `PeriodicTimer` chạy callback mỗi N milliseconds trong
 *    một background thread. Có method stop(). Dùng ScopedThread hoặc jthread.
 *    Đây là pattern cơ bản giống ROS2 wall timer.
 *
 * ═══ NGUY HIỂM CẦN TRÁNH ═══════════════════════════════════════════════════
 *
 * 1. KHÔNG detach() nếu thread access local variables → dangling reference
 * 2. KHÔNG để thread destructor được gọi khi thread joinable → terminate()
 * 3. KHÔNG truyền pointer/reference đến object có thể bị destroy
 * 4. KHÔNG chia sẻ data giữa threads mà không có synchronization
 */
