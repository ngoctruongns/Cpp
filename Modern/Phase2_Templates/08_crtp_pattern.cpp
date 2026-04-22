/**
 * PHASE 2 - Bài 08: CRTP (Curiously Recurring Template Pattern)
 *
 * Mục tiêu:
 *  - Hiểu CRTP: Base<Derived> cho static polymorphism
 *  - So sánh CRTP vs virtual function (hiệu năng, tradeoffs)
 *  - Ứng dụng: mixin, interface injection, static dispatch
 *  - Pattern dùng trong rclcpp (Lifecycle Node, component)
 *
 * Compile: g++ -std=c++17 08_crtp_pattern.cpp -o out
 */

#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <memory>

// ─── Ví dụ 1: Cơ bản CRTP vs virtual ─────────────────────────────────────

// Virtual version — dynamic dispatch (vtable lookup)
class VirtualAnimal {
public:
    virtual void speak() const = 0;
    void speak_twice() {
        speak();  // virtual call × 2
        speak();
    }
    virtual ~VirtualAnimal() = default;
};

class VirtualDog : public VirtualAnimal {
public:
    void speak() const override { std::cout << "  Woof!\n"; }
};

// CRTP version — static dispatch (no vtable)
template<typename Derived>
class CRTPAnimal {
public:
    void speak() const {
        // Downcasting to Derived — resolved at compile time
        static_cast<const Derived*>(this)->speak_impl();
    }
    void speak_twice() const {
        static_cast<const Derived*>(this)->speak_impl();
        static_cast<const Derived*>(this)->speak_impl();
    }
};

class CRTPDog : public CRTPAnimal<CRTPDog> {
public:
    void speak_impl() const { std::cout << "  Woof! (CRTP)\n"; }
};

class CRTPCat : public CRTPAnimal<CRTPCat> {
public:
    void speak_impl() const { std::cout << "  Meow! (CRTP)\n"; }
};

void demo_crtp_basics() {
    std::cout << "\n=== Virtual vs CRTP ===\n";
    std::cout << "Virtual:\n";
    VirtualDog vd;
    vd.speak_twice();

    std::cout << "CRTP:\n";
    CRTPDog cd;
    cd.speak_twice();

    CRTPCat cc;
    cc.speak_twice();

    // Key difference: CRTP types are NOT related via pointer
    // Cannot: CRTPAnimal<?>* p = &cd;  (no common base)
    // For polymorphic containers → still need virtual
}

// ─── Ví dụ 2: CRTP Mixin — thêm chức năng không cần virtual ──────────────

// Mixin: thêm print capability vào bất kỳ class nào
template<typename Derived>
class Printable {
public:
    void print() const {
        std::cout << static_cast<const Derived*>(this)->to_string() << "\n";
    }
    void print_n(int n) const {
        for (int i = 0; i < n; ++i) print();
    }
};

// Mixin: thêm comparison operators
template<typename Derived>
class Comparable {
public:
    bool operator!=(const Derived& other) const {
        return !(*static_cast<const Derived*>(this) == other);
    }
    bool operator<=(const Derived& other) const {
        auto self = static_cast<const Derived*>(this);
        return (*self == other) || (*self < other);
    }
    bool operator>(const Derived& other) const {
        auto self = static_cast<const Derived*>(this);
        return !(*self <= other);
    }
    bool operator>=(const Derived& other) const {
        auto self = static_cast<const Derived*>(this);
        return !(*self < other);
    }
    // Derived must implement: operator== and operator<
};

// Sensor class using multiple CRTP mixins
class Sensor : public Printable<Sensor>, public Comparable<Sensor> {
public:
    std::string name;
    double      value;

    Sensor(const std::string& n, double v) : name(n), value(v) {}

    // Required by Printable
    std::string to_string() const {
        return "Sensor{" + name + "=" + std::to_string(value) + "}";
    }

    // Required by Comparable
    bool operator==(const Sensor& o) const { return value == o.value; }
    bool operator<(const Sensor& o)  const { return value <  o.value; }
};

