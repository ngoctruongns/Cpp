# ROS2 Design Patterns: Plugin, Param Server, Lifecycle, Realtime Buffer

> **Bài tập liên quan:** Phase4 / 05, 06, 07, 08, 09, 10

---

## 1. Plugin System

Tách implementation ra khỏi interface — load/unload tại runtime (tương tự `pluginlib` trong ROS2).

```cpp
// Interface (shared header)
class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual std::string name() const = 0;
    virtual void execute() = 0;
};

// Concrete plugin
class LidarPlugin : public IPlugin {
public:
    std::string name() const override { return "lidar"; }
    void execute() override { std::cout << "Lidar scanning...\n"; }
};

// Plugin registry
class PluginManager {
    using Factory = std::function<std::unique_ptr<IPlugin>()>;
    std::unordered_map<std::string, Factory> registry_;

public:
    void register_plugin(const std::string& name, Factory factory) {
        registry_[name] = std::move(factory);
    }

    std::unique_ptr<IPlugin> create(const std::string& name) {
        auto it = registry_.find(name);
        if (it == registry_.end()) return nullptr;
        return it->second();
    }
};

// Đăng ký & dùng
PluginManager mgr;
mgr.register_plugin("lidar", []{ return std::make_unique<LidarPlugin>(); });
mgr.register_plugin("imu",   []{ return std::make_unique<ImuPlugin>(); });

auto plugin = mgr.create("lidar");
plugin->execute();
```

---

## 2. Parameter Server Simulation

Tập trung quản lý config parameters, hỗ trợ nhiều type (như `rclcpp::Node::declare_parameter`).

```cpp
class ParamServer {
    std::unordered_map<std::string, std::any>  params_;
    mutable std::shared_mutex                   mtx_;

public:
    template<typename T>
    void set(const std::string& name, T value) {
        std::unique_lock lock(mtx_);
        params_[name] = std::move(value);
    }

    template<typename T>
    std::optional<T> get(const std::string& name) const {
        std::shared_lock lock(mtx_);
        auto it = params_.find(name);
        if (it == params_.end()) return std::nullopt;
        try {
            return std::any_cast<T>(it->second);
        } catch (...) {
            return std::nullopt;
        }
    }

    template<typename T>
    T get_or(const std::string& name, T default_val) const {
        return get<T>(name).value_or(std::move(default_val));
    }
};

// Dùng
ParamServer params;
params.set("max_speed",  1.5);
params.set("robot_name", std::string("diff_bot"));

double speed = params.get_or<double>("max_speed", 0.5);
auto name    = params.get<std::string>("robot_name");
```

---

## 3. Lifecycle Node Simulation

Mô phỏng ROS2 Managed Node — node có vòng đời rõ ràng với states và transitions.

```cpp
// ROS2 Lifecycle states
enum class LifecycleState {
    UNCONFIGURED,
    INACTIVE,
    ACTIVE,
    FINALIZED,
    ERROR
};

class LifecycleNode {
    LifecycleState state_ = LifecycleState::UNCONFIGURED;
    std::string    name_;

public:
    explicit LifecycleNode(const std::string& name) : name_(name) {}

    // Transition callbacks — override trong subclass
    virtual bool on_configure()  { return true; }
    virtual bool on_activate()   { return true; }
    virtual bool on_deactivate() { return true; }
    virtual bool on_cleanup()    { return true; }
    virtual bool on_shutdown()   { return true; }

    // Trigger transitions
    bool configure() {
        if (state_ != LifecycleState::UNCONFIGURED) return false;
        if (on_configure()) { state_ = LifecycleState::INACTIVE; return true; }
        state_ = LifecycleState::ERROR; return false;
    }
    bool activate() {
        if (state_ != LifecycleState::INACTIVE) return false;
        if (on_activate()) { state_ = LifecycleState::ACTIVE; return true; }
        state_ = LifecycleState::ERROR; return false;
    }
    // ... deactivate, cleanup, shutdown tương tự

    LifecycleState get_state() const { return state_; }
};

// Custom node
class MotorController : public LifecycleNode {
public:
    explicit MotorController() : LifecycleNode("motor_ctrl") {}

    bool on_configure() override {
        std::cout << "Loading motor config...\n";
        // load params, init hardware
        return true;
    }

    bool on_activate() override {
        std::cout << "Starting motor control loop\n";
        // start timer, threads
        return true;
    }
};
```

---

## 4. Component Node (Intra-process)

Nodes có thể chạy trong cùng process, chia sẻ data mà không cần serialize.

```cpp
// Node component interface
class NodeComponent {
public:
    virtual ~NodeComponent() = default;
    virtual void spin_once() = 0;
    virtual std::string get_name() const = 0;
};

// Executor: chạy nhiều components
class Executor {
    std::vector<std::shared_ptr<NodeComponent>> nodes_;
    std::atomic<bool> running_{true};

public:
    void add_node(std::shared_ptr<NodeComponent> node) {
        nodes_.push_back(std::move(node));
    }

    void spin() {
        while (running_) {
            for (auto& n : nodes_) n->spin_once();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void shutdown() { running_ = false; }
};
```

---

## 5. Realtime Buffer (Thread-safe Data Exchange)

Pattern trao đổi data giữa realtime thread (fast loop) và non-realtime thread (slow logic) không dùng lock trong hot path.

```cpp
// Dùng double buffer hoặc atomic + copy
template<typename T>
class RealtimeBuffer {
    T   buffer_[2];
    std::atomic<int> write_idx_{0};
    std::atomic<int> read_idx_{0};
    std::mutex write_mtx_;

public:
    // Ghi từ non-realtime thread
    void write(const T& data) {
        std::lock_guard lock(write_mtx_);
        int next = 1 - read_idx_.load(std::memory_order_acquire);
        buffer_[next] = data;
        write_idx_.store(next, std::memory_order_release);
        read_idx_.store(next, std::memory_order_release);
    }

    // Đọc từ realtime thread — không block, không lock
    const T& read() const {
        return buffer_[read_idx_.load(std::memory_order_acquire)];
    }
};

// Dùng
RealtimeBuffer<ControlParams> params_buf;

// Slow thread (config updates)
std::thread slow([&]{
    params_buf.write({.max_speed = 1.5, .kp = 0.8});
});

// Fast realtime loop (10ms)
while (running) {
    auto& params = params_buf.read();  // no lock!
    motor.set_speed(params.max_speed);
}
```

---

## 6. Mini ROS Framework (Tổng hợp)

Kết hợp tất cả patterns:

```cpp
class MiniRos {
    PluginManager    plugins_;
    ParamServer      params_;
    Executor         executor_;

public:
    template<typename NodeT, typename... Args>
    std::shared_ptr<NodeT> create_node(Args&&... args) {
        auto node = std::make_shared<NodeT>(std::forward<Args>(args)...);
        executor_.add_node(node);
        return node;
    }

    void spin()     { executor_.spin(); }
    void shutdown() { executor_.shutdown(); }

    ParamServer& get_param_server() { return params_; }
};
```

---

## 7. Tóm tắt

| Pattern | Tương đương ROS2 | Key technique |
|---------|-----------------|---------------|
| Plugin System | `pluginlib` | `unordered_map` + `std::function` factory |
| Param Server | `rclcpp` parameters | `std::any`, `shared_mutex`, `optional` |
| Lifecycle Node | Managed Node | State machine, virtual callbacks |
| Component Node | Composable Node | `shared_ptr`, Executor |
| Realtime Buffer | `realtime_tools::RealtimeBuffer` | Double buffer + `atomic` |
