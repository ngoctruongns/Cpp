/**
 * PHASE 1 - Bài 10: Robot Resource Manager (Bài tổng hợp)
 *
 * Kết hợp tất cả kiến thức Phase 1:
 *  - unique_ptr: exclusive ownership (hardware handle, plugin)
 *  - shared_ptr: shared sensor data buffer
 *  - weak_ptr: observer callback registry (không làm chủ)
 *  - enable_shared_from_this: Node tự đăng ký vào Manager
 *  - Custom deleter: giải phóng hardware resource
 *  - Move semantics: truyền ownership qua các layer
 *  - Perfect forwarding: factory functions
 *
 * Compile: g++ -std=c++17 10_robot_resource_manager.cpp -o out
 */

#include <iostream>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include <string>
#include <algorithm>
#include <stdexcept>

// ══════════════════════════════════════════════════════════════════════════
// PART 1: Hardware Abstractions (unique_ptr + custom deleter)
// ══════════════════════════════════════════════════════════════════════════

struct HWHandle {
    int    fd;
    std::string dev;
};

// Simulated hardware open/close
HWHandle* hw_open(const std::string& device) {
    static int fd_counter = 100;
    std::cout << "  [HW] open(" << device << ") → fd=" << fd_counter << "\n";
    return new HWHandle{fd_counter++, device};
}

void hw_close(HWHandle* h) {
    if (h) {
        std::cout << "  [HW] close(fd=" << h->fd << ", dev=" << h->dev << ")\n";
        delete h;
    }
}

using HWPtr = std::unique_ptr<HWHandle, decltype(&hw_close)>;

HWPtr open_device(const std::string& dev) {
    return HWPtr(hw_open(dev), &hw_close);
}

// ══════════════════════════════════════════════════════════════════════════
// PART 2: Sensor Data Buffer (shared_ptr — nhiều node share cùng data)
// ══════════════════════════════════════════════════════════════════════════

struct SensorData {
    std::string frame_id;
    double      value;
    uint64_t    stamp_ns;
};

class SensorBuffer {
public:
    explicit SensorBuffer(const std::string& name) : name_(name) {
        std::cout << "[+] SensorBuffer '" << name_ << "'\n";
    }
    ~SensorBuffer() { std::cout << "[-] SensorBuffer '" << name_ << "'\n"; }

    void push(SensorData d) { data_.push_back(std::move(d)); }
    const std::vector<SensorData>& data() const { return data_; }
    const std::string& name() const { return name_; }
    size_t size() const { return data_.size(); }

private:
    std::string              name_;
    std::vector<SensorData>  data_;
};

// ══════════════════════════════════════════════════════════════════════════
// PART 3: Robot Component Node (enable_shared_from_this)
// ══════════════════════════════════════════════════════════════════════════

class ResourceManager;  // forward

class RobotNode : public std::enable_shared_from_this<RobotNode> {
public:
    std::string node_name;

    explicit RobotNode(const std::string& name) : node_name(name) {
        std::cout << "[+] RobotNode '" << node_name << "'\n";
    }
    virtual ~RobotNode() {
        std::cout << "[-] RobotNode '" << node_name << "'\n";
    }

    // Called after make_shared — safe to use shared_from_this()
    virtual void on_init() {}
    virtual void on_tick() = 0;

    void register_to(ResourceManager& mgr);  // defined after ResourceManager
};

// ══════════════════════════════════════════════════════════════════════════
// PART 4: Resource Manager (weak_ptr observer registry)
// ══════════════════════════════════════════════════════════════════════════

class ResourceManager {
public:
    // Sensor buffers — managed as shared_ptr (shared ownership)
    std::unordered_map<std::string, std::shared_ptr<SensorBuffer>> buffers;

    // Node registry — weak_ptr (manager doesn't own nodes)
    std::vector<std::weak_ptr<RobotNode>> nodes;

    // Hardware handles — unique_ptr (exclusive ownership)
    std::vector<HWPtr> hw_handles;

    ResourceManager() { std::cout << "[+] ResourceManager\n"; }
    ~ResourceManager() { std::cout << "[-] ResourceManager\n"; }

    // Create or get a shared sensor buffer
    std::shared_ptr<SensorBuffer> get_or_create_buffer(const std::string& name) {
        auto it = buffers.find(name);
        if (it != buffers.end()) return it->second;
        auto buf = std::make_shared<SensorBuffer>(name);
        buffers[name] = buf;
        return buf;
    }

    // Register a node (weak reference)
    void add_node(std::shared_ptr<RobotNode> node) {
        nodes.push_back(node);  // store as weak_ptr
        std::cout << "[Manager] Registered node '" << node->node_name << "'\n";
    }

    // Open and own a hardware device
    void open_hw(const std::string& dev) {
        hw_handles.push_back(open_device(dev));
    }

