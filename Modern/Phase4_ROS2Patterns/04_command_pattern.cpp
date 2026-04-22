/**
 * PHASE 4 - Bài 04: Command Pattern (ROS2 Action-like)
 *
 * Mục tiêu:
 *  - Command pattern: encapsulate request as object
 *  - Undo/redo queue, macro command
 *  - Async command với future/callback (giống ROS2 Action)
 *
 * Compile: g++ -std=c++17 -pthread 04_command_pattern.cpp -o out
 */

#include <iostream>
#include <memory>
#include <functional>
#include <vector>
#include <queue>
#include <stack>
#include <string>
#include <thread>
#include <future>
#include <chrono>
#include <atomic>
#include <mutex>

using namespace std::chrono_literals;

// ──────────────────────────────────────────────────────────────────────────
// PART 1: Command interface
// ──────────────────────────────────────────────────────────────────────────
class Command {
public:
    virtual ~Command() = default;
    virtual void execute()   = 0;
    virtual void undo()      = 0;
    virtual std::string name() const = 0;
};

// ──────────────────────────────────────────────────────────────────────────
// PART 2: Robot actuator (receiver)
// ──────────────────────────────────────────────────────────────────────────
class RobotArm {
public:
    double joint_angle = 0.0;   // degrees
    double gripper     = 0.0;   // 0=open, 1=closed
    std::string status = "idle";
    std::mutex mtx;

    void move_joint(double delta) {
        std::lock_guard<std::mutex> lk(mtx);
        joint_angle += delta;
        std::cout << "  [Arm] joint_angle=" << joint_angle << "°\n";
    }

    void set_gripper(double pos) {
        std::lock_guard<std::mutex> lk(mtx);
        gripper = pos;
        std::cout << "  [Arm] gripper=" << gripper
                  << (pos > 0.5 ? " (closed)" : " (open)") << "\n";
    }

    void print_state() const {
        std::cout << "  [Arm] joint=" << joint_angle
                  << "° gripper=" << gripper << "\n";
    }
};

// ──────────────────────────────────────────────────────────────────────────
// PART 3: Concrete commands
// ──────────────────────────────────────────────────────────────────────────
class MoveJointCommand : public Command {
    RobotArm& arm_;
    double    delta_;
    double    prev_angle_ = 0.0;

public:
    MoveJointCommand(RobotArm& arm, double delta)
        : arm_(arm), delta_(delta) {}

    void execute() override {
        prev_angle_ = arm_.joint_angle;
        arm_.move_joint(delta_);
    }

    void undo() override {
        arm_.move_joint(-delta_);
        std::cout << "  [undo] joint back to " << arm_.joint_angle << "°\n";
    }

    std::string name() const override {
        return "MoveJoint(" + std::to_string(delta_) + ")";
    }
};

class SetGripperCommand : public Command {
    RobotArm& arm_;
    double    pos_;
    double    prev_pos_ = 0.0;

public:
    SetGripperCommand(RobotArm& arm, double pos)
        : arm_(arm), pos_(pos) {}

    void execute() override {
        prev_pos_ = arm_.gripper;
        arm_.set_gripper(pos_);
    }

    void undo() override {
        arm_.set_gripper(prev_pos_);
        std::cout << "  [undo] gripper back to " << arm_.gripper << "\n";
    }

    std::string name() const override {
        return "SetGripper(" + std::to_string(pos_) + ")";
    }
};

// Macro command: group multiple commands as one
class MacroCommand : public Command {
    std::string                              macro_name_;
    std::vector<std::unique_ptr<Command>>    commands_;

public:
    explicit MacroCommand(const std::string& n) : macro_name_(n) {}

    void add(std::unique_ptr<Command> cmd) {
        commands_.push_back(std::move(cmd));
    }

    void execute() override {
        std::cout << "  [Macro:" << macro_name_ << "] executing "
                  << commands_.size() << " commands\n";
        for (auto& c : commands_) c->execute();
    }

    void undo() override {
        // Undo in reverse order
        for (auto it = commands_.rbegin(); it != commands_.rend(); ++it) {
            (*it)->undo();
        }
    }

    std::string name() const override { return "Macro(" + macro_name_ + ")"; }
};

// ──────────────────────────────────────────────────────────────────────────
// PART 4: Command Invoker with undo history
// ──────────────────────────────────────────────────────────────────────────
class CommandInvoker {
    std::stack<std::unique_ptr<Command>> history_;

public:
    void execute(std::unique_ptr<Command> cmd) {
        std::cout << "[Invoker] execute: " << cmd->name() << "\n";
        cmd->execute();
        history_.push(std::move(cmd));
    }

