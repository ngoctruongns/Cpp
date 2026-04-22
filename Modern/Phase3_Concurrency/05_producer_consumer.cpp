/**
 * PHASE 3 - Bài 05: Producer-Consumer Problem
 *
 * Mục tiêu:
 *  - Classic producer-consumer với condition_variable + mutex + queue
 *  - Multiple producers, multiple consumers
 *  - Graceful shutdown với stop flag
 *  - Bounded buffer (max queue size)
 *
 * Compile: g++ -std=c++17 -pthread 05_producer_consumer.cpp -o out
 */

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include <functional>

using namespace std::chrono_literals;

// ──────────────────────────────────────────────────────────────────────────
// Bounded Thread-Safe Queue
// ──────────────────────────────────────────────────────────────────────────
template<typename T>
class BoundedQueue {
    std::queue<T>           queue_;
    mutable std::mutex      mtx_;
    std::condition_variable not_empty_;      // signaled when item added
    std::condition_variable not_full_;       // signaled when item removed
    size_t                  max_size_;
    std::atomic<bool>       stopped_{false};

public:
    explicit BoundedQueue(size_t max_size = 10) : max_size_(max_size) {}

    // Push item — blocks if full, returns false if stopped
    bool push(T item) {
        std::unique_lock<std::mutex> lock(mtx_);

        // Wait until queue has space or is stopped
        not_full_.wait(lock, [this]() {
            return queue_.size() < max_size_ || stopped_.load();
        });

        if (stopped_) return false;

        queue_.push(std::move(item));
        lock.unlock();
        not_empty_.notify_one();  // wake a waiting consumer
        return true;
    }

    // Pop item — blocks if empty, returns false if stopped and empty
    bool pop(T& item) {
        std::unique_lock<std::mutex> lock(mtx_);

        // Wait until queue has item or is stopped
        not_empty_.wait(lock, [this]() {
            return !queue_.empty() || stopped_.load();
        });

        if (queue_.empty()) return false;  // stopped and empty

        item = std::move(queue_.front());
        queue_.pop();
        lock.unlock();
        not_full_.notify_one();  // wake a waiting producer
        return true;
    }

    // Signal all waiters to wake up and check stop condition
    void stop() {
        stopped_.store(true);
        not_empty_.notify_all();
        not_full_.notify_all();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return queue_.size();
    }

    bool is_stopped() const { return stopped_.load(); }
};

// ──────────────────────────────────────────────────────────────────────────
// Sensor Message (payload)
// ──────────────────────────────────────────────────────────────────────────
struct SensorMsg {
    int         producer_id;
    int         seq;
    double      value;
    std::string topic;
};

// ──────────────────────────────────────────────────────────────────────────
// Demo 1: Classic single producer, single consumer
// ──────────────────────────────────────────────────────────────────────────
void demo_single_producer_consumer() {
    std::cout << "\n=== Single Producer-Consumer ===\n";

    BoundedQueue<SensorMsg> queue(5);  // max 5 items
    std::atomic<int> consumed{0};

    // Producer
    std::thread producer([&]() {
        for (int i = 0; i < 8; ++i) {
            SensorMsg msg{0, i, i * 1.5, "/scan"};
            std::cout << "[Producer] push seq=" << i << "\n";
            if (!queue.push(std::move(msg))) break;
            std::this_thread::sleep_for(20ms);
        }
        queue.stop();
        std::cout << "[Producer] done\n";
    });

    // Consumer
    std::thread consumer([&]() {
        SensorMsg msg;
        while (queue.pop(msg)) {
            std::cout << "  [Consumer] got seq=" << msg.seq
                      << " value=" << msg.value << "\n";
            ++consumed;
            std::this_thread::sleep_for(40ms);  // slower than producer
        }
        std::cout << "  [Consumer] done, total=" << consumed << "\n";
    });

    producer.join();
    consumer.join();
}

// ──────────────────────────────────────────────────────────────────────────
// Demo 2: Multiple producers, multiple consumers
// ──────────────────────────────────────────────────────────────────────────
void demo_multi_producer_consumer() {
    std::cout << "\n=== Multi Producer-Consumer (3 producers, 2 consumers) ===\n";

    BoundedQueue<SensorMsg> queue(8);
    std::atomic<int> total_produced{0};
    std::atomic<int> total_consumed{0};
    std::mutex print_mtx;

    auto producer_fn = [&](int id, int count) {
        std::mt19937 rng(id);
        std::uniform_real_distribution<double> dist(0.0, 10.0);

        for (int i = 0; i < count; ++i) {
            SensorMsg msg{id, i, dist(rng), "/topic_" + std::to_string(id)};
            if (!queue.push(std::move(msg))) break;
            ++total_produced;
            std::this_thread::sleep_for(std::chrono::milliseconds(5 + id * 3));
        }
        {
            std::lock_guard lk(print_mtx);
            std::cout << "[Producer " << id << "] finished\n";
        }
    };

    auto consumer_fn = [&](int id) {
        SensorMsg msg;
        while (queue.pop(msg)) {
            ++total_consumed;
            std::this_thread::sleep_for(15ms);  // simulate processing
        }
        std::lock_guard lk(print_mtx);
        std::cout << "  [Consumer " << id << "] finished, consumed so far="
                  << total_consumed.load() << "\n";
    };

    std::vector<std::thread> producers, consumers;
    for (int i = 0; i < 3; ++i) producers.emplace_back(producer_fn, i, 10);
    for (int i = 0; i < 2; ++i) consumers.emplace_back(consumer_fn, i);

    // Wait for producers, then signal stop
    for (auto& t : producers) t.join();
    queue.stop();

    for (auto& t : consumers) t.join();

    std::cout << "Total produced: " << total_produced
              << ", consumed: " << total_consumed << "\n";
}

// ──────────────────────────────────────────────────────────────────────────
// Demo 3: ROS2-style callback queue simulation
// ──────────────────────────────────────────────────────────────────────────
struct CallbackItem {
    std::string topic;
    std::function<void()> callback;
};

void demo_callback_queue() {
    std::cout << "\n=== ROS2-style Callback Queue ===\n";

    BoundedQueue<CallbackItem> cbq(16);
    std::atomic<bool> spinning{true};
    std::atomic<int>  executed{0};

    // Executor thread — drains and executes callbacks
    std::thread executor([&]() {
        CallbackItem item;
        while (spinning || cbq.size() > 0) {
            if (cbq.pop(item)) {
                std::cout << "  [Executor] running callback on " << item.topic << "\n";
                item.callback();
                ++executed;
            }
        }
        std::cout << "  [Executor] stop\n";
    });

    // Simulate topics publishing → pushing callbacks
    for (int i = 0; i < 5; ++i) {
        std::string topic = "/sensor_" + std::to_string(i);
        cbq.push({topic, [topic, i]() {
            std::cout << "    → processing " << topic << " data=" << i * 1.1 << "\n";
        }});
        std::this_thread::sleep_for(10ms);
    }

    std::this_thread::sleep_for(50ms);
    spinning = false;
    cbq.stop();
    executor.join();

    std::cout << "Executed callbacks: " << executed << "\n";
}

int main() {
    demo_single_producer_consumer();
    demo_multi_producer_consumer();
    demo_callback_queue();
    return 0;
}