    // Tick all alive nodes, remove expired ones
    void tick_all() {
        std::cout << "\n--- ResourceManager::tick_all() ---\n";
        nodes.erase(
            std::remove_if(nodes.begin(), nodes.end(),
                [](const std::weak_ptr<RobotNode>& wp) {
                    if (auto n = wp.lock()) {
                        n->on_tick();
                        return false;  // keep
                    }
                    std::cout << "[Manager] Node expired, removing\n";
                    return true;  // remove expired
                }),
            nodes.end()
        );
    }

    void print_status() const {
        std::cout << "\n=== ResourceManager Status ===\n";
        std::cout << "  HW handles : " << hw_handles.size() << "\n";
        std::cout << "  Buffers    : " << buffers.size() << "\n";
        std::cout << "  Nodes (alive): ";
        int alive = 0;
        for (const auto& wp : nodes) if (!wp.expired()) ++alive;
        std::cout << alive << "/" << nodes.size() << "\n";
    }
};

void RobotNode::register_to(ResourceManager& mgr) {
    mgr.add_node(shared_from_this());  // safe: already owned by shared_ptr
}

// ══════════════════════════════════════════════════════════════════════════
// PART 5: Concrete Nodes
// ══════════════════════════════════════════════════════════════════════════

class LidarNode : public RobotNode {
    std::shared_ptr<SensorBuffer> buffer_;  // shared ownership
    int tick_count_ = 0;
public:
    LidarNode(const std::string& name, std::shared_ptr<SensorBuffer> buf)
        : RobotNode(name), buffer_(std::move(buf)) {}

    void on_init() override {
        std::cout << "  [" << node_name << "] init: using buffer '"
                  << buffer_->name() << "'\n";
    }

    void on_tick() override {
        SensorData d{node_name, 3.14 + tick_count_ * 0.1,
                     static_cast<uint64_t>(tick_count_) * 1000000};
        buffer_->push(d);
        std::cout << "  [" << node_name << "] tick #" << ++tick_count_
                  << " total_data=" << buffer_->size() << "\n";
    }
};

class FusionNode : public RobotNode {
    std::shared_ptr<SensorBuffer> lidar_buf_;  // shared — same buffer as LidarNode
    std::shared_ptr<SensorBuffer> imu_buf_;
public:
    FusionNode(const std::string& name,
               std::shared_ptr<SensorBuffer> lb,
               std::shared_ptr<SensorBuffer> ib)
        : RobotNode(name), lidar_buf_(std::move(lb)), imu_buf_(std::move(ib)) {}

    void on_init() override {
        std::cout << "  [" << node_name << "] init: watching '"
                  << lidar_buf_->name() << "' + '" << imu_buf_->name() << "'\n";
    }

    void on_tick() override {
        std::cout << "  [" << node_name << "] fusing lidar("
                  << lidar_buf_->size() << ") + imu("
                  << imu_buf_->size() << ") readings\n";
    }
};

// ══════════════════════════════════════════════════════════════════════════
// MAIN
// ══════════════════════════════════════════════════════════════════════════
int main() {
    std::cout << "======= Robot Resource Manager Demo =======\n\n";

    ResourceManager mgr;

    // Open hardware devices (unique_ptr + custom deleter)
    std::cout << "--- Opening hardware ---\n";
    mgr.open_hw("/dev/lidar0");
    mgr.open_hw("/dev/imu0");

    // Create shared sensor buffers
    auto lidar_buf = mgr.get_or_create_buffer("lidar/scan");
    auto imu_buf   = mgr.get_or_create_buffer("imu/data");

    // Create nodes, init, register
    std::cout << "\n--- Creating nodes ---\n";
    {
        auto lidar_node  = std::make_shared<LidarNode>("lidar_node", lidar_buf);
        auto fusion_node = std::make_shared<FusionNode>("fusion_node",
                                                        lidar_buf, imu_buf);

        lidar_node->on_init();
        fusion_node->on_init();
        lidar_node->register_to(mgr);   // uses enable_shared_from_this
        fusion_node->register_to(mgr);

        std::cout << "\n--- lidar_buf use_count: " << lidar_buf.use_count()
                  << " (mgr + lidar_node + fusion_node + local = 4) ---\n";

        // Run 3 ticks
        mgr.tick_all();
        mgr.tick_all();
        mgr.tick_all();

        mgr.print_status();

        std::cout << "\n--- lidar_node goes out of scope ---\n";
    }

    // After scope: lidar_node and fusion_node destroyed
    // Manager's weak_ptrs now expired
    std::cout << "\n--- After scope ---\n";
    mgr.tick_all();  // expired nodes removed
    mgr.print_status();

    // lidar_buf and imu_buf still alive (shared ownership with mgr.buffers)
    std::cout << "\n--- lidar_buf use_count: " << lidar_buf.use_count()
              << " (mgr.buffers + local = 2) ---\n";

    std::cout << "\n======= Cleanup (RAII order) =======\n";
    // mgr destroyed: hw_handles (unique_ptr) → hw_close() called
    //                buffers (shared_ptr) → ref count decremented
    // lidar_buf, imu_buf local vars → ref count hits 0 → SensorBuffer destroyed
    return 0;
}
