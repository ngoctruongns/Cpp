/**
 * PHASE 4 - Bài 10: Mini Pub/Sub Framework (Bài tổng hợp)
 *
 * Kết hợp tất cả kỹ năng đã học để xây dựng framework pub/sub
 * giống ROS2 topic system nhưng simplified, chạy trong một process.
 *
 * Kỹ năng áp dụng:
 *  - Smart pointers (shared_ptr, weak_ptr, enable_shared_from_this)
 *  - Templates (generic topic type, variadic)
 *  - Concurrency (mutex, condition_variable, thread pool)
 *  - Modern C++ (concepts, if constexpr, move semantics)
 *
 * Compile: g++ -std=c++20 -pthread 10_mini_ros_framework.cpp -o out
 */

#include <iostream>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>
#include <typeindex>
#include <any>
#include <stdexcept>
#include <concepts>
#include <sstream>

using namespace std::chrono_literals;

// ══════════════════════════════════════════════════════════════════════════════
// PART 1: Message Types (giống ROS2 .msg files)
// ══════════════════════════════════════════════════════════════════════════════

struct Header {
    uint64_t    stamp_ns;
    std::string frame_id;
};

struct LaserScan {
    using SharedPtr = std::shared_ptr<LaserScan>;
    using ConstSharedPtr = std::shared_ptr<const LaserScan>;

    Header header;
    std::vector<float> ranges;
    float angle_min, angle_max, angle_increment;

    std::string to_string() const {
        std::ostringstream oss;
        oss << "LaserScan[frame=" << header.frame_id
            << ", n_ranges=" << ranges.size() << "]";
        return oss.str();
    }
};

struct Twist {
    using SharedPtr = std::shared_ptr<Twist>;
    struct Vec3 { double x, y, z; };
    Header header;
    Vec3 linear;
    Vec3 angular;
};

struct BatteryStatus {
    using SharedPtr = std::shared_ptr<BatteryStatus>;
    Header header;
    double voltage;
    double percentage;
    bool is_charging;
};

// ══════════════════════════════════════════════════════════════════════════════
// PART 2: Core Framework — MessageBus
// ══════════════════════════════════════════════════════════════════════════════

// Type-erased subscription handle
class SubscriptionBase {
public:
    virtual ~SubscriptionBase() = default;
    virtual void dispatch(const std::any& msg) = 0;
    std::string topic;
};

// Typed subscription
template<typename MsgT>
class Subscription : public SubscriptionBase {
public:
    using Callback = std::function<void(std::shared_ptr<const MsgT>)>;

    Subscription(std::string topic_name, Callback cb)
        : callback_(std::move(cb)) {
        topic = std::move(topic_name);
    }

    void dispatch(const std::any& msg) override {
        try {
            auto typed_msg = std::any_cast<std::shared_ptr<const MsgT>>(msg);
            callback_(typed_msg);
        } catch (const std::bad_any_cast&) {
            // Type mismatch — should not happen in correct usage
        }
    }

private:
    Callback callback_;
};

// ─── MessageBus: trung tâm routing ───────────────────────────────────────
class MessageBus {
public:
    static MessageBus& instance() {
        static MessageBus bus;
        return bus;
    }

    // Subscribe một topic với callback
    template<typename MsgT>
    std::shared_ptr<Subscription<MsgT>> subscribe(
        const std::string& topic,
        std::function<void(std::shared_ptr<const MsgT>)> callback)
    {
        auto sub = std::make_shared<Subscription<MsgT>>(topic, std::move(callback));
        std::lock_guard<std::mutex> lock(mtx_);
        subscriptions_[topic].push_back(sub);
        return sub;
    }

    // Publish message đến một topic
    template<typename MsgT>
    void publish(const std::string& topic, std::shared_ptr<const MsgT> msg) {
        std::vector<std::weak_ptr<SubscriptionBase>> subs_copy;
        {
            std::lock_guard<std::mutex> lock(mtx_);
            if (auto it = subscriptions_.find(topic); it != subscriptions_.end()) {
                // Xóa dead subscriptions (weak_ptr đã expired)
                auto& vec = it->second;
                vec.erase(
                    std::remove_if(vec.begin(), vec.end(),
                        [](const auto& wp) { return wp.expired(); }),
                    vec.end()
                );
                subs_copy = vec;
            }
        }

        // Dispatch NGOÀI lock để tránh deadlock
        std::any any_msg = msg;
        for (auto& weak_sub : subs_copy) {
            if (auto sub = weak_sub.lock()) {
                sub->dispatch(any_msg);
            }
        }
    }

    void unsubscribe(const std::string& topic) {
        std::lock_guard<std::mutex> lock(mtx_);
        subscriptions_.erase(topic);
    }

