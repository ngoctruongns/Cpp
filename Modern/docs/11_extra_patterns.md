# Extra Design Patterns for Robotics

> Bổ sung cho Phase4 — Các pattern quan trọng còn thiếu trong bài tập

---

## 1. Active Object Pattern

**Vấn đề:** Mỗi subsystem (sensor, controller, communicator) cần chạy độc lập trong thread riêng, nhận lệnh bất đồng bộ mà không bị block caller.

**Đây là nền tảng của cách ROS2 Node hoạt động nội bộ.**

```cpp
class ActiveObject {
    ThreadSafeQueue<std::function<void()>> mailbox_;
    std::jthread                           worker_;

public:
    ActiveObject() : worker_([this](std::stop_token st) {
        while (!st.stop_requested()) {
            if (auto task = mailbox_.pop_timeout(10ms)) {
                (*task)();
            }
        }
    }) {}

    // Gửi lệnh từ bất kỳ thread nào — không block
    template<typename F>
    void send(F&& task) {
        mailbox_.push(std::forward<F>(task));
    }

    // Fire-and-forget với future để lấy kết quả
    template<typename F>
    auto send_future(F&& task) -> std::future<std::invoke_result_t<F>> {
        using R = std::invoke_result_t<F>;
        auto promise = std::make_shared<std::promise<R>>();
        auto future  = promise->get_future();
        mailbox_.push([p = promise, t = std::forward<F>(task)]() mutable {
            try { p->set_value(t()); }
            catch (...) { p->set_exception(std::current_exception()); }
        });
        return future;
    }
};

// Robot motor controller — có thread riêng
class MotorController : public ActiveObject {
public:
    void set_velocity(double v) {
        send([this, v]{ do_set_velocity(v); });  // async, không block
    }

    std::future<double> get_current() {
        return send_future([this]{ return read_current_sensor(); });
    }
private:
    void do_set_velocity(double v) { /* chỉ gọi từ thread nội bộ */ }
    double read_current_sensor()   { return 0.5; }
};

// Dùng
MotorController motor;
motor.set_velocity(1.5);           // non-blocking call
auto f = motor.get_current();
std::cout << f.get() << "\n";      // blocking wait for result
```

---

## 2. Pipeline Pattern

**Vấn đề:** Xử lý dữ liệu qua nhiều bước tuần tự: raw → filter → transform → output. Phổ biến trong sensor processing, control loop.

```cpp
template<typename T>
class Pipeline {
    using Stage = std::function<T(T)>;
    std::vector<Stage> stages_;

public:
    Pipeline& add(Stage stage) {
        stages_.push_back(std::move(stage));
        return *this;   // fluent interface
    }

    T process(T input) const {
        T result = std::move(input);
        for (const auto& stage : stages_) result = stage(std::move(result));
        return result;
    }
};

// Sensor data pipeline
struct SensorData { double raw, filtered, calibrated; };

Pipeline<SensorData> imu_pipeline;
imu_pipeline
    .add([](SensorData d) { d.filtered   = lowpass(d.raw, 0.1); return d; })
    .add([](SensorData d) { d.calibrated = d.filtered * 9.81;   return d; })
    .add([](SensorData d) { if (std::abs(d.calibrated) > 100) throw std::range_error{"overflow"}; return d; });

auto result = imu_pipeline.process({.raw = 1.02});
```

### Concurrent Pipeline (stages chạy song song)

```cpp
// Stage N+1 xử lý ngay khi stage N xong — pipeline parallelism
template<typename T>
class ConcurrentPipeline {
    std::vector<ThreadSafeQueue<T>> queues_;
    std::vector<std::jthread>       workers_;

public:
    // Mỗi stage chạy trên thread riêng
    // Throughput = throughput của stage chậm nhất
    // Latency    = sum của tất cả stages
};
```

---

## 3. Strategy Pattern

**Vấn đề:** Cho phép hoán đổi thuật toán tại runtime mà không thay đổi code dùng nó. Dùng cho: path planner, controller type, serialization format.

