/**
 * PHASE 4 - Bài 02: Observer Pattern (thread-safe topic pub/sub)
 *
 * Mục tiêu:
 *  - Observer pattern: tách Publisher khỏi Subscriber
 *  - Thread-safe subscription: weak_ptr tránh dangling callbacks
 *  - Giống ROS2 topic system: publish → dispatch callbacks
 *
 * Compile: g++ -std=c++17 -pthread 02_observer_pattern.cpp -o out
 */

#include <iostream>
#include <functional>
#include <vector>
#include <memory>
#include <mutex>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <atomic>

using namespace std::chrono_literals;

// ──────────────────────────────────────────────────────────────────────────
// Generic thread-safe Event Bus (topic-based pub/sub)
// ──────────────────────────────────────────────────────────────────────────

// Subscription handle — keeps subscription alive; when destroyed, auto-unsubscribes
class Subscription {
public:
    using Ptr = std::shared_ptr<Subscription>;
    explicit Subscription(std::function<void()> unsubscribe)
        : unsubscribe_(std::move(unsubscribe)) {}
    ~Subscription() { if (unsubscribe_) unsubscribe_(); }

    // Non-copyable, movable
    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
private:
    std::function<void()> unsubscribe_;
};

template<typename T>
class Publisher {
    struct SubscriberEntry {
        int                             id;
        std::weak_ptr<Subscription>     handle;
        std::function<void(const T&)>   callback;
    };

    std::vector<SubscriberEntry> subscribers_;
    mutable std::mutex           mtx_;
    int                          next_id_ = 0;
    std::string                  topic_;

public:
    explicit Publisher(const std::string& topic) : topic_(topic) {}

    // Subscribe: returns a Subscription handle; when handle is destroyed → auto-unsubscribe
    [[nodiscard]] Subscription::Ptr subscribe(std::function<void(const T&)> cb) {
        std::lock_guard<std::mutex> lock(mtx_);
        int id = next_id_++;

        // Create subscription that removes itself when destroyed
        auto sub = std::make_shared<Subscription>([this, id]() {
            std::lock_guard<std::mutex> lk(mtx_);
            subscribers_.erase(
                std::remove_if(subscribers_.begin(), subscribers_.end(),
                               [id](const SubscriberEntry& e){ return e.id == id; }),
                subscribers_.end()
            );
            std::cout << "[Publisher:" << topic_ << "] subscription #"
                      << id << " removed\n";
        });

        subscribers_.push_back({id, sub, std::move(cb)});
        std::cout << "[Publisher:" << topic_ << "] subscriber #" << id << " added\n";
        return sub;
    }

    // Publish: call all alive subscribers
    void publish(const T& msg) {
        std::vector<std::function<void(const T&)>> to_call;

        {
            std::lock_guard<std::mutex> lock(mtx_);
            // Remove expired subscriptions, collect live callbacks
            subscribers_.erase(
                std::remove_if(subscribers_.begin(), subscribers_.end(),
                               [](const SubscriberEntry& e){ return e.handle.expired(); }),
                subscribers_.end()
            );
            for (auto& entry : subscribers_) to_call.push_back(entry.callback);
        }

        // Call outside lock to avoid lock inversion
        for (auto& cb : to_call) cb(msg);
    }

    size_t subscriber_count() const {
        std::lock_guard<std::mutex> lock(mtx_);
        return subscribers_.size();
    }

    const std::string& topic() const { return topic_; }
};

// ──────────────────────────────────────────────────────────────────────────
// Demo messages
// ──────────────────────────────────────────────────────────────────────────
struct ScanMsg {
    float range;
    int   seq;
};

struct CmdVelMsg {
    double linear;
    double angular;
};