    size_t subscriber_count(const std::string& topic) const {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = subscriptions_.find(topic);
        return (it != subscriptions_.end()) ? it->second.size() : 0;
    }

private:
    MessageBus() = default;
    mutable std::mutex mtx_;
    std::unordered_map<std::string, std::vector<std::weak_ptr<SubscriptionBase>>> subscriptions_;
};

// ══════════════════════════════════════════════════════════════════════════════
// PART 3: Node base class (giống rclcpp::Node)
// ══════════════════════════════════════════════════════════════════════════════

class Node : public std::enable_shared_from_this<Node> {
public:
    explicit Node(const std::string& name) : name_(name) {
        std::cout << "[+] Node '" << name_ << "' created\n";
    }
    virtual ~Node() {
        std::cout << "[-] Node '" << name_ << "' destroyed\n";
        // Subscriptions tự động unsubscribe qua weak_ptr
    }

    const std::string& get_name() const { return name_; }

    template<typename MsgT>
    auto create_subscription(
        const std::string& topic,
        std::function<void(std::shared_ptr<const MsgT>)> callback)
    {
        auto sub = MessageBus::instance().subscribe<MsgT>(topic, std::move(callback));
        subscriptions_.push_back(sub);  // keep alive
        return sub;
    }

    template<typename MsgT>
    void publish(const std::string& topic, std::shared_ptr<const MsgT> msg) {
        MessageBus::instance().publish(topic, msg);
    }

    template<typename MsgT>
    void publish(const std::string& topic, MsgT msg) {
        publish(topic, std::make_shared<const MsgT>(std::move(msg)));
    }

protected:
    std::string name_;
    std::vector<std::shared_ptr<SubscriptionBase>> subscriptions_;
};

// ══════════════════════════════════════════════════════════════════════════════
// PART 4: Concrete Nodes (giống ROS2 node implementations)
// ══════════════════════════════════════════════════════════════════════════════

// ─── LiDAR Driver Node ────────────────────────────────────────────────────
class LidarDriverNode : public Node {
public:
    LidarDriverNode() : Node("lidar_driver"), running_(false) {}

    void start() {
        running_ = true;
        driver_thread_ = std::thread([this]() {
            uint64_t seq = 0;
            while (running_) {
                // Giả lập LiDAR data
                LaserScan scan;
                scan.header = {seq++ * 100'000'000ULL, "laser_frame"};
                scan.angle_min = -1.57f;
                scan.angle_max = 1.57f;
                scan.angle_increment = 0.01f;
                scan.ranges.resize(314, 2.0f + (seq % 10) * 0.1f);

                publish("/scan", std::move(scan));
                std::this_thread::sleep_for(100ms);  // 10Hz
            }
        });
    }

    void stop() {
        running_ = false;
        if (driver_thread_.joinable()) driver_thread_.join();
    }

    ~LidarDriverNode() { stop(); }

private:
    std::thread driver_thread_;
    std::atomic<bool> running_;
};

// ─── Navigation Node ─────────────────────────────────────────────────────
class NavigationNode : public Node {
public:
    NavigationNode() : Node("navigation") {
        // Subscribe /scan, publish /cmd_vel
        scan_sub_ = create_subscription<LaserScan>(
            "/scan",
            [this](LaserScan::ConstSharedPtr scan) {
                on_scan(scan);
            });
    }

private:
    void on_scan(LaserScan::ConstSharedPtr scan) {
        // Simple obstacle avoidance logic
        float min_dist = *std::min_element(
            scan->ranges.begin(), scan->ranges.end());

        Twist cmd;
        cmd.header = scan->header;
        if (min_dist > 1.0f) {
            cmd.linear.x = 0.5;   // move forward
            cmd.angular.z = 0.0;
        } else {
            cmd.linear.x = 0.0;   // stop
            cmd.angular.z = 0.5;  // turn
        }

        publish("/cmd_vel", std::move(cmd));
        ++scan_count_;
    }

    std::shared_ptr<Subscription<LaserScan>> scan_sub_;
    std::atomic<int> scan_count_{0};
};

// ─── Battery Monitor Node ────────────────────────────────────────────────
class BatteryMonitorNode : public Node {
public:
    BatteryMonitorNode() : Node("battery_monitor"), running_(false) {}

    void start() {
        running_ = true;
        monitor_thread_ = std::thread([this]() {
            double charge = 100.0;
            while (running_ && charge > 0) {
                BatteryStatus status;
                status.header = {0, "base_link"};
                status.voltage = 24.0 * (charge / 100.0);
                status.percentage = charge;
                status.is_charging = false;
                publish("/battery", std::move(status));

                charge -= 5.0;  // drain 5% per tick
                std::this_thread::sleep_for(200ms);
            }
        });
    }

    void stop() {
        running_ = false;
        if (monitor_thread_.joinable()) monitor_thread_.join();
    }
    ~BatteryMonitorNode() { stop(); }

private:
    std::thread monitor_thread_;
    std::atomic<bool> running_;
};

