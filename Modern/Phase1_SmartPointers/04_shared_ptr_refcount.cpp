/**
 * PHASE 1 - Bài 04: shared_ptr & weak_ptr — Shared Ownership
 *
 * Mục tiêu:
 *  - Hiểu reference counting của shared_ptr
 *  - Biết khi nào dùng shared_ptr vs unique_ptr
 *  - Giải quyết circular reference bằng weak_ptr
 *  - enable_shared_from_this (pattern cốt lõi của ROS2 Node)
 *
 * Compile: g++ -std=c++17 04_shared_ptr_refcount.cpp -o out
 */

#include <iostream>
#include <memory>
#include <vector>
#include <string>

// ─── Ví dụ 1: shared_ptr cơ bản & use_count ──────────────────────────────
void shared_ptr_basics() {
    std::cout << "\n=== shared_ptr Basics ===\n";

    auto sp1 = std::make_shared<int>(100);
    std::cout << "sp1 value: " << *sp1 << ", use_count: " << sp1.use_count() << "\n";

    {
        auto sp2 = sp1;  // Copy → tăng ref count lên 2
        std::cout << "After sp2=sp1, use_count: " << sp1.use_count() << "\n";

        auto sp3 = sp1;  // ref count = 3
        std::cout << "After sp3=sp1, use_count: " << sp1.use_count() << "\n";
    }
    // sp2, sp3 ra scope → destroy → ref count về 1
    std::cout << "After scope, use_count: " << sp1.use_count() << "\n";
    // sp1 ra scope → ref count = 0 → object bị destroy
}

// ─── Ví dụ 2: Chia sẻ sensor giữa nhiều node (như ROS2) ─────────────────
struct SensorReading {
    double value;
    uint64_t timestamp_ns;
    std::string frame_id;
};

class SensorBuffer {
public:
    std::vector<SensorReading> readings;
    explicit SensorBuffer(const std::string& sensor_name) : name_(sensor_name) {
        std::cout << "[+] SensorBuffer '" << name_ << "' created\n";
    }
    ~SensorBuffer() {
        std::cout << "[-] SensorBuffer '" << name_ << "' destroyed\n";
    }
    void add(double v) { readings.push_back({v, 0, name_}); }
    const std::string& name() const { return name_; }
private:
    std::string name_;
};

class ProcessorNode {
public:
    std::string node_name;
    std::shared_ptr<SensorBuffer> buffer;  // shared ownership

    ProcessorNode(const std::string& n, std::shared_ptr<SensorBuffer> b)
        : node_name(n), buffer(std::move(b)) {}

    void process() {
        std::cout << "[" << node_name << "] Processing "
                  << buffer->readings.size() << " readings from "
                  << buffer->name() << "\n";
    }
};

void shared_sensor_buffer_demo() {
    std::cout << "\n=== Shared Sensor Buffer ===\n";

    // Buffer được tạo một lần, chia sẻ cho nhiều processor
    auto buffer = std::make_shared<SensorBuffer>("lidar_front");
    buffer->add(1.5); buffer->add(2.3); buffer->add(0.9);

    // Nhiều nodes cùng share buffer này (như subscriber nodes trong ROS2)
    ProcessorNode mapper("mapping_node", buffer);
    ProcessorNode planner("planning_node", buffer);
    ProcessorNode viz("rviz_node", buffer);

    std::cout << "Buffer use_count: " << buffer.use_count() << "\n";  // 4

    mapper.process();
    planner.process();
    viz.process();
}

// ─── Ví dụ 3: Circular Reference → memory leak ───────────────────────────
struct NodeWithRawPtr;
// Nếu dùng shared_ptr → circular ref → leak
struct BadParent {
    std::string name;
    std::shared_ptr<struct BadChild> child;
    BadParent(const std::string& n) : name(n) {
        std::cout << "[+] BadParent '" << name << "'\n";
    }
    ~BadParent() { std::cout << "[-] BadParent '" << name << "'\n"; }
};

struct BadChild {
    std::string name;
    std::shared_ptr<BadParent> parent;  // ← CÁI NÀY GÂY CIRCULAR REF
    BadChild(const std::string& n) : name(n) {
        std::cout << "[+] BadChild '" << name << "'\n";
    }
    ~BadChild() { std::cout << "[-] BadChild '" << name << "'\n"; }
};

void circular_ref_leak_demo() {
    std::cout << "\n=== CIRCULAR REF LEAK (destructors KHÔNG được gọi!) ===\n";
    auto parent = std::make_shared<BadParent>("robot");
    auto child  = std::make_shared<BadChild>("arm");
    parent->child = child;
    child->parent = parent;  // tạo vòng tròn!
    // Khi ra scope: parent ref count = 2 (main + child->parent)
    //               child ref count = 2 (main + parent->child)
    // → không về 0 → memory leak!
    std::cout << "parent use_count: " << parent.use_count() << "\n";
    std::cout << "child use_count: " << child.use_count() << "\n";
    // Destructors sẽ KHÔNG được gọi!
}

