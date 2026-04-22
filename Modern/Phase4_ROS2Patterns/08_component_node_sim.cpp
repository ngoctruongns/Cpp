/**
 * PHASE 4 - Bài 08: Component Node Simulation (ROS2 Composition)
 *
 * Mục tiêu:
 *  - Component pattern: nodes được load vào cùng một process
 *  - Intra-process communication (không qua network/DDS)
 *  - NodeFactory: tạo node từ string tên
 *  - Container: quản lý nhiều components
 *
 * Compile: g++ -std=c++17 -pthread 08_component_node_sim.cpp -o out
 */

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <stdexcept>

// ── C++17 compatible jthread/stop_token ─────────────────────────────────
struct StopToken {
    std::shared_ptr<std::atomic<bool>> flag_;
    explicit StopToken(std::shared_ptr<std::atomic<bool>> f) : flag_(std::move(f)) {}
    bool stop_requested() const { return flag_->load(std::memory_order_relaxed); }
};

class JThread {
    std::thread                        t_;
    std::shared_ptr<std::atomic<bool>> flag_;
public:
    JThread() : flag_(std::make_shared<std::atomic<bool>>(false)) {}
    template<typename F>
    explicit JThread(F&& fn) : flag_(std::make_shared<std::atomic<bool>>(false)) {
        auto f = flag_;
        t_ = std::thread([fn=std::forward<F>(fn), f]() mutable { fn(StopToken{f}); });
    }
    JThread(JThread&&) = default;
    JThread& operator=(JThread&&) = default;
    JThread(const JThread&) = delete;
    JThread& operator=(const JThread&) = delete;
    ~JThread() { if (t_.joinable()) { flag_->store(true); t_.join(); } }
    void request_stop() { flag_->store(true, std::memory_order_relaxed); }
    void join() { if (t_.joinable()) t_.join(); }
};
// ────────────────────────────────────────────────────────────────────────

using namespace std::chrono_literals;

// ──────────────────────────────────────────────────────────────────────────
// PART 1: Intra-process topic bus
// ──────────────────────────────────────────────────────────────────────────
template<typename T>
class IntraTopicBus {
    struct Sub {
        int id;
        std::function<void(const T&)> cb;
    };

    std::vector<Sub>    subs_;
    std::mutex          mtx_;
    std::string         topic_;
    int                 next_id_ = 0;
    std::atomic<int>    msg_count_{0};

public:
    explicit IntraTopicBus(const std::string& t) : topic_(t) {}

    int subscribe(std::function<void(const T&)> cb) {
        std::lock_guard<std::mutex> lk(mtx_);
        int id = next_id_++;
        subs_.push_back({id, std::move(cb)});
        return id;
    }

    void unsubscribe(int id) {
        std::lock_guard<std::mutex> lk(mtx_);
        subs_.erase(std::remove_if(subs_.begin(), subs_.end(),
            [id](const Sub& s){ return s.id == id; }), subs_.end());
    }

    void publish(const T& msg) {
        std::vector<std::function<void(const T&)>> cbs;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            for (auto& s : subs_) cbs.push_back(s.cb);
        }
        for (auto& cb : cbs) cb(msg);
        ++msg_count_;
    }

    int sub_count() {
        std::lock_guard<std::mutex> lk(mtx_);
        return static_cast<int>(subs_.size());
    }

    const std::string& topic() const { return topic_; }
    int msg_count() const { return msg_count_.load(); }
};

// ──────────────────────────────────────────────────────────────────────────
// PART 2: Component base class
// ──────────────────────────────────────────────────────────────────────────
class ComponentContainer;  // forward

class ComponentNode {
    std::string name_;
protected:
    ComponentContainer* container_ = nullptr;  // injected by container

public:
    using SharedPtr = std::shared_ptr<ComponentNode>;
    using Ptr = std::unique_ptr<ComponentNode>;

    explicit ComponentNode(const std::string& name) : name_(name) {}
    virtual ~ComponentNode() = default;

    virtual void on_init()  {}
    virtual void on_start() {}
    virtual void on_stop()  {}

    const std::string& get_name() const { return name_; }
    void set_container(ComponentContainer* c) { container_ = c; }
};

// ──────────────────────────────────────────────────────────────────────────
// PART 3: Message types
// ──────────────────────────────────────────────────────────────────────────
struct ScanMsg { float range; int seq; std::string frame_id; };
struct CmdVel  { double linear; double angular; };
struct Pose2D  { double x; double y; double theta; };

// ──────────────────────────────────────────────────────────────────────────
// PART 4: Component Container
// ──────────────────────────────────────────────────────────────────────────
class ComponentContainer {
    std::unordered_map<std::string, ComponentNode::Ptr> components_;
    std::unordered_map<std::string, std::shared_ptr<void>> topic_buses_;
    mutable std::mutex component_mtx_;
    mutable std::mutex topic_mtx_;
    std::atomic<bool>  running_{false};
    std::string        name_;

public:
    explicit ComponentContainer(const std::string& name) : name_(name) {
        std::cout << "[Container:" << name_ << "] created\n";
    }

    ~ComponentContainer() {
        stop();
        std::cout << "[Container:" << name_ << "] destroyed\n";
    }

    // Add component to the container
    void add_component(ComponentNode::Ptr node) {
        std::lock_guard<std::mutex> lk(component_mtx_);
        node->set_container(this);
        node->on_init();
        std::cout << "[Container:" << name_ << "] loaded component '"
                  << node->get_name() << "'\n";
        components_[node->get_name()] = std::move(node);
    }