// ─── Dashboard Node — aggregate semua data ────────────────────────────────
class DashboardNode : public Node {
public:
    DashboardNode() : Node("dashboard") {
        cmd_sub_ = create_subscription<Twist>(
            "/cmd_vel",
            [this](std::shared_ptr<const Twist> cmd) {
                std::lock_guard lock(data_mtx_);
                last_cmd_ = cmd;
                ++cmd_count_;
            });

        battery_sub_ = create_subscription<BatteryStatus>(
            "/battery",
            [this](std::shared_ptr<const BatteryStatus> bat) {
                std::lock_guard lock(data_mtx_);
                last_battery_ = bat;
            });
    }

    void print_status() {
        std::lock_guard lock(data_mtx_);
        std::cout << "\n── Dashboard Status ──────────────────────────\n";
        if (last_cmd_) {
            std::cout << "  Velocity: vx=" << last_cmd_->linear.x
                      << " wz=" << last_cmd_->angular.z
                      << " (cmd count: " << cmd_count_ << ")\n";
        }
        if (last_battery_) {
            std::cout << "  Battery: " << last_battery_->percentage
                      << "% (" << last_battery_->voltage << "V)\n";
        }
        std::cout << "─────────────────────────────────────────────\n";
    }

private:
    std::shared_ptr<Subscription<Twist>> cmd_sub_;
    std::shared_ptr<Subscription<BatteryStatus>> battery_sub_;
    std::shared_ptr<const Twist> last_cmd_;
    std::shared_ptr<const BatteryStatus> last_battery_;
    mutable std::mutex data_mtx_;
    int cmd_count_ = 0;
};

// ══════════════════════════════════════════════════════════════════════════════
// PART 5: Main — wiring everything together
// ══════════════════════════════════════════════════════════════════════════════

int main() {
    std::cout << "╔══════════════════════════════════════════╗\n";
    std::cout << "║     Mini ROS2 Framework Simulation       ║\n";
    std::cout << "╚══════════════════════════════════════════╝\n\n";

    // Tạo các nodes — tất cả dùng shared_ptr (như rclcpp::make_shared)
    auto lidar     = std::make_shared<LidarDriverNode>();
    auto nav       = std::make_shared<NavigationNode>();
    auto battery   = std::make_shared<BatteryMonitorNode>();
    auto dashboard = std::make_shared<DashboardNode>();

    // Start hardware drivers
    lidar->start();
    battery->start();

    // Run for 1 second, then print status
    std::this_thread::sleep_for(500ms);
    dashboard->print_status();
    std::this_thread::sleep_for(500ms);
    dashboard->print_status();

    // Shutdown
    std::cout << "\n[System] Shutting down...\n";
    lidar->stop();
    battery->stop();

    // Subscriber count từ bus
    std::cout << "\n[System] /scan subscribers: "
              << MessageBus::instance().subscriber_count("/scan") << "\n";
    std::cout << "[System] /cmd_vel subscribers: "
              << MessageBus::instance().subscriber_count("/cmd_vel") << "\n";

    return 0;
}

/**
 * ═══ BÀI MỞ RỘNG ═══════════════════════════════════════════════════════════
 *
 * 1. Thêm QoS (Quality of Service):
 *    - RELIABLE: guarantee delivery, buffer messages
 *    - BEST_EFFORT: drop if subscriber busy
 *    - TRANSIENT_LOCAL: lưu last N messages cho subscriber mới
 *
 * 2. Thêm Service (Request/Response pattern):
 *    - ServiceServer<Req, Res>: xử lý request và trả response
 *    - ServiceClient<Req, Res>: gửi request, nhận response qua future
 *
 * 3. Thêm Parameter Server:
 *    - Node có thể đăng ký parameters với default value + validator
 *    - Parameters có thể được read/write qua một API chung
 *
 * 4. Viết unit test cho MessageBus:
 *    - Test: publish khi không có subscriber → không crash
 *    - Test: subscriber bị destroy → không nhận message
 *    - Test: concurrent publish từ nhiều threads → thread-safe
 *
 * ═══ SO SÁNH VỚI ROS2 THỰC ═════════════════════════════════════════════════
 *
 * Mini framework này:           ROS2 thực:
 *  MessageBus                →  rclcpp::Context + DDS middleware
 *  Node                      →  rclcpp::Node
 *  Subscription<T>           →  rclcpp::Subscription<T>
 *  publish(topic, msg)       →  publisher_->publish(msg)
 *  shared_ptr<const MsgT>    →  std::shared_ptr<const MsgT> (giống!)
 *  enable_shared_from_this   →  kế thừa trong rclcpp::Node (giống!)
 *  std::any cho type erasure →  rmw (ROS middleware) type erasure
 */
