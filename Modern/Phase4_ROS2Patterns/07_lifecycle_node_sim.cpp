/**
 * PHASE 4 - Bài 07: Lifecycle Node Simulation (ROS2 Managed Node)
 *
 * Mục tiêu:
 *  - Simulate ROS2 LifecycleNode state machine
 *  - States: Unconfigured → Inactive → Active → (Finalized)
 *  - Transition callbacks: on_configure, on_activate, on_deactivate, on_cleanup
 *  - Error handling trong từng transition
 *
 * Compile: g++ -std=c++17 07_lifecycle_node_sim.cpp -o out
 */

#include <iostream>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <stdexcept>

// ──────────────────────────────────────────────────────────────────────────
// Lifecycle States (matches ROS2 Managed Node)
// ──────────────────────────────────────────────────────────────────────────
enum class LifecycleState {
    UNCONFIGURED,
    INACTIVE,
    ACTIVE,
    FINALIZED,
    ERROR_PROCESSING  // transient error state
};

std::string state_name(LifecycleState s) {
    switch(s) {
        case LifecycleState::UNCONFIGURED:    return "UNCONFIGURED";
        case LifecycleState::INACTIVE:        return "INACTIVE";
        case LifecycleState::ACTIVE:          return "ACTIVE";
        case LifecycleState::FINALIZED:       return "FINALIZED";
        case LifecycleState::ERROR_PROCESSING:return "ERROR_PROCESSING";
    }
    return "UNKNOWN";
}

// Transition result
struct TransitionResult {
    bool        success;
    std::string error_msg;

    static TransitionResult ok()                       { return {true, ""}; }
    static TransitionResult fail(const std::string& e) { return {false, e}; }
};

// ──────────────────────────────────────────────────────────────────────────
// LifecycleNode base class
// ──────────────────────────────────────────────────────────────────────────
class LifecycleNode {
    LifecycleState state_ = LifecycleState::UNCONFIGURED;
    std::string    name_;

public:
    explicit LifecycleNode(const std::string& name) : name_(name) {
        std::cout << "[" << name_ << "] created in UNCONFIGURED state\n";
    }

    virtual ~LifecycleNode() {
        std::cout << "[" << name_ << "] destroyed\n";
    }

    // ── Public lifecycle triggers ──────────────────────────────────────────
    bool configure() { return transition("configure",
        LifecycleState::UNCONFIGURED, LifecycleState::INACTIVE,
        [this]{ return on_configure(); }); }

    bool activate() { return transition("activate",
        LifecycleState::INACTIVE, LifecycleState::ACTIVE,
        [this]{ return on_activate(); }); }

    bool deactivate() { return transition("deactivate",
        LifecycleState::ACTIVE, LifecycleState::INACTIVE,
        [this]{ return on_deactivate(); }); }

    bool cleanup() { return transition("cleanup",
        LifecycleState::INACTIVE, LifecycleState::UNCONFIGURED,
        [this]{ return on_cleanup(); }); }

    bool shutdown() {
        // Can be called from INACTIVE or ACTIVE
        if (state_ == LifecycleState::ACTIVE) deactivate();
        if (state_ == LifecycleState::INACTIVE) cleanup();
        state_ = LifecycleState::FINALIZED;
        on_shutdown();
        std::cout << "  [" << name_ << "] → FINALIZED\n";
        return true;
    }

    LifecycleState get_state() const { return state_; }
    const std::string& get_name() const { return name_; }

    bool is_active() const { return state_ == LifecycleState::ACTIVE; }

protected:
    // ── Override these in derived nodes ───────────────────────────────────
    virtual TransitionResult on_configure()  { return TransitionResult::ok(); }
    virtual TransitionResult on_activate()   { return TransitionResult::ok(); }
    virtual TransitionResult on_deactivate() { return TransitionResult::ok(); }
    virtual TransitionResult on_cleanup()    { return TransitionResult::ok(); }
    virtual void             on_shutdown()   {}

    // Tick when ACTIVE
    virtual void on_timer_callback() {}

private:
    bool transition(const std::string& name,
                    LifecycleState from,
                    LifecycleState to,
                    std::function<TransitionResult()> fn)
    {
        if (state_ != from) {
            std::cout << "  [" << name_ << "] INVALID transition '" << name
                      << "' from " << state_name(state_) << "\n";
            return false;
        }

        std::cout << "  [" << name_ << "] " << name << "(): "
                  << state_name(from) << " → ";
        std::cout.flush();

        TransitionResult result = fn();

        if (result.success) {
            state_ = to;
            std::cout << state_name(to) << "\n";
        } else {
            state_ = LifecycleState::ERROR_PROCESSING;
            std::cout << "ERROR_PROCESSING (" << result.error_msg << ")\n";
        }
        return result.success;
    }
};

