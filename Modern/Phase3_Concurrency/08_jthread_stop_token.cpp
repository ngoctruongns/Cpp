/**
 * PHASE 3 - Bài 08: std::jthread & stop_token (C++20)
 *
 * Mục tiêu:
 *  - jthread: auto-join khi destructor, không cần join() thủ công
 *  - stop_source / stop_token: cooperative cancellation
 *  - stop_callback: cleanup khi cancellation requested
 *  - So sánh jthread vs thread cho RAII patterns
 *
 * Compile: g++ -std=c++20 -pthread 08_jthread_stop_token.cpp -o out
 */

#include <iostream>
#include <thread>
#include <stop_token>
#include <chrono>
#include <vector>
#include <atomic>
#include <string>
#include <mutex>

using namespace std::chrono_literals;

// ─── Ví dụ 1: jthread auto-join ──────────────────────────────────────────
void demo_jthread_auto_join() {
    std::cout << "\n=== jthread: auto-join on destruction ===\n";
    {
        // std::thread would terminate() if not joined before destructor!
        std::jthread t([]() {
            std::cout << "[jthread] running\n";
            std::this_thread::sleep_for(50ms);
            std::cout << "[jthread] done\n";
        });
        std::cout << "[main] jthread created, going out of scope...\n";
    }
    // jthread destructor calls join() automatically
    std::cout << "[main] jthread joined automatically\n";
}

// ─── Ví dụ 2: jthread with stop_token ────────────────────────────────────
void demo_stop_token_basic() {
    std::cout << "\n=== stop_token: cooperative cancellation ===\n";

    // jthread passes stop_token as FIRST arg if function accepts it
    std::jthread worker([](std::stop_token stoken) {
        int tick = 0;
        while (!stoken.stop_requested()) {
            std::cout << "  [worker] tick " << ++tick << "\n";
            std::this_thread::sleep_for(30ms);
        }
        std::cout << "  [worker] cancellation requested — exiting cleanly\n";
    });

    std::this_thread::sleep_for(120ms);
    // request_stop() — signals the stop_token
    worker.request_stop();
    // destructor will join (stop already requested)
    std::cout << "[main] stop requested\n";
}

// ─── Ví dụ 3: stop_source / stop_token sharing ────────────────────────────
// Multiple threads sharing the same stop_source
void demo_shared_stop_source() {
    std::cout << "\n=== Shared stop_source across multiple jthreads ===\n";

    std::stop_source stop_src;
    std::mutex print_mtx;

    auto make_worker = [&](int id) {
        return std::jthread([&, id](std::stop_token st) {
            while (!st.stop_requested()) {
                {
                    std::lock_guard lk(print_mtx);
                    std::cout << "  [worker " << id << "] alive\n";
                }
                std::this_thread::sleep_for(25ms);
            }
            std::lock_guard lk(print_mtx);
            std::cout << "  [worker " << id << "] stopped\n";
        });
    };

    // All workers share the same stop_source
    std::vector<std::jthread> workers;
    for (int i = 0; i < 3; ++i) {
        workers.emplace_back(make_worker(i));
        workers.back().get_stop_source().swap(stop_src);    // share the source
    }

    // Give our own stop_source to each worker
    std::vector<std::jthread> ws;
    for (int i = 0; i < 3; ++i) {
        ws.emplace_back([&stop_src, &print_mtx, i](std::stop_token) {
            auto st2 = stop_src.get_token();
            while (!st2.stop_requested()) {
                {
                    std::lock_guard lk(print_mtx);
                    std::cout << "  [ws " << i << "] alive\n";
                }
                std::this_thread::sleep_for(20ms);
            }
            std::lock_guard lk(print_mtx);
            std::cout << "  [ws " << i << "] exited\n";
        });
    }

    std::this_thread::sleep_for(80ms);
    std::cout << "[main] stopping all workers\n";
    stop_src.request_stop();  // signals ALL workers sharing this source

    for (auto& t : ws) t.join();
}

// ─── Ví dụ 4: stop_callback — cleanup on cancellation ────────────────────
void demo_stop_callback() {
    std::cout << "\n=== stop_callback for cleanup ===\n";

    std::jthread worker([](std::stop_token stoken) {
        // Register cleanup to run when stop is requested
        // (runs in the thread that calls request_stop, not this thread!)
        std::stop_callback cleanup(stoken, []() {
            std::cout << "  [stop_callback] cleanup: closing sensor connection\n";
        });

        std::cout << "  [worker] initializing sensor...\n";
        std::this_thread::sleep_for(20ms);
        std::cout << "  [worker] sensor ready, processing...\n";

        while (!stoken.stop_requested()) {
            std::this_thread::sleep_for(15ms);
        }
        std::cout << "  [worker] stop_requested — exiting\n";
    });

    std::this_thread::sleep_for(60ms);
    std::cout << "[main] requesting stop\n";
    worker.request_stop();  // also triggers the stop_callback
}

// ─── Ví dụ 5: ROS2-style jthread node simulation ─────────────────────────
class SensorNode {
    std::string name_;
    std::jthread spin_thread_;
    std::atomic<int> count_{0};

public:
    explicit SensorNode(const std::string& name) : name_(name) {
        // jthread automatically passes stop_token!
        spin_thread_ = std::jthread([this](std::stop_token st) {
            spin_loop(st);
        });
        std::cout << "[" << name_ << "] node started\n";
    }

    ~SensorNode() {
        // jthread destructor: request_stop() + join() automatically
        std::cout << "[" << name_ << "] node destroyed (auto-stop-join)\n";
    }

    int message_count() const { return count_.load(); }

private:
    void spin_loop(std::stop_token st) {
        while (!st.stop_requested()) {
            ++count_;
            std::this_thread::sleep_for(10ms);
        }
        std::cout << "[" << name_ << "] spin stopped, processed "
                  << count_.load() << " msgs\n";
    }
};

void demo_ros2_jthread() {
    std::cout << "\n=== ROS2-style SensorNode with jthread ===\n";
    {
        SensorNode lidar("lidar_node");
        SensorNode imu("imu_node");

        std::this_thread::sleep_for(80ms);
        std::cout << "[main] nodes going out of scope...\n";
    }
    // Both nodes auto-stop and auto-join here
    std::cout << "[main] all nodes stopped\n";
}

int main() {
    demo_jthread_auto_join();
    demo_stop_token_basic();
    demo_stop_callback();
    demo_ros2_jthread();

    // shared stop source test (simpler version)
    std::cout << "\n=== Bonus: jthread with shared stop token ===\n";
    std::stop_source src;
    std::jthread w1([](std::stop_token st) {
        while (!st.stop_requested()) std::this_thread::sleep_for(10ms);
        std::cout << "  w1 stopped\n";
    });
    w1.get_stop_source().swap(src);
    std::jthread w2([&src](std::stop_token) {
        auto st = src.get_token();
        while (!st.stop_requested()) std::this_thread::sleep_for(10ms);
        std::cout << "  w2 stopped\n";
    });
    std::this_thread::sleep_for(50ms);
    src.request_stop();
    w1.join();
    w2.join();

    return 0;
}
