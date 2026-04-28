# RAII & Design Patterns: ScopeGuard, Observer, State Machine, Command

> **Bài tập liên quan:** Phase4 / 01, 02, 03, 04

---

## 1. RAII — Resource Acquisition Is Initialization

**Nguyên tắc:** Gắn lifetime của resource với lifetime của object. Constructor acquire, destructor release — kể cả khi có exception.

```cpp
// Ví dụ tổng quát
class ResourceHolder {
    Resource* res_;
public:
    explicit ResourceHolder(/* args */) : res_(acquire_resource()) {}
    ~ResourceHolder() { release_resource(res_); }

    ResourceHolder(const ResourceHolder&) = delete;   // không copy resource
    ResourceHolder& operator=(const ResourceHolder&) = delete;
};
```

---

## 2. ScopeGuard — Chạy Cleanup Khi Ra Scope

Hữu dụng khi không muốn tạo class riêng cho mỗi resource.

```cpp
class ScopeGuard {
    std::function<void()> fn_;
    bool dismissed_ = false;
public:
    explicit ScopeGuard(std::function<void()> fn) : fn_(std::move(fn)) {}
    ~ScopeGuard() { if (!dismissed_ && fn_) fn_(); }

    void dismiss() { dismissed_ = true; }  // hủy cleanup (success path)

    ScopeGuard(ScopeGuard&&) = default;
    ScopeGuard(const ScopeGuard&) = delete;
};

// Helper
ScopeGuard on_exit(std::function<void()> fn) { return ScopeGuard(std::move(fn)); }

// Dùng
void risky_init() {
    hardware_init();
    auto guard = on_exit([]{ hardware_deinit(); });  // deinit nếu có exception

    do_risky_stuff();   // nếu throw → guard deinit tự động

    guard.dismiss();    // thành công → không cần deinit
}
```

---

## 3. Observer Pattern

Loose coupling: Subject không biết gì về Observers — chỉ gọi callback khi có event.

```cpp
// Callback-based (modern C++)
template<typename... EventArgs>
class EventEmitter {
    using Handler = std::function<void(EventArgs...)>;
    std::vector<Handler> handlers_;

public:
    // Subscribe: trả về ID để unsubscribe sau này
    size_t on(Handler h) {
        handlers_.push_back(std::move(h));
        return handlers_.size() - 1;
    }

    void emit(EventArgs... args) {
        for (auto& h : handlers_) h(args...);
    }
};

// Dùng
EventEmitter<double> on_sensor_data;

auto id = on_sensor_data.on([](double v) {
    std::cout << "Sensor: " << v << "\n";
});

on_sensor_data.emit(3.14);  // gọi tất cả handlers
```

### Weak Observer (tránh dangling pointer)

```cpp
class Subject {
    std::vector<std::weak_ptr<Observer>> observers_;  // weak_ptr!

public:
    void notify(const Event& e) {
        observers_.erase(
            std::remove_if(observers_.begin(), observers_.end(),
                [&](auto& wp) {
                    if (auto sp = wp.lock()) { sp->update(e); return false; }
                    return true;  // observer đã bị destroy → xóa khỏi list
                }),
            observers_.end()
        );
    }
};
```

---

## 4. State Machine

Quản lý trạng thái phức tạp (robot modes, lifecycle) theo cách rõ ràng.

```cpp
// Enum cho states và events
enum class State  { IDLE, RUNNING, PAUSED, ERROR };
enum class Event  { START, PAUSE, RESUME, STOP, FAULT };

class StateMachine {
    State current_ = State::IDLE;

    // Transition table
    struct Transition {
        State from; Event event; State to;
        std::function<void()> action;  // optional action
    };

    std::vector<Transition> transitions_;

public:
    void add_transition(State from, Event ev, State to,
                        std::function<void()> action = {}) {
        transitions_.push_back({from, ev, to, std::move(action)});
    }

    bool process(Event ev) {
        for (auto& t : transitions_) {
            if (t.from == current_ && t.event == ev) {
                if (t.action) t.action();
                current_ = t.to;
                return true;
            }
        }
        return false;  // no valid transition
    }

    State state() const { return current_; }
};

// Dùng cho robot
StateMachine robot_sm;
robot_sm.add_transition(State::IDLE,    Event::START, State::RUNNING,
    []{ std::cout << "Robot started\n"; });
robot_sm.add_transition(State::RUNNING, Event::PAUSE, State::PAUSED);
robot_sm.add_transition(State::RUNNING, Event::FAULT, State::ERROR,
    []{ emergency_stop(); });

robot_sm.process(Event::START);
robot_sm.process(Event::FAULT);
```

---

## 5. Command Pattern

Đóng gói request thành object — hỗ trợ undo, queue, log.

```cpp
// Interface
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
};

// Concrete commands
class MoveRobotCommand : public Command {
    Robot& robot_; double dx_, dy_;
public:
    MoveRobotCommand(Robot& r, double dx, double dy)
        : robot_(r), dx_(dx), dy_(dy) {}

    void execute() override { robot_.move(dx_, dy_); }
    void undo()    override { robot_.move(-dx_, -dy_); }
};

// Command queue với undo stack
class CommandInvoker {
    std::queue<std::unique_ptr<Command>> queue_;
    std::stack<std::unique_ptr<Command>> history_;

public:
    void enqueue(std::unique_ptr<Command> cmd) {
        queue_.push(std::move(cmd));
    }

    void execute_next() {
        if (queue_.empty()) return;
        auto cmd = std::move(queue_.front());
        queue_.pop();
        cmd->execute();
        history_.push(std::move(cmd));
    }

    void undo_last() {
        if (history_.empty()) return;
        history_.top()->undo();
        history_.pop();
    }
};
```

---

## 6. So sánh các Pattern

| Pattern | Vấn đề giải quyết | Key C++ technique |
|---------|------------------|-------------------|
| RAII | Resource leak, exception safety | Constructor/Destructor |
| ScopeGuard | Cleanup code ad-hoc | `std::function`, move |
| Observer | Loose coupling, event notification | `std::function`, `weak_ptr` |
| State Machine | Complex state logic | Enum, transition table |
| Command | Request queuing, undo/redo | Polymorphism, `unique_ptr` |