void demo_crtp_mixin() {
    std::cout << "\n=== CRTP Mixin ===\n";
    Sensor s1{"lidar", 3.14};
    Sensor s2{"imu",   9.81};

    s1.print();
    s2.print_n(2);

    std::cout << "s1 < s2:  " << (s1 <  s2) << "\n";
    std::cout << "s1 <= s2: " << (s1 <= s2) << "\n";
    std::cout << "s1 > s2:  " << (s1 >  s2) << "\n";
    std::cout << "s1 != s2: " << (s1 != s2) << "\n";
}

// ─── Ví dụ 3: CRTP cho ROS2-style Node interface (static dispatch) ────────

template<typename Derived>
class NodeInterface {
public:
    // Template method pattern — calls derived's implementations
    void spin_once() {
        auto* d = static_cast<Derived*>(this);
        d->on_timer();
        d->on_message();
    }

    const std::string& get_name() const {
        return static_cast<const Derived*>(this)->name_;
    }

    // Default no-op implementations (Derived can override without virtual)
    void on_timer()   { /* default: nothing */ }
    void on_message() { /* default: nothing */ }
};

class LidarNode : public NodeInterface<LidarNode> {
public:
    std::string name_ = "lidar_node";
    int tick_ = 0;

    void on_timer() {
        std::cout << "[" << name_ << "] timer tick #" << ++tick_ << "\n";
    }
    // on_message not overridden → uses default no-op
};

class FusionNode : public NodeInterface<FusionNode> {
public:
    std::string name_ = "fusion_node";

    void on_timer() {
        std::cout << "[" << name_ << "] fusion timer\n";
    }
    void on_message() {
        std::cout << "[" << name_ << "] processing message\n";
    }
};

void demo_ros2_crtp() {
    std::cout << "\n=== ROS2-style CRTP Node ===\n";
    LidarNode lidar;
    FusionNode fusion;

    for (int i = 0; i < 2; ++i) {
        lidar.spin_once();
        fusion.spin_once();
    }
}

// ─── Ví dụ 4: CRTP Clone pattern ─────────────────────────────────────────
// Problem: unique_ptr<Base> needs to clone → virtual clone()
// CRTP: automatically generate clone() for each derived type

template<typename Base, typename Derived>
class Cloneable {
public:
    std::unique_ptr<Base> clone() const {
        return std::make_unique<Derived>(*static_cast<const Derived*>(this));
    }
};

class PluginBase {
public:
    virtual std::unique_ptr<PluginBase> clone() const = 0;
    virtual void execute(const std::string& input) const = 0;
    virtual ~PluginBase() = default;
};

class FilterPlugin : public PluginBase, public Cloneable<PluginBase, FilterPlugin> {
public:
    double threshold;
    explicit FilterPlugin(double t) : threshold(t) {}

    std::unique_ptr<PluginBase> clone() const override {
        return Cloneable<PluginBase, FilterPlugin>::clone();
    }
    void execute(const std::string& input) const override {
        std::cout << "[FilterPlugin t=" << threshold << "] on: " << input << "\n";
    }
};

class TransformPlugin : public PluginBase, public Cloneable<PluginBase, TransformPlugin> {
public:
    std::string frame;
    explicit TransformPlugin(const std::string& f) : frame(f) {}

    std::unique_ptr<PluginBase> clone() const override {
        return Cloneable<PluginBase, TransformPlugin>::clone();
    }
    void execute(const std::string& input) const override {
        std::cout << "[TransformPlugin frame=" << frame << "] on: " << input << "\n";
    }
};

void demo_crtp_clone() {
    std::cout << "\n=== CRTP Clone Pattern ===\n";
    std::vector<std::unique_ptr<PluginBase>> plugins;
    plugins.emplace_back(std::make_unique<FilterPlugin>(0.5));
    plugins.emplace_back(std::make_unique<TransformPlugin>("map"));

    // Clone the pipeline
    std::vector<std::unique_ptr<PluginBase>> cloned;
    for (const auto& p : plugins) cloned.push_back(p->clone());

    std::cout << "Original:\n";
    for (const auto& p : plugins)  p->execute("data");

    std::cout << "Cloned:\n";
    for (const auto& p : cloned)   p->execute("data");
}

int main() {
    demo_crtp_basics();
    demo_crtp_mixin();
    demo_ros2_crtp();
    demo_crtp_clone();
    return 0;
}
