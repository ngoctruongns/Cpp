/**
 * PHASE 3 - Bài 07: Thread Pool
 *
 * Mục tiêu:
 *  - Hiểu tại sao cần thread pool (tránh tạo/hủy thread liên tục)
 *  - Implement ThreadPool từ đầu dùng các kiến thức đã học
 *  - Submit tasks với future để lấy kết quả
 *  - Hiểu cách ROS2 MultiThreadedExecutor thực sự hoạt động
 *
 * Compile: g++ -std=c++17 -pthread 07_thread_pool.cpp -o out
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>
#include <future>
#include <memory>
#include <stdexcept>
#include <chrono>
#include <atomic>
#include <numeric>

using namespace std::chrono_literals;

// ─── ThreadPool Implementation ────────────────────────────────────────────
class ThreadPool {
public:
    explicit ThreadPool(size_t num_threads) {
        workers_.reserve(num_threads);
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this, i]() {
                worker_loop(i);
            });
        }
        std::cout << "[ThreadPool] Started with " << num_threads << " workers\n";
    }

    // Không copy, không move (threads đang chạy)
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mtx_);
            stop_ = true;
        }
        queue_cv_.notify_all();  // wake up tất cả idle workers

        for (auto& w : workers_) {
            if (w.joinable()) w.join();
        }
        std::cout << "[ThreadPool] Shutdown complete. Tasks executed: " << tasks_done_ << "\n";
    }

    // Submit task, trả về future để lấy kết quả
    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>>
    {
        using ReturnType = std::invoke_result_t<F, Args...>;

        // packaged_task: bọc function để có thể lấy future
        auto task = std::make_shared<std::packaged_task<ReturnType()>>(
            [f = std::forward<F>(f),
             args_tuple = std::make_tuple(std::forward<Args>(args)...)]() mutable {
                return std::apply(std::move(f), std::move(args_tuple));
            }
        );

        std::future<ReturnType> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queue_mtx_);
            if (stop_) throw std::runtime_error("ThreadPool is stopped");

            // Thêm vào task queue (type-erased bằng std::function<void()>)
            tasks_.emplace([task]() { (*task)(); });
        }
        queue_cv_.notify_one();  // wake up một idle worker

        return result;
    }

    size_t pending_tasks() const {
        std::lock_guard<std::mutex> lock(queue_mtx_);
        return tasks_.size();
    }

    size_t worker_count() const { return workers_.size(); }
    size_t tasks_executed() const { return tasks_done_.load(); }

private:
    void worker_loop(size_t worker_id) {
        while (true) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(queue_mtx_);
                queue_cv_.wait(lock, [this]() {
                    return stop_ || !tasks_.empty();
                });

                if (stop_ && tasks_.empty()) return;  // graceful shutdown

                task = std::move(tasks_.front());
                tasks_.pop();
            }

            task();  // execute OUTSIDE lock
            ++tasks_done_;
        }
    }

    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;

    mutable std::mutex queue_mtx_;
    std::condition_variable queue_cv_;
    std::atomic<bool> stop_{false};
    std::atomic<size_t> tasks_done_{0};
};

// ─── Ví dụ 1: Basic usage ────────────────────────────────────────────────
void basic_usage_demo() {
    std::cout << "\n=== Basic Thread Pool Usage ===\n";

    ThreadPool pool(4);

    // Submit tasks và collect futures
    std::vector<std::future<int>> futures;

    for (int i = 0; i < 10; ++i) {
        futures.push_back(pool.submit([i]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(50 + i * 10));
            return i * i;
        }));
    }

    // Lấy kết quả
    int total = 0;
    for (auto& f : futures) {
        total += f.get();  // blocking
    }
    std::cout << "Sum of squares 0..9 = " << total << " (expected 285)\n";
}

// ─── Ví dụ 2: Parallel sensor processing (ROS2-like) ─────────────────────
struct SensorFrame {
    int id;
    std::vector<double> data;
};

SensorFrame process_frame(const SensorFrame& frame) {
    // Giả lập xử lý nặng (filter, transform, etc.)
    SensorFrame result{frame.id, frame.data};
    for (auto& v : result.data) {
        v = v * 2.0 - 1.0;  // normalize
    }
    std::this_thread::sleep_for(20ms);  // simulate computation
    return result;
}

void parallel_processing_demo() {
    std::cout << "\n=== Parallel Sensor Frame Processing ===\n";

    ThreadPool pool(std::thread::hardware_concurrency());

    // Tạo 20 frames giả
    std::vector<SensorFrame> frames;
    for (int i = 0; i < 20; ++i) {
        frames.push_back({i, {1.0, 2.0, 3.0, 4.0, 5.0}});
    }

    auto t_start = std::chrono::high_resolution_clock::now();

    // Submit all frames for parallel processing
    std::vector<std::future<SensorFrame>> futures;
    for (const auto& f : frames) {
        futures.push_back(pool.submit(process_frame, f));
    }

    // Collect results
    std::vector<SensorFrame> results;
    results.reserve(frames.size());
    for (auto& f : futures) {
        results.push_back(f.get());
    }

    auto t_end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();

    std::cout << "Processed " << results.size() << " frames in " << ms << "ms\n";
    std::cout << "Expected sequential time: " << frames.size() * 20 << "ms\n";
    std::cout << "Speedup: ~" << (frames.size() * 20) / std::max(ms, (long)1) << "x\n";
}

// ─── Ví dụ 3: Exception handling trong thread pool ───────────────────────
void exception_handling_demo() {
    std::cout << "\n=== Exception Handling in Thread Pool ===\n";

    ThreadPool pool(2);

    // Task có thể throw
    auto f1 = pool.submit([]() -> std::string {
        return "OK";
    });

    auto f2 = pool.submit([]() -> std::string {
        throw std::runtime_error("Sensor calibration failed!");
        return "never";
    });

    try {
        std::cout << "f1: " << f1.get() << "\n";
    } catch (const std::exception& e) {
        std::cout << "f1 error: " << e.what() << "\n";
    }

    try {
        std::cout << "f2: " << f2.get() << "\n";  // exception được rethrow ở đây
    } catch (const std::exception& e) {
        std::cout << "f2 exception caught: " << e.what() << "\n";
    }
}

int main() {
    basic_usage_demo();
    parallel_processing_demo();
    exception_handling_demo();
    return 0;
}

/**
 * ═══ BÀI TẬP TỰ LÀM ═══════════════════════════════════════════════════════
 *
 * 1. Thêm `priority queue` vào ThreadPool:
 *    - Task có priority cao được thực thi trước
 *    - Dùng std::priority_queue thay vì std::queue
 *    Ứng dụng: safety-critical callbacks > normal callbacks
 *
 * 2. Thêm `submit_after(delay, func)` — execute task sau delay milliseconds.
 *    Dùng một timer thread riêng, schedule task vào pool sau delay.
 *
 * 3. `parallel_for(begin, end, func, chunk_size)`:
 *    - Chia range [begin, end) thành chunks
 *    - Submit mỗi chunk lên ThreadPool
 *    - Wait tất cả futures
 *    Test với: parallel_for(0, 1000000, [](int i){ return i*i; }, 10000)
 *
 * ═══ LIÊN KẾT ROS2 ═════════════════════════════════════════════════════════
 *
 * ROS2 MultiThreadedExecutor:
 *   rclcpp::executors::MultiThreadedExecutor executor(
 *     rclcpp::ExecutorOptions(), num_threads);
 *   executor.add_node(node);
 *   executor.spin();  // ← dùng thread pool bên trong
 *
 * Callback Groups:
 *   // Reentrant: nhiều threads có thể chạy cùng lúc
 *   auto group = node->create_callback_group(
 *     rclcpp::CallbackGroupType::Reentrant);
 *
 *   // MutuallyExclusive: chỉ 1 callback trong group tại một thời điểm
 *   auto exclusive_group = node->create_callback_group(
 *     rclcpp::CallbackGroupType::MutuallyExclusive);
 */
