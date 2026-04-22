/**
 * PHASE 4 - Bài 03: Robot State Machine
 *
 * Mục tiêu:
 *  - State pattern: encapsulate behavior in state objects
 *  - Hierarchical state transitions with guards
 *  - Giống ROS2 Lifecycle Node states
 *
 * Compile: g++ -std=c++17 03_state_machine.cpp -o out
 */

#include <iostream>
#include <memory>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <stdexcept>

// ──────────────────────────────────────────────────────────────────────────
// Events
// ──────────────────────────────────────────────────────────────────────────
enum class Event {
    CONFIGURE,
    ACTIVATE,
    DEACTIVATE,
    CLEANUP,
    ERROR_DETECTED,
    RECOVER,
    SHUTDOWN
};

std::string event_name(Event e) {
    switch (e) {
        case Event::CONFIGURE:      return "CONFIGURE";
        case Event::ACTIVATE:       return "ACTIVATE";
        case Event::DEACTIVATE:     return "DEACTIVATE";
        case Event::CLEANUP:        return "CLEANUP";
        case Event::ERROR_DETECTED: return "ERROR_DETECTED";
        case Event::RECOVER:        return "RECOVER";
        case Event::SHUTDOWN:       return "SHUTDOWN";
    }
    return "UNKNOWN";
}

// ──────────────────────────────────────────────────────────────────────────
// State base class
// ──────────────────────────────────────────────────────────────────────────
class RobotContext;  // forward

class State {
public:
    virtual ~State() = default;
    virtual std::string name() const = 0;
    virtual void on_enter(RobotContext& ctx) {}
    virtual void on_exit(RobotContext& ctx)  {}
    virtual void handle(RobotContext& ctx, Event e) {
        std::cout << "  [" << name() << "] Cannot handle event: "
                  << event_name(e) << "\n";
    }
};

// ──────────────────────────────────────────────────────────────────────────
// Robot context (state machine owner)
// ──────────────────────────────────────────────────────────────────────────
class RobotContext {
    std::shared_ptr<State> current_state_;
    std::string            robot_name_;
    double                 battery_ = 100.0;
    bool                   sensor_ok_ = true;

public:
    explicit RobotContext(const std::string& name, std::shared_ptr<State> initial)
        : robot_name_(name)
    {
        transition_to(std::move(initial));
    }

    void process(Event e) {
        std::cout << "\n[" << robot_name_ << "] Event: " << event_name(e) << "\n";
        current_state_->handle(*this, e);
    }

    void transition_to(std::shared_ptr<State> next_state) {
        if (current_state_) {
            std::cout << "  [SM] " << current_state_->name()
                      << " → " << next_state->name() << "\n";
            current_state_->on_exit(*this);
        } else {
            std::cout << "  [SM] → " << next_state->name() << " (initial)\n";
        }
        current_state_ = std::move(next_state);
        current_state_->on_enter(*this);
    }

    std::string state_name() const { return current_state_->name(); }
    double battery() const { return battery_; }
    bool sensor_ok() const { return sensor_ok_; }

    void consume_battery(double amount) { battery_ -= amount; }
    void set_sensor_ok(bool ok) { sensor_ok_ = ok; }
};

// ──────────────────────────────────────────────────────────────────────────
// Concrete States (ROS2 Lifecycle-inspired)
// ──────────────────────────────────────────────────────────────────────────

// Forward declarations
class UnconfiguredState;
class InactiveState;
class ActiveState;
class ErrorState;
class FinalizedState;

class UnconfiguredState : public State {
public:
    std::string name() const override { return "UNCONFIGURED"; }

    void on_enter(RobotContext&) override {
        std::cout << "  [UNCONFIGURED] Waiting for configuration\n";
    }

    void handle(RobotContext& ctx, Event e) override;
};

class InactiveState : public State {
public:
    std::string name() const override { return "INACTIVE"; }

    void on_enter(RobotContext&) override {
        std::cout << "  [INACTIVE] Node configured, waiting to activate\n";
    }

    void handle(RobotContext& ctx, Event e) override;
};

class ActiveState : public State {
    int tick_ = 0;
public:
    std::string name() const override { return "ACTIVE"; }

    void on_enter(RobotContext& ctx) override {
        std::cout << "  [ACTIVE] Node active — starting sensors\n";
        tick_ = 0;
    }