```cpp
// Strategy interface
class ControllerStrategy {
public:
    virtual ~ControllerStrategy() = default;
    virtual double compute(double error, double dt) = 0;
    virtual void reset() = 0;
};

// Concrete strategies
class PIDController : public ControllerStrategy {
    double kp_, ki_, kd_, integral_{0}, prev_error_{0};
public:
    PIDController(double kp, double ki, double kd)
        : kp_(kp), ki_(ki), kd_(kd) {}

    double compute(double error, double dt) override {
        integral_    += error * dt;
        double deriv  = (error - prev_error_) / dt;
        prev_error_   = error;
        return kp_ * error + ki_ * integral_ + kd_ * deriv;
    }
    void reset() override { integral_ = prev_error_ = 0; }
};

class BangBangController : public ControllerStrategy {
    double threshold_, output_;
public:
    BangBangController(double threshold, double output)
        : threshold_(threshold), output_(output) {}
    double compute(double error, double /*dt*/) override {
        return std::abs(error) > threshold_ ? (error > 0 ? output_ : -output_) : 0.0;
    }
    void reset() override {}
};

// Context — không biết strategy cụ thể là gì
class VelocityLoop {
    std::unique_ptr<ControllerStrategy> ctrl_;
public:
    void set_controller(std::unique_ptr<ControllerStrategy> c) {
        ctrl_ = std::move(c);
    }

    double update(double setpoint, double actual, double dt) {
        return ctrl_->compute(setpoint - actual, dt);
    }
};

// Runtime switching
VelocityLoop loop;
loop.set_controller(std::make_unique<PIDController>(1.0, 0.1, 0.05));
// ... sau khi tune xong hoặc mode thay đổi:
loop.set_controller(std::make_unique<BangBangController>(0.1, 1.0));
```

---

## 4. Object Pool Pattern

**Vấn đề:** Realtime loop không được phép cấp phát heap (`new`/`delete`) vì `malloc` không deterministic. Object Pool pre-allocate trước.

```cpp
template<typename T, size_t Capacity>
class ObjectPool {
    struct Slot {
        alignas(T) std::byte storage[sizeof(T)];
        bool in_use{false};
    };

    std::array<Slot, Capacity> slots_;
    std::mutex                 mtx_;

public:
    // Trả về unique_ptr với custom deleter (tự động trả về pool)
    std::unique_ptr<T, std::function<void(T*)>> acquire(auto&&... args) {
        std::lock_guard lock(mtx_);
        for (auto& slot : slots_) {
            if (!slot.in_use) {
                slot.in_use = true;
                T* obj = new (&slot.storage) T(std::forward<decltype(args)>(args)...);
                return {obj, [this, &slot](T* p) {
                    p->~T();
                    slot.in_use = false;
                }};
            }
        }
        return {nullptr, [](T*){}};  // pool exhausted
    }

    size_t available() const {
        std::lock_guard lock(mtx_);
        return std::count_if(slots_.begin(), slots_.end(),
                             [](const Slot& s){ return !s.in_use; });
    }
};

// Dùng cho message allocation trong realtime loop
ObjectPool<SensorMessage, 32> msg_pool;

void realtime_callback() {
    auto msg = msg_pool.acquire();   // O(n) nhưng không gọi malloc
    if (!msg) return;                // pool exhausted
    msg->timestamp = now_ns();
    publish(std::move(msg));         // trả về pool khi hết scope
}
```

---

## 5. Mediator / Topic-based Event Bus

**Vấn đề:** Observer đơn giản là 1-1 hoặc 1-N với cùng type. Event Bus cho phép **nhiều topic khác nhau, nhiều type**, các subscriber không biết nhau — giống ROS2 topic.

