/**
 * PHASE 1 - Bài 05: Circular Reference & weak_ptr Fix
 *
 * Mục tiêu:
 *  - Nhận biết circular reference gây memory leak
 *  - Fix bằng weak_ptr
 *  - Hiểu lock() và expired()
 *
 * Compile: g++ -std=c++17 05_circular_ref_fix.cpp -o out
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ─── Ví dụ 1: Lỗi circular reference ────────────────────────────────────
// Tình huống: Node A trỏ đến Node B, Node B trỏ lại Node A
// → cả hai không bao giờ được giải phóng!
struct NodeBad;

struct NodeBad {
    std::string name;
    std::shared_ptr<NodeBad> next;  // strong reference → giữ nhau mãi

    explicit NodeBad(const std::string& n) : name(n) {
        std::cout << "[+] NodeBad '" << name << "' created\n";
    }
    ~NodeBad() {
        std::cout << "[-] NodeBad '" << name << "' destroyed\n";
    }
};

void demo_circular_ref_leak() {
    std::cout << "\n=== CIRCULAR REFERENCE (Memory Leak!) ===\n";
    {
        auto a = std::make_shared<NodeBad>("A");
        auto b = std::make_shared<NodeBad>("B");

        a->next = b;  // A holds B
        b->next = a;  // B holds A → circular!

        std::cout << "a.use_count=" << a.use_count() << "\n";  // 2
        std::cout << "b.use_count=" << b.use_count() << "\n";  // 2
    }
    // a, b ra scope → use_count giảm còn 1 (không phải 0!)
    // → KHÔNG có destructor nào được gọi → MEMORY LEAK
    std::cout << "--- Scope ended (notice: NO destructors called!) ---\n";
}

// ─── Ví dụ 2: Fix bằng weak_ptr ──────────────────────────────────────────
struct NodeGood {
    std::string name;
    std::weak_ptr<NodeGood> next;  // weak ref → không tăng use_count!

    explicit NodeGood(const std::string& n) : name(n) {
        std::cout << "[+] NodeGood '" << name << "' created\n";
    }
    ~NodeGood() {
        std::cout << "[-] NodeGood '" << name << "' destroyed\n";
    }

    // Truy cập next an toàn qua lock()
    void visit_next() {
        if (auto locked = next.lock()) {  // lock() trả về shared_ptr
            std::cout << "[" << name << "] next node = '" << locked->name << "'\n";
        } else {
            std::cout << "[" << name << "] next node expired or nullptr\n";
        }
    }
};

void demo_circular_ref_fixed() {
    std::cout << "\n=== FIXED WITH weak_ptr ===\n";
    {
        auto a = std::make_shared<NodeGood>("A");
        auto b = std::make_shared<NodeGood>("B");

        a->next = b;  // weak reference  → b.use_count vẫn = 1
        b->next = a;  // weak reference  → a.use_count vẫn = 1

        std::cout << "a.use_count=" << a.use_count() << "\n";  // 1
        std::cout << "b.use_count=" << b.use_count() << "\n";  // 1

        a->visit_next();  // "b" còn sống → lock() thành công
        b->visit_next();  // "a" còn sống → lock() thành công
    }
    // a, b ra scope → use_count = 0 → cả hai bị destroy
    std::cout << "--- Scope ended (both nodes properly destroyed) ---\n";
}

// ─── Ví dụ 3: weak_ptr expired() — kiểm tra đối tượng còn tồn tại ───────
void demo_weak_ptr_expired() {
    std::cout << "\n=== weak_ptr expired() ===\n";

    std::weak_ptr<int> wp;

    {
        auto sp = std::make_shared<int>(42);
        wp = sp;
        std::cout << "Inside scope: expired=" << wp.expired()
                  << " value=" << *wp.lock() << "\n";
    }
    // sp đã bị destroy
    std::cout << "After scope: expired=" << wp.expired() << "\n";

    // Truy cập an toàn:
    if (auto locked = wp.lock()) {
        std::cout << "Value: " << *locked << "\n";
    } else {
        std::cout << "Object no longer exists — safe, no crash!\n";
    }
}

// ─── Ví dụ 4: Tình huống ROS2 — Publisher trỏ đến Node, Node trỏ đến Publisher
struct ROS2Node;

struct Publisher {
    std::string topic;
    std::weak_ptr<ROS2Node> owner;  // non-owning back-reference

    explicit Publisher(const std::string& t) : topic(t) {
        std::cout << "[+] Publisher '" << topic << "' created\n";
    }
    ~Publisher() {
        std::cout << "[-] Publisher '" << topic << "' destroyed\n";
    }

    void publish(const std::string& msg) {
        if (auto node = owner.lock()) {
            std::cout << "[Publisher:" << topic << "] Msg='" << msg << "'\n";
        }
    }
};

struct ROS2Node {
    std::string name;
    std::vector<std::shared_ptr<Publisher>> publishers;  // owns publishers

    explicit ROS2Node(const std::string& n) : name(n) {
        std::cout << "[+] ROS2Node '" << name << "' created\n";
    }
    ~ROS2Node() {
        std::cout << "[-] ROS2Node '" << name << "' destroyed\n";
    }

    std::shared_ptr<Publisher> create_publisher(const std::string& topic) {
        auto pub = std::make_shared<Publisher>(topic);
        publishers.push_back(pub);
        return pub;
    }
};

void demo_ros2_pattern() {
    std::cout << "\n=== ROS2 Publisher Pattern (no circular ref) ===\n";
    {
        auto node = std::make_shared<ROS2Node>("my_node");
        auto pub = node->create_publisher("/scan");
        pub->owner = node;  // weak back-reference → node.use_count vẫn = 1

        std::cout << "node.use_count=" << node.use_count() << "\n";  // 1
        pub->publish("hello scan data");
    }
    std::cout << "--- Scope ended (all destroyed properly) ---\n";
}

int main() {
    demo_circular_ref_leak();
    demo_circular_ref_fixed();
    demo_weak_ptr_expired();
    demo_ros2_pattern();
    return 0;
}
