/**
 * PHASE 4 - Bài 01: RAII Resource Guard
 *
 * Mục tiêu:
 *  - RAII pattern: resource tied to object lifetime
 *  - Generic ScopeGuard: run cleanup on scope exit
 *  - Hardware resource guards cho robotics
 *  - Ứng dụng trong ROS2: guard hardware init/deinit
 *
 * Compile: g++ -std=c++17 01_raii_resource_guard.cpp -o out
 */

#include <iostream>
#include <functional>
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

// ─── Ví dụ 1: Generic ScopeGuard ─────────────────────────────────────────
// Run a cleanup function when scope exits (on both normal and exception paths)
class ScopeGuard {
    std::function<void()> cleanup_;
    bool dismissed_ = false;

public:
    explicit ScopeGuard(std::function<void()> fn)
        : cleanup_(std::move(fn)) {}

    // Move-only (no copy)
    ScopeGuard(ScopeGuard&& other) noexcept
        : cleanup_(std::move(other.cleanup_))
        , dismissed_(other.dismissed_)
    {
        other.dismissed_ = true;
    }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    ~ScopeGuard() {
        if (!dismissed_ && cleanup_) cleanup_();
    }

    // Dismiss so cleanup does NOT run (success path)
    void dismiss() { dismissed_ = true; }
};

// Helper macro-like factory
ScopeGuard on_scope_exit(std::function<void()> fn) {
    return ScopeGuard(std::move(fn));
}

void demo_scope_guard() {
    std::cout << "\n=== ScopeGuard Demo ===\n";

    {
        auto guard = on_scope_exit([]() {
            std::cout << "  [cleanup] ScopeGuard fired on normal exit\n";
        });
        std::cout << "  [work] doing work...\n";
        // cleanup runs here (normal exit)
    }

    std::cout << "\n  Exception path:\n";
    try {
        auto guard = on_scope_exit([]() {
            std::cout << "  [cleanup] ScopeGuard fired on exception exit\n";
        });
        std::cout << "  [work] about to throw...\n";
        throw std::runtime_error("simulated error");
    } catch (const std::exception& e) {
        std::cout << "  [catch] " << e.what() << "\n";
    }

    std::cout << "\n  Dismissed path (success, no cleanup):\n";
    {
        auto guard = on_scope_exit([]() {
            std::cout << "  [cleanup] This should NOT print\n";
        });
        std::cout << "  [work] success — dismissing guard\n";
        guard.dismiss();  // cancel the cleanup
    }
}

// ─── Ví dụ 2: Hardware Resource RAII ─────────────────────────────────────
class SerialPort {
    std::string device_;
    int         fd_ = -1;

    static int open_port(const std::string& dev) {
        std::cout << "[SerialPort] opening " << dev << "\n";
        // Simulated fd
        static int counter = 10;
        return counter++;
    }

    static void close_port(int fd) {
        std::cout << "[SerialPort] closing fd=" << fd << "\n";
    }

public:
    explicit SerialPort(const std::string& dev) : device_(dev) {
        fd_ = open_port(dev);
        if (fd_ < 0) throw std::runtime_error("Failed to open: " + dev);
    }

    // Rule of Five: non-copyable, movable
    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    SerialPort(SerialPort&& other) noexcept
        : device_(std::move(other.device_)), fd_(other.fd_)
    {
        other.fd_ = -1;  // prevent double-close
    }

    SerialPort& operator=(SerialPort&& other) noexcept {
        if (this != &other) {
            if (fd_ >= 0) close_port(fd_);
            device_ = std::move(other.device_);
            fd_ = other.fd_;
            other.fd_ = -1;
        }
        return *this;
    }

    ~SerialPort() {
        if (fd_ >= 0) close_port(fd_);
    }

    bool is_open() const { return fd_ >= 0; }
    int  fd() const { return fd_; }

    void write(const std::string& msg) const {
        if (fd_ < 0) throw std::runtime_error("Port not open");
        std::cout << "[SerialPort fd=" << fd_ << "] write: " << msg << "\n";
    }
};

void demo_serial_port_raii() {
    std::cout << "\n=== SerialPort RAII ===\n";
    {
        SerialPort p1("/dev/ttyUSB0");
        p1.write("INIT_SENSOR");

        SerialPort p2 = std::move(p1);  // move
        std::cout << "p1.is_open=" << p1.is_open()
                  << ", p2.is_open=" << p2.is_open() << "\n";
        p2.write("READ_DATA");
    }
    // p2 closed automatically; p1 already moved-from (fd=-1, no close)
}

// ─── Ví dụ 3: Multi-resource RAII with dependency ordering ───────────────
class CAN_Bus {
    std::string name_;
public:
    explicit CAN_Bus(const std::string& n) : name_(n) {
        std::cout << "[CAN_Bus] " << name_ << " initialized\n";
    }
    ~CAN_Bus() {
        std::cout << "[CAN_Bus] " << name_ << " shutdown\n";
    }
    const std::string& name() const { return name_; }
};

class MotorController {
    std::string         name_;
    const CAN_Bus&      bus_;  // reference: must outlive this object
public:
    MotorController(const std::string& n, const CAN_Bus& bus)
        : name_(n), bus_(bus)
    {
        std::cout << "[Motor] " << name_ << " on bus '" << bus_.name() << "' ready\n";
    }
    ~MotorController() {
        std::cout << "[Motor] " << name_ << " stopping\n";
    }
    void move(double speed) {
        std::cout << "[Motor] " << name_ << " speed=" << speed << "\n";
    }
};

void demo_layered_raii() {
    std::cout << "\n=== Layered RAII (dependency ordering) ===\n";
    // CAN_Bus must be created FIRST and destroyed LAST
    // (MotorController depends on it)
    CAN_Bus       bus("can0");               // created first
    MotorController left ("left_wheel",  bus);   // depends on bus
    MotorController right("right_wheel", bus);   // depends on bus

    left.move(0.5);
    right.move(0.5);
    // Destroyed in reverse order: right → left → bus ✓
}

// ─── Ví dụ 4: ROS2-style hardware lifecycle ─────────────────────────────
class HardwareInterface {
public:
    enum class State { UNINITIALIZED, CONFIGURED, ACTIVE, ERROR };

    std::string name;
    State       state = State::UNINITIALIZED;

    explicit HardwareInterface(const std::string& n) : name(n) {
        std::cout << "[HW:" << name << "] created\n";
    }
    ~HardwareInterface() {
        if (state == State::ACTIVE) deactivate();
        if (state == State::CONFIGURED) cleanup();
        std::cout << "[HW:" << name << "] destroyed\n";
    }

    void configure() {
        std::cout << "[HW:" << name << "] configure()\n";
        state = State::CONFIGURED;
    }
    void activate() {
        std::cout << "[HW:" << name << "] activate()\n";
        state = State::ACTIVE;
    }
    void deactivate() {
        std::cout << "[HW:" << name << "] deactivate()\n";
        state = State::CONFIGURED;
    }
    void cleanup() {
        std::cout << "[HW:" << name << "] cleanup()\n";
        state = State::UNINITIALIZED;
    }
};

void demo_ros2_hardware_lifecycle() {
    std::cout << "\n=== ROS2 Hardware Lifecycle (RAII) ===\n";
    {
        HardwareInterface hw("diff_drive_controller");
        hw.configure();
        hw.activate();

        std::cout << "  [node] running...\n";
        // On node destruction / error → RAII ensures deactivate + cleanup
    }
}

int main() {
    demo_scope_guard();
    demo_serial_port_raii();
    demo_layered_raii();
    demo_ros2_hardware_lifecycle();
    return 0;
}