// ──────────────────────────────────────────────────────────────────────────
// Demo 1: Basic subscribe → publish → auto-unsubscribe
// ──────────────────────────────────────────────────────────────────────────
void demo_basic_pubsub() {
    std::cout << "\n=== Basic Pub/Sub ===\n";
    Publisher<ScanMsg> scan_pub("/scan");

    // Subscribe A
    auto sub_a = scan_pub.subscribe([](const ScanMsg& msg) {
        std::cout << "  [A] received seq=" << msg.seq << " range=" << msg.range << "\n";
    });

    // Subscribe B
    auto sub_b = scan_pub.subscribe([](const ScanMsg& msg) {
        std::cout << "  [B] received seq=" << msg.seq << "\n";
    });

    std::cout << "Subscribers: " << scan_pub.subscriber_count() << "\n";

    scan_pub.publish({3.14f, 1});
    scan_pub.publish({2.71f, 2});

    // B goes out of scope → auto-unsubscribe
    std::cout << "\n[main] sub_b going out of scope\n";
    sub_b.reset();

    std::cout << "Subscribers: " << scan_pub.subscriber_count() << "\n";
    scan_pub.publish({1.0f, 3});  // only A receives
}

// ──────────────────────────────────────────────────────────────────────────
// Demo 2: ROS2-style Node with subscriber members
// ──────────────────────────────────────────────────────────────────────────
class LidarProcessor {
    Subscription::Ptr scan_sub_;
    int count_ = 0;

public:
    explicit LidarProcessor(Publisher<ScanMsg>& pub) {
        scan_sub_ = pub.subscribe([this](const ScanMsg& msg) {
            on_scan(msg);
        });
        std::cout << "[LidarProcessor] created\n";
    }

    ~LidarProcessor() {
        // scan_sub_ destruction → auto-unsubscribe
        std::cout << "[LidarProcessor] destroyed (" << count_ << " msgs processed)\n";
    }

private:
    void on_scan(const ScanMsg& msg) {
        ++count_;
        std::cout << "  [LidarProcessor] scan #" << count_
                  << " range=" << msg.range << "\n";
    }
};

void demo_node_lifetime() {
    std::cout << "\n=== Node Lifetime — auto-unsubscribe ===\n";
    Publisher<ScanMsg> pub("/scan");

    {
        LidarProcessor node(pub);
        std::cout << "Subscribers: " << pub.subscriber_count() << "\n";
        pub.publish({1.0f, 1});
        pub.publish({2.0f, 2});
        std::cout << "[main] node going out of scope\n";
    }
    // node destroyed → subscription removed
    std::cout << "Subscribers after node destruction: "
              << pub.subscriber_count() << "\n";
    pub.publish({3.0f, 3});  // no one receives
}

// ──────────────────────────────────────────────────────────────────────────
// Demo 3: Thread-safe publish from multiple threads
// ──────────────────────────────────────────────────────────────────────────
void demo_threaded_pubsub() {
    std::cout << "\n=== Thread-safe Pub/Sub ===\n";

    Publisher<ScanMsg> pub("/scan_mt");
    std::mutex print_mtx;
    std::atomic<int> received{0};

    // Two subscribers
    auto sub1 = pub.subscribe([&](const ScanMsg& msg) {
        ++received;
        std::lock_guard<std::mutex> lk(print_mtx);
        std::cout << "  [sub1] seq=" << msg.seq << "\n";
    });
    auto sub2 = pub.subscribe([&](const ScanMsg& msg) {
        ++received;
        std::lock_guard<std::mutex> lk(print_mtx);
        std::cout << "  [sub2] seq=" << msg.seq << "\n";
    });

    // Two publisher threads
    std::thread t1([&pub]() {
        for (int i = 0; i < 3; ++i) {
            pub.publish({static_cast<float>(i), i});
            std::this_thread::sleep_for(10ms);
        }
    });
    std::thread t2([&pub]() {
        for (int i = 10; i < 13; ++i) {
            pub.publish({static_cast<float>(i), i});
            std::this_thread::sleep_for(10ms);
        }
    });

    t1.join(); t2.join();
    std::cout << "Total received: " << received << " (expected 12: 6 msgs × 2 subs)\n";
}

int main() {
    demo_basic_pubsub();
    demo_node_lifetime();
    demo_threaded_pubsub();
    return 0;
}
