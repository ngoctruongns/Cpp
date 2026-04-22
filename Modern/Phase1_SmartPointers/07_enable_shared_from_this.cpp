/**
 * PHASE 1 - Bài 07: enable_shared_from_this
 *
 * Mục tiêu:
 *  - Hiểu tại sao không thể dùng shared_ptr<T>(this) trong method
 *  - Kế thừa enable_shared_from_this để lấy shared_ptr từ this
 *  - Pattern cốt lõi của rclcpp::Node trong ROS2
 *
 * Compile: g++ -std=c++17 07_enable_shared_from_this.cpp -o out
 */

#include <iostream>
#include <memory>
#include <vector>
#include <functional>
#include <string>

// ─── Ví dụ 1: Vấn đề — shared_ptr<T>(this) gây double free ──────────────
struct BadNode {
    std::string name;
    explicit BadNode(const std::string& n) : name(n) {
        std::cout << "[+] BadNode '" << name << "'\n";
    }
    ~BadNode() {
        std::cout << "[-] BadNode '" << name << "'\n";
    }

    // WRONG: tạo ra shared_ptr mới, control block riêng biệt!
    // → Khi cả hai shared_ptr bị destroy → double free!
    std::shared_ptr<BadNode> get_self_WRONG() {
        return std::shared_ptr<BadNode>(this);  // DANGER!
    }
};

void demo_bad_shared_from_this() {
    std::cout << "\n=== BAD: shared_ptr(this) → double free ===\n";
    std::cout << "(skipped — would crash; demonstrating concept only)\n";
    // auto node = std::make_shared<BadNode>("bad");
    // auto self = node->get_self_WRONG();  // DANGER: 2 control blocks!
    // → khi cả hai ra scope → delete ptr 2 lần → CRASH
}

// ─── Ví dụ 2: Fix — enable_shared_from_this ──────────────────────────────
class GoodNode : public std::enable_shared_from_this<GoodNode> {
public:
    std::string name;

    explicit GoodNode(const std::string& n) : name(n) {
        std::cout << "[+] GoodNode '" << name << "'\n";
    }
    ~GoodNode() {
        std::cout << "[-] GoodNode '" << name << "'\n";
    }

    // CORRECT: shared_from_this() chia sẻ control block gốc
    std::shared_ptr<GoodNode> get_self() {
        return shared_from_this();  // OK!
    }

    void register_to(std::vector<std::shared_ptr<GoodNode>>& registry) {
        // Tự đăng ký vào registry — cần shared_ptr của chính mình
        registry.push_back(shared_from_this());
        std::cout << "[" << name << "] registered (use_count="
                  << shared_from_this().use_count() << ")\n";
    }
};

void demo_enable_shared_from_this() {
    std::cout << "\n=== GOOD: enable_shared_from_this ===\n";

    // PHẢI tạo qua make_shared, không phải stack allocation!
    auto node = std::make_shared<GoodNode>("node_A");
    std::cout << "use_count after creation: " << node.use_count() << "\n";  // 1

    auto self1 = node->get_self();
    auto self2 = node->get_self();
    std::cout << "use_count after 2 get_self(): " << node.use_count() << "\n";  // 3

    // Tất cả cùng trỏ đến một đối tượng, một control block
    std::cout << "Same object: " << (node.get() == self1.get()) << "\n";  // 1
}

// ─── Ví dụ 3: Pattern ROS2 Node ──────────────────────────────────────────
// rclcpp::Node kế thừa enable_shared_from_this → init() có thể dùng
// shared_from_this() để truyền cho các internal components

class Subscription;
class Timer;

class ROSNode : public std::enable_shared_from_this<ROSNode> {
public:
    std::string                           node_name;
    std::vector<std::shared_ptr<Subscription>> subscriptions;
    std::vector<std::shared_ptr<Timer>>        timers;

    explicit ROSNode(const std::string& name) : node_name(name) {
        std::cout << "[+] ROSNode '" << node_name << "' created\n";
    }
    ~ROSNode() {
        std::cout << "[-] ROSNode '" << node_name << "' destroyed\n";
    }

    // PHẢI gọi sau khi đối tượng đã có shared_ptr (post-construction init)
    void init();

    std::shared_ptr<Subscription> create_subscription(const std::string& topic,
                                                       std::function<void(int)> cb);
    std::shared_ptr<Timer>        create_timer(int interval_ms,
                                               std::function<void()> cb);
};

class Subscription {
public:
    std::string topic;
    std::function<void(int)> callback;
    std::weak_ptr<ROSNode> node;  // back-ref (weak để tránh circular)

    Subscription(const std::string& t, std::function<void(int)> cb,
                 std::shared_ptr<ROSNode> n)
        : topic(t), callback(std::move(cb)), node(n)
    {
        std::cout << "[+] Subscription '" << topic << "'\n";
    }
    ~Subscription() { std::cout << "[-] Subscription '" << topic << "'\n"; }

    void receive(int msg) {
        if (callback) callback(msg);
    }
};

class Timer {
public:
    int interval_ms;
    std::function<void()> callback;

    Timer(int ms, std::function<void()> cb)
        : interval_ms(ms), callback(std::move(cb))
    {
        std::cout << "[+] Timer interval=" << interval_ms << "ms\n";
    }
    ~Timer() { std::cout << "[-] Timer interval=" << interval_ms << "ms\n"; }

    void tick() { if (callback) callback(); }
};

std::shared_ptr<Subscription> ROSNode::create_subscription(
    const std::string& topic, std::function<void(int)> cb)
{
    // shared_from_this() — truyền shared_ptr của node vào Subscription
    auto sub = std::make_shared<Subscription>(topic, std::move(cb), shared_from_this());
    subscriptions.push_back(sub);
    return sub;
}

std::shared_ptr<Timer> ROSNode::create_timer(int ms, std::function<void()> cb) {
    auto timer = std::make_shared<Timer>(ms, std::move(cb));
    timers.push_back(timer);
    return timer;
}

void ROSNode::init() {
    // Dùng shared_from_this() an toàn vì *this đã được quản lý bởi shared_ptr
    create_subscription("/scan", [this](int msg) {
        std::cout << "[" << node_name << "] Received scan: " << msg << "\n";
    });
    create_timer(100, [this]() {
        std::cout << "[" << node_name << "] Timer tick\n";
    });
}

void demo_ros2_node_pattern() {
    std::cout << "\n=== ROS2 Node Pattern with enable_shared_from_this ===\n";
    {
        auto node = std::make_shared<ROSNode>("lidar_node");
        node->init();

        std::cout << "use_count=" << node.use_count() << "\n";

        // Simulate receiving messages
        for (auto& sub : node->subscriptions) sub->receive(42);
        for (auto& t   : node->timers)       t->tick();
    }
    // node, subscriptions, timers tất cả được destroy đúng thứ tự
}

// ─── Lỗi phổ biến: gọi shared_from_this() từ constructor ────────────────
class EarlyCall : public std::enable_shared_from_this<EarlyCall> {
public:
    EarlyCall() {
        // WRONG: constructor chạy TRƯỚC khi shared_ptr được tạo
        // → shared_from_this() ở đây sẽ throw std::bad_weak_ptr
        // auto self = shared_from_this();  // CRASH!
        std::cout << "(Cannot call shared_from_this() in constructor)\n";
    }
    // Solution: dùng post-construction init() như ở trên
};

int main() {
    demo_bad_shared_from_this();
    demo_enable_shared_from_this();
    demo_ros2_node_pattern();

    std::cout << "\n=== Early call warning ===\n";
    auto obj = std::make_shared<EarlyCall>();
    return 0;
}