    void on_exit(RobotContext& ctx) override {
        std::cout << "  [ACTIVE] Stopping sensors (ticks=" << tick_ << ")\n";
    }

    void handle(RobotContext& ctx, Event e) override;

    void tick(RobotContext& ctx) {
        ++tick_;
        ctx.consume_battery(0.5);
        std::cout << "  [ACTIVE] tick #" << tick_
                  << " battery=" << ctx.battery() << "\n";
        if (ctx.battery() < 20.0) {
            std::cout << "  [ACTIVE] Low battery!\n";
        }
    }
};

class ErrorState : public State {
    std::string error_msg_;
public:
    explicit ErrorState(const std::string& msg) : error_msg_(msg) {}
    std::string name() const override { return "ERROR"; }

    void on_enter(RobotContext&) override {
        std::cout << "  [ERROR] " << error_msg_ << "\n";
    }

    void handle(RobotContext& ctx, Event e) override;
};

class FinalizedState : public State {
public:
    std::string name() const override { return "FINALIZED"; }

    void on_enter(RobotContext&) override {
        std::cout << "  [FINALIZED] Node shutdown complete\n";
    }

    void handle(RobotContext&, Event e) override {
        std::cout << "  [FINALIZED] Ignoring event (node is shutdown): "
                  << event_name(e) << "\n";
    }
};

// ─── State transitions ───────────────────────────────────────────────────
void UnconfiguredState::handle(RobotContext& ctx, Event e) {
    switch (e) {
        case Event::CONFIGURE:
            ctx.transition_to(std::make_shared<InactiveState>());
            break;
        case Event::SHUTDOWN:
            ctx.transition_to(std::make_shared<FinalizedState>());
            break;
        default:
            State::handle(ctx, e);
    }
}

void InactiveState::handle(RobotContext& ctx, Event e) {
    switch (e) {
        case Event::ACTIVATE:
            ctx.transition_to(std::make_shared<ActiveState>());
            break;
        case Event::CLEANUP:
            ctx.transition_to(std::make_shared<UnconfiguredState>());
            break;
        case Event::SHUTDOWN:
            ctx.transition_to(std::make_shared<FinalizedState>());
            break;
        default:
            State::handle(ctx, e);
    }
}

void ActiveState::handle(RobotContext& ctx, Event e) {
    switch (e) {
        case Event::DEACTIVATE:
            ctx.transition_to(std::make_shared<InactiveState>());
            break;
        case Event::ERROR_DETECTED:
            ctx.transition_to(std::make_shared<ErrorState>("Sensor failure detected"));
            break;
        case Event::SHUTDOWN:
            ctx.transition_to(std::make_shared<InactiveState>());
            ctx.transition_to(std::make_shared<FinalizedState>());
            break;
        default:
            State::handle(ctx, e);
    }
}

void ErrorState::handle(RobotContext& ctx, Event e) {
    switch (e) {
        case Event::RECOVER:
            ctx.transition_to(std::make_shared<InactiveState>());
            break;
        case Event::SHUTDOWN:
            ctx.transition_to(std::make_shared<FinalizedState>());
            break;
        default:
            State::handle(ctx, e);
    }
}

// ──────────────────────────────────────────────────────────────────────────
// Demo
// ──────────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "===== Robot State Machine (ROS2 Lifecycle) =====\n";

    RobotContext robot("my_robot", std::make_shared<UnconfiguredState>());

    // Normal lifecycle
    robot.process(Event::CONFIGURE);
    robot.process(Event::ACTIVATE);

    // Simulate 3 ticks while active (need to call tick directly)
    // (normally this would be timer-driven)
    // active state managed via process() transitions

    robot.process(Event::DEACTIVATE);
    robot.process(Event::ACTIVATE);  // reactivate
    robot.process(Event::ERROR_DETECTED);  // simulate error
    robot.process(Event::RECOVER);   // recover to INACTIVE
    robot.process(Event::SHUTDOWN);  // graceful shutdown

    std::cout << "\nFinal state: " << robot.state_name() << "\n";

    // Invalid transitions
    std::cout << "\n--- Invalid transitions ---\n";
    robot.process(Event::CONFIGURE);
    robot.process(Event::ACTIVATE);

    return 0;
}