// ──────────────────────────────────────────────────────────────────────────
// Concrete lifecycle nodes
// ──────────────────────────────────────────────────────────────────────────
class LidarNode : public LifecycleNode {
    // Simulated hardware handle
    bool hw_initialized_ = false;
    int  scan_count_     = 0;

public:
    explicit LidarNode(const std::string& name) : LifecycleNode(name) {}

protected:
    TransitionResult on_configure() override {
        std::cout << "    → initializing LiDAR parameters\n";
        // Simulate potential failure
        // return TransitionResult::fail("LiDAR not found");
        hw_initialized_ = false;
        return TransitionResult::ok();
    }

    TransitionResult on_activate() override {
        std::cout << "    → opening LiDAR hardware\n";
        hw_initialized_ = true;
        scan_count_ = 0;
        return TransitionResult::ok();
    }

    TransitionResult on_deactivate() override {
        std::cout << "    → stopping LiDAR scan (processed " << scan_count_ << " scans)\n";
        hw_initialized_ = false;
        return TransitionResult::ok();
    }

    TransitionResult on_cleanup() override {
        std::cout << "    → releasing LiDAR resources\n";
        return TransitionResult::ok();
    }

    void on_shutdown() override {
        std::cout << "    → LiDAR node shutdown\n";
    }

public:
    void tick() {
        if (!is_active()) return;
        ++scan_count_;
        // Simulate rare error
        if (scan_count_ == 5) {
            std::cout << "    → [LidarNode] scan error detected\n";
        }
        std::cout << "    → [LidarNode] scan #" << scan_count_ << " published\n";
    }
};

class NavigationNode : public LifecycleNode {
    std::vector<std::shared_ptr<LifecycleNode>> managed_nodes_;

public:
    explicit NavigationNode(const std::string& name) : LifecycleNode(name) {}

    void add_managed(std::shared_ptr<LifecycleNode> node) {
        managed_nodes_.push_back(std::move(node));
    }

protected:
    TransitionResult on_configure() override {
        std::cout << "    → configuring managed nodes\n";
        for (auto& n : managed_nodes_) {
            if (!n->configure()) {
                return TransitionResult::fail("Failed to configure: " + n->get_name());
            }
        }
        return TransitionResult::ok();
    }

    TransitionResult on_activate() override {
        std::cout << "    → activating managed nodes\n";
        for (auto& n : managed_nodes_) {
            if (!n->activate()) {
                return TransitionResult::fail("Failed to activate: " + n->get_name());
            }
        }
        return TransitionResult::ok();
    }

    TransitionResult on_deactivate() override {
        std::cout << "    → deactivating managed nodes\n";
        for (auto it = managed_nodes_.rbegin(); it != managed_nodes_.rend(); ++it) {
            (*it)->deactivate();
        }
        return TransitionResult::ok();
    }

    TransitionResult on_cleanup() override {
        for (auto it = managed_nodes_.rbegin(); it != managed_nodes_.rend(); ++it) {
            (*it)->cleanup();
        }
        return TransitionResult::ok();
    }
};

int main() {
    std::cout << "===== Lifecycle Node Simulation =====\n\n";

    // --- Single node lifecycle ---
    std::cout << "--- Single LidarNode ---\n";
    LidarNode lidar("lidar_node");

    lidar.configure();
    lidar.activate();
    lidar.tick();
    lidar.tick();
    lidar.deactivate();
    lidar.activate();    // reactivate
    lidar.tick();
    lidar.tick();
    lidar.tick();        // scan #5 triggers warning
    lidar.shutdown();

    std::cout << "\n--- Invalid transition ---\n";
    lidar.configure();   // FINALIZED → invalid

    // --- Hierarchical lifecycle manager ---
    std::cout << "\n--- Hierarchical Navigation Node ---\n";
    auto nav = std::make_shared<NavigationNode>("nav_node");
    auto sensor1 = std::make_shared<LidarNode>("lidar_front");
    auto sensor2 = std::make_shared<LidarNode>("lidar_rear");
    nav->add_managed(sensor1);
    nav->add_managed(sensor2);

    // Nav node manages all sensors
    nav->configure();  // configures sensor1, sensor2
    nav->activate();   // activates sensor1, sensor2

    sensor1->tick();
    sensor2->tick();

    nav->deactivate(); // deactivates all
    nav->shutdown();

    return 0;
}