    // Create or get intra-process topic bus
    template<typename T>
    std::shared_ptr<IntraTopicBus<T>> get_topic(const std::string& topic) {
        std::lock_guard<std::mutex> lk(topic_mtx_);
        auto it = topic_buses_.find(topic);
        if (it != topic_buses_.end()) {
            return std::static_pointer_cast<IntraTopicBus<T>>(it->second);
        }
        auto bus = std::make_shared<IntraTopicBus<T>>(topic);
        topic_buses_[topic] = bus;
        std::cout << "[Container] created intra-process topic: " << topic << "\n";
        return bus;
    }

    void start() {
        running_ = true;
        std::lock_guard<std::mutex> lk(component_mtx_);
        for (auto& [name, c] : components_) {
            c->on_start();
        }
        std::cout << "[Container:" << name_ << "] started with "
                  << components_.size() << " components\n";
    }

    void stop() {
        if (!running_.exchange(false)) return;
        std::lock_guard<std::mutex> lk(component_mtx_);
        for (auto& [name, c] : components_) {
            c->on_stop();
        }
        std::cout << "[Container:" << name_ << "] stopped\n";
    }

    bool is_running() const { return running_.load(); }

    void print_status() const {
        std::lock_guard<std::mutex> lk1(component_mtx_);
        std::lock_guard<std::mutex> lk2(topic_mtx_);
        std::cout << "\n[Container:" << name_ << "] Status:\n";
        std::cout << "  Components (" << components_.size() << "):\n";
        for (const auto& [n, _] : components_)
            std::cout << "    - " << n << "\n";
        std::cout << "  Topics (" << topic_buses_.size() << "):\n";
        for (const auto& [n, _] : topic_buses_)
            std::cout << "    - " << n << "\n";
    }
};

// ──────────────────────────────────────────────────────────────────────────
// PART 5: Concrete components
// ──────────────────────────────────────────────────────────────────────────
class LidarPublisher : public ComponentNode {
    std::shared_ptr<IntraTopicBus<ScanMsg>> pub_;
    JThread scan_thread_;
    int seq_ = 0;

public:
    explicit LidarPublisher(const std::string& n) : ComponentNode(n) {}

    void on_init() override {
        pub_ = container_->get_topic<ScanMsg>("/scan");
        std::cout << "  [" << get_name() << "] on_init\n";
    }

    void on_start() override {
        std::cout << "  [" << get_name() << "] on_start\n";
        scan_thread_ = JThread([this](StopToken st) {
            while (!st.stop_requested()) {
                ScanMsg msg{3.14f + seq_ * 0.1f, seq_++, "laser_frame"};
                pub_->publish(msg);
                std::this_thread::sleep_for(50ms);
            }
        });
    }

    void on_stop() override {
        scan_thread_.request_stop();
        scan_thread_.join();
        std::cout << "  [" << get_name() << "] stopped, seq=" << seq_ << "\n";
    }
};

class ScanProcessor : public ComponentNode {
    std::shared_ptr<IntraTopicBus<ScanMsg>>  scan_sub_;
    std::shared_ptr<IntraTopicBus<Pose2D>>   pose_pub_;
    int sub_id_ = -1;
    std::atomic<int> processed_{0};

public:
    explicit ScanProcessor(const std::string& n) : ComponentNode(n) {}

    void on_init() override {
        scan_sub_ = container_->get_topic<ScanMsg>("/scan");
        pose_pub_ = container_->get_topic<Pose2D>("/pose");
        std::cout << "  [" << get_name() << "] on_init\n";
    }

    void on_start() override {
        sub_id_ = scan_sub_->subscribe([this](const ScanMsg& msg) {
            // Intra-process: direct function call, no serialization!
            Pose2D pose{msg.range * 0.5, 0.0, 0.0};
            pose_pub_->publish(pose);
            ++processed_;
            if (processed_ % 3 == 0) {
                std::cout << "  [" << get_name() << "] processed=" << processed_
                          << " range=" << msg.range << "\n";
            }
        });
        std::cout << "  [" << get_name() << "] on_start, sub_id=" << sub_id_ << "\n";
    }

    void on_stop() override {
        scan_sub_->unsubscribe(sub_id_);
        std::cout << "  [" << get_name() << "] stopped, processed="
                  << processed_ << "\n";
    }
};

class PoseLogger : public ComponentNode {
    std::shared_ptr<IntraTopicBus<Pose2D>> pose_sub_;
    int sub_id_ = -1;
    std::atomic<int> logged_{0};

public:
    explicit PoseLogger(const std::string& n) : ComponentNode(n) {}

    void on_init() override {
        pose_sub_ = container_->get_topic<Pose2D>("/pose");
        std::cout << "  [" << get_name() << "] on_init\n";
    }

    void on_start() override {
        sub_id_ = pose_sub_->subscribe([this](const Pose2D& p) {
            ++logged_;
            if (logged_ % 3 == 0) {
                std::cout << "  [" << get_name() << "] pose x="
                          << p.x << " y=" << p.y << "\n";
            }
        });
        std::cout << "  [" << get_name() << "] on_start\n";
    }

    void on_stop() override {
        pose_sub_->unsubscribe(sub_id_);
        std::cout << "  [" << get_name() << "] stopped, logged="
                  << logged_ << "\n";
    }
};

int main() {
    std::cout << "===== Component Node Composition =====\n\n";

    ComponentContainer container("main_container");

    container.add_component(std::make_unique<LidarPublisher>("lidar_publisher"));
    container.add_component(std::make_unique<ScanProcessor>("scan_processor"));
    container.add_component(std::make_unique<PoseLogger>("pose_logger"));

    container.print_status();

    std::cout << "\n--- Starting container ---\n";
    container.start();

    // Let it run for 200ms
    std::this_thread::sleep_for(200ms);

    std::cout << "\n--- Stopping container ---\n";
    container.stop();

    std::cout << "\nDone!\n";
    return 0;
}