```cpp
class EventBus {
    using AnyHandler = std::function<void(const std::any&)>;
    std::unordered_map<std::string, std::vector<AnyHandler>> handlers_;
    mutable std::shared_mutex mtx_;

public:
    template<typename T>
    void subscribe(const std::string& topic,
                   std::function<void(const T&)> handler) {
        std::unique_lock lock(mtx_);
        handlers_[topic].push_back([h = std::move(handler)](const std::any& msg) {
            h(std::any_cast<const T&>(msg));
        });
    }

    template<typename T>
    void publish(const std::string& topic, T msg) {
        std::shared_lock lock(mtx_);
        auto it = handlers_.find(topic);
        if (it == handlers_.end()) return;
        std::any payload = std::move(msg);
        for (auto& h : it->second) h(payload);
    }
};

// Dùng — giống ROS2 pub/sub
EventBus bus;

bus.subscribe<OdomMsg>("/odom", [](const OdomMsg& msg) {
    std::cout << "x=" << msg.x << "\n";
});
bus.subscribe<OdomMsg>("/odom", [](const OdomMsg& msg) {
    update_slam(msg);   // 2 subscribers cùng topic
});

bus.publish("/odom", OdomMsg{.x = 1.0, .y = 0.5, .theta = 0.1});
```

---

## 6. Template Method Pattern

**Vấn đề:** Định nghĩa skeleton của một thuật toán trong base class, các bước cụ thể được override trong subclass.

```cpp
// Base: algorithm skeleton
class RobotTask {
public:
    // Template method — không override
    void run() {
        if (!initialize()) { handle_error("init failed"); return; }
        while (!is_done()) {
            auto data = read_sensor();
            auto cmd  = compute(data);
            send_command(cmd);
            on_cycle_end();   // hook — optional override
        }
        cleanup();
    }

protected:
    // Primitive operations — subclass phải implement
    virtual bool initialize() = 0;
    virtual SensorData read_sensor() = 0;
    virtual Command compute(const SensorData&) = 0;
    virtual void cleanup() = 0;
    virtual bool is_done() = 0;

    // Hooks — subclass có thể override (có default impl)
    virtual void on_cycle_end() {}
    virtual void handle_error(const std::string& msg) {
        std::cerr << "[ERROR] " << msg << "\n";
    }
};

// Concrete task
class WallFollower : public RobotTask {
    LidarSensor lidar_;
    MotorDriver motors_;

    bool initialize() override { return lidar_.open() && motors_.init(); }
    SensorData read_sensor() override { return lidar_.scan(); }
    Command compute(const SensorData& d) override {
        return wall_follow_algorithm(d);
    }
    void cleanup() override { motors_.stop(); }
    bool is_done() override { return obstacle_detected_; }
    void on_cycle_end() override { log_telemetry(); }  // optional hook

    bool obstacle_detected_{false};
};
```

---

## 7. Tổng quan — Design Patterns Robotics

| Category | Pattern | Khi nào dùng |
|----------|---------|-------------|
| **Resource** | RAII, ScopeGuard | Mọi lúc — hardware init/deinit |
| **Concurrency** | Active Object | Mỗi subsystem cần thread riêng |
| **Concurrency** | Realtime Buffer | Trao đổi data giữa RT và non-RT thread |
| **Data flow** | Pipeline | Sensor processing chain |
| **Behavior** | Strategy | Interchangeable algorithm (controller, planner) |
| **Behavior** | State Machine | Robot mode management |
| **Behavior** | Template Method | Algorithm with fixed structure, variable steps |
| **Communication** | Observer | 1 source → N listeners |
| **Communication** | Event Bus/Mediator | Multi-topic pub/sub (như ROS2) |
| **Communication** | Command | Request queuing, undo, log |
| **Extensibility** | Plugin | Runtime-loadable drivers/algorithms |
| **Performance** | Object Pool | Pre-allocated objects trong realtime |
| **Configuration** | Param Server | Centralized runtime config |
| **Lifecycle** | Lifecycle Node | Managed startup/shutdown |
| **Architecture** | Builder | Complex robot configuration |