// ─── Ví dụ 4: Fix circular ref bằng weak_ptr ─────────────────────────────
struct GoodParent {
    std::string name;
    std::shared_ptr<struct GoodChild> child;
    GoodParent(const std::string& n) : name(n) {
        std::cout << "[+] GoodParent '" << name << "'\n";
    }
    ~GoodParent() { std::cout << "[-] GoodParent '" << name << "'\n"; }
};
struct GoodChild {
    std::string name;
    std::weak_ptr<GoodParent> parent;  // ← weak_ptr: không tăng ref count
    GoodChild(const std::string& n) : name(n) {
        std::cout << "[+] GoodChild '" << name << "'\n";
    }
    ~GoodChild() { std::cout << "[-] GoodChild '" << name << "'\n"; }

    void print_parent_name() {
        // Phải lock() để truy cập an toàn
        if (auto p = parent.lock()) {  // lock() → shared_ptr hoặc nullptr
            std::cout << "My parent is: " << p->name << "\n";
        } else {
            std::cout << "Parent no longer exists!\n";
        }
    }
};

void circular_ref_fixed_demo() {
    std::cout << "\n=== CIRCULAR REF FIXED (weak_ptr) ===\n";
    {
        auto parent = std::make_shared<GoodParent>("robot");
        auto child  = std::make_shared<GoodChild>("arm");
        parent->child = child;
        child->parent = parent;  // weak_ptr → ref count KHÔNG tăng

        std::cout << "parent use_count: " << parent.use_count() << "\n"; // 1
        std::cout << "child use_count: " << child.use_count() << "\n";   // 2

        child->print_parent_name();
    }
    // Cả hai destructors đều được gọi!
    // Sau khi parent bị destroy, weak_ptr.lock() trả về nullptr
}

// ─── Ví dụ 5: enable_shared_from_this — Pattern của rclcpp::Node ─────────
class RobotNode : public std::enable_shared_from_this<RobotNode> {
public:
    std::string name;

    explicit RobotNode(const std::string& n) : name(n) {
        std::cout << "[+] RobotNode '" << name << "'\n";
    }
    ~RobotNode() {
        std::cout << "[-] RobotNode '" << name << "'\n";
    }

    // Trả về shared_ptr trỏ đến chính mình
    std::shared_ptr<RobotNode> get_shared() {
        return shared_from_this();  // thay vì shared_ptr<RobotNode>(this) ← SAI!
    }

    void register_callback(std::shared_ptr<RobotNode>& registry) {
        // Đăng ký chính mình vào registry (giống ROS2 executor)
        registry = shared_from_this();
    }
};

void enable_shared_from_this_demo() {
    std::cout << "\n=== enable_shared_from_this ===\n";

    // PHẢI tạo bằng make_shared, không được dùng raw pointer!
    auto node = std::make_shared<RobotNode>("my_robot_node");

    auto ref1 = node->get_shared();
    auto ref2 = node->get_shared();

    std::cout << "use_count: " << node.use_count() << "\n";  // 3
    std::cout << "All point to same object: "
              << (node.get() == ref1.get() ? "yes" : "no") << "\n";
}

int main() {
    shared_ptr_basics();
    shared_sensor_buffer_demo();
    circular_ref_leak_demo();
    circular_ref_fixed_demo();
    enable_shared_from_this_demo();

    return 0;
}

/**
 * ═══ BÀI TẬP TỰ LÀM ═══════════════════════════════════════════════════════
 *
 * 1. Viết class `TF2Buffer` (như trong ROS2 tf2) chứa shared_ptr<TransformData>.
 *    Nhiều "listener node" cùng share một TF2Buffer. Khi tất cả listener mất
 *    đi, TF2Buffer cũng bị destroy.
 *
 * 2. Observer Pattern dùng weak_ptr:
 *    - EventEmitter giữ vector<weak_ptr<Handler>>
 *    - Khi emit event, dùng lock() để check handler còn sống không
 *    - Handler tự động bị remove khi bị destroy (không cần unsubscribe)
 *
 * 3. Tại sao KHÔNG được dùng:
 *      shared_ptr<MyNode>(this)  // trong member function
 *    mà phải dùng:
 *      shared_from_this()
 *    Giải thích bằng diagram ref count.
 *
 * ═══ LIÊN KẾT ROS2 ═════════════════════════════════════════════════════════
 *
 * rclcpp::Node kế thừa enable_shared_from_this:
 *   class Node : public std::enable_shared_from_this<Node> { ... };
 *
 * Tạo node:
 *   auto node = std::make_shared<MyNode>("node_name");
 *   rclcpp::spin(node);  // executor giữ shared_ptr đến node
 *
 * Trong timer callback (node còn sống):
 *   auto timer_ = create_wall_timer(1s, [this]() {
 *     auto shared_this = shared_from_this(); // đảm bảo node còn sống
 *   });
 */