    void undo() {
        if (history_.empty()) {
            std::cout << "[Invoker] nothing to undo\n";
            return;
        }
        auto& cmd = history_.top();
        std::cout << "[Invoker] undo: " << cmd->name() << "\n";
        cmd->undo();
        history_.pop();
    }

    void undo_all() {
        while (!history_.empty()) undo();
    }

    size_t history_size() const { return history_.size(); }
};

// ──────────────────────────────────────────────────────────────────────────
// PART 5: Async command (ROS2 Action-like)
// ──────────────────────────────────────────────────────────────────────────
struct ActionResult {
    bool        success;
    std::string message;
    double      final_angle;
};

struct ActionFeedback {
    int    step;
    double current_angle;
    double progress_pct;
};

class MoveToAngleAction {
    RobotArm&                            arm_;
    std::function<void(ActionFeedback)>  feedback_cb_;
    std::atomic<bool>                    cancel_requested_{false};

public:
    explicit MoveToAngleAction(RobotArm& arm,
                                std::function<void(ActionFeedback)> fb = nullptr)
        : arm_(arm), feedback_cb_(std::move(fb)) {}

    // Execute asynchronously — returns future<ActionResult>
    std::future<ActionResult> execute_async(double target_angle) {
        return std::async(std::launch::async, [this, target_angle]() {
            return run(target_angle);
        });
    }

    void cancel() { cancel_requested_ = true; }

private:
    ActionResult run(double target_angle) {
        const int steps = 10;
        double start_angle = arm_.joint_angle;
        double step_size   = (target_angle - start_angle) / steps;

        for (int i = 1; i <= steps; ++i) {
            if (cancel_requested_) {
                return {false, "Action cancelled", arm_.joint_angle};
            }
            arm_.move_joint(step_size);

            if (feedback_cb_) {
                feedback_cb_({i, arm_.joint_angle,
                              100.0 * i / steps});
            }
            std::this_thread::sleep_for(30ms);
        }

        return {true, "Reached target", arm_.joint_angle};
    }
};

// ──────────────────────────────────────────────────────────────────────────
// DEMOS
// ──────────────────────────────────────────────────────────────────────────
void demo_command_undo() {
    std::cout << "\n=== Command Pattern with Undo ===\n";
    RobotArm arm;
    CommandInvoker invoker;

    invoker.execute(std::make_unique<MoveJointCommand>(arm, 30.0));
    invoker.execute(std::make_unique<MoveJointCommand>(arm, 45.0));
    invoker.execute(std::make_unique<SetGripperCommand>(arm, 1.0));

    arm.print_state();

    std::cout << "\n--- Undo last 2 commands ---\n";
    invoker.undo();
    invoker.undo();
    arm.print_state();
}

void demo_macro_command() {
    std::cout << "\n=== Macro Command (pick sequence) ===\n";
    RobotArm arm;
    CommandInvoker invoker;

    auto pick = std::make_unique<MacroCommand>("pick_object");
    pick->add(std::make_unique<MoveJointCommand>(arm, 45.0));
    pick->add(std::make_unique<SetGripperCommand>(arm, 0.0));    // open
    pick->add(std::make_unique<MoveJointCommand>(arm, -15.0));
    pick->add(std::make_unique<SetGripperCommand>(arm, 1.0));    // close

    invoker.execute(std::move(pick));
    arm.print_state();

    std::cout << "\n--- Undo macro (undo all 4 steps) ---\n";
    invoker.undo();
    arm.print_state();
}

void demo_async_action() {
    std::cout << "\n=== Async Action (ROS2 Action-like) ===\n";
    RobotArm arm;
    arm.joint_angle = 0.0;

    MoveToAngleAction action(arm, [](const ActionFeedback& fb) {
        std::cout << "  [feedback] step=" << fb.step
                  << " angle=" << fb.current_angle
                  << " (" << fb.progress_pct << "%)\n";
    });

    std::cout << "Sending goal: move to 90°\n";
    auto future = action.execute_async(90.0);

    // Do other work while action runs
    std::cout << "[main] doing other work...\n";
    std::this_thread::sleep_for(100ms);

    // Get result
    ActionResult result = future.get();
    std::cout << "Action result: success=" << result.success
              << " msg='" << result.message << "'"
              << " final_angle=" << result.final_angle << "\n";
}

int main() {
    demo_command_undo();
    demo_macro_command();
    demo_async_action();
    return 0;
}
