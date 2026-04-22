/**
 * PHASE 4 - Bài 06: Parameter Server Simulation (ROS2 params-like)
 *
 * Mục tiêu:
 *  - Key-value store với type-safe access
 *  - Parameter change callbacks (on_set_parameters)
 *  - Parameter validation
 *  - Thread-safe read/write
 *
 * Compile: g++ -std=c++17 -pthread 06_param_server_sim.cpp -o out
 */

#include <iostream>
#include <string>
#include <variant>
#include <unordered_map>
#include <vector>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <optional>
#include <type_traits>

// ──────────────────────────────────────────────────────────────────────────
// Parameter Value (type-safe variant)
// ──────────────────────────────────────────────────────────────────────────
using ParamValue = std::variant<
    bool,
    int,
    double,
    std::string,
    std::vector<double>,
    std::vector<int>,
    std::vector<std::string>
>;

enum class ParamType { BOOL, INT, DOUBLE, STRING, DOUBLE_ARRAY, INT_ARRAY, STRING_ARRAY };

std::string param_type_name(const ParamValue& v) {
    return std::visit([](const auto& val) -> std::string {
        using T = std::decay_t<decltype(val)>;
        if constexpr      (std::is_same_v<T, bool>)                    return "bool";
        else if constexpr (std::is_same_v<T, int>)                     return "int";
        else if constexpr (std::is_same_v<T, double>)                  return "double";
        else if constexpr (std::is_same_v<T, std::string>)             return "string";
        else if constexpr (std::is_same_v<T, std::vector<double>>)     return "double[]";
        else if constexpr (std::is_same_v<T, std::vector<int>>)        return "int[]";
        else if constexpr (std::is_same_v<T, std::vector<std::string>>)return "string[]";
        else return "unknown";
    }, v);
}

std::string param_value_str(const ParamValue& v) {
    return std::visit([](const auto& val) -> std::string {
        using T = std::decay_t<decltype(val)>;
        if constexpr (std::is_same_v<T, bool>)   return val ? "true" : "false";
        else if constexpr (std::is_same_v<T, int>)    return std::to_string(val);
        else if constexpr (std::is_same_v<T, double>) return std::to_string(val);
        else if constexpr (std::is_same_v<T, std::string>) return "\"" + val + "\"";
        else if constexpr (std::is_same_v<T, std::vector<double>>  ||
                           std::is_same_v<T, std::vector<int>>) {
            std::string s = "[";
            for (size_t i = 0; i < val.size(); ++i) {
                if (i) s += ",";
                s += std::to_string(val[i]);
            }
            return s + "]";
        } else if constexpr (std::is_same_v<T, std::vector<std::string>>) {
            std::string s = "[";
            for (size_t i = 0; i < val.size(); ++i) {
                if (i) s += ",";
                s += "\"" + val[i] + "\"";
            }
            return s + "]";
        }
        return "?";
    }, v);
}

// ──────────────────────────────────────────────────────────────────────────
// Parameter Descriptor (metadata + validation)
// ──────────────────────────────────────────────────────────────────────────
struct ParamDescriptor {
    std::string description;
    bool        read_only = false;
    // Optional range for numeric types
    std::optional<double> min_value;
    std::optional<double> max_value;
};

// ──────────────────────────────────────────────────────────────────────────
// Parameter Server
// ──────────────────────────────────────────────────────────────────────────
class ParameterServer {
public:
    using Callback = std::function<bool(const std::string& name,
                                        const ParamValue& old_val,
                                        const ParamValue& new_val)>;

    struct Param {
        ParamValue      value;
        ParamDescriptor descriptor;
    };

private:
    std::unordered_map<std::string, Param> params_;
    std::vector<Callback>                  callbacks_;
    mutable std::mutex                     mtx_;

public:
    // Declare a parameter with initial value
    void declare(const std::string& name, ParamValue value,
                 ParamDescriptor desc = {}) {
        std::lock_guard<std::mutex> lock(mtx_);
        if (params_.count(name)) {
            throw std::runtime_error("Parameter already declared: " + name);
        }
        params_[name] = {std::move(value), std::move(desc)};
        std::cout << "[ParamServer] declared '" << name << "'"
                  << " type=" << param_type_name(params_[name].value)
                  << " value=" << param_value_str(params_[name].value) << "\n";
    }

    // Get parameter value, throws if not found or wrong type
    template<typename T>
    T get(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mtx_);
        auto it = params_.find(name);
        if (it == params_.end())
            throw std::runtime_error("Parameter not found: " + name);
        if (!std::holds_alternative<T>(it->second.value))
            throw std::runtime_error("Parameter type mismatch: " + name);
        return std::get<T>(it->second.value);
    }

    // Get with default (no throw)
    template<typename T>
    T get_or(const std::string& name, const T& default_val) const {
        try { return get<T>(name); }
        catch (...) { return default_val; }
    }

    // Set parameter value — runs callbacks and validation
    bool set(const std::string& name, ParamValue new_value) {
        std::unique_lock<std::mutex> lock(mtx_);
        auto it = params_.find(name);
        if (it == params_.end()) {
            std::cout << "[ParamServer] WARNING: undeclared param '" << name << "' set\n";
            params_[name] = {std::move(new_value), {}};
            return true;
        }

        if (it->second.descriptor.read_only) {
            std::cout << "[ParamServer] REJECTED: '" << name << "' is read-only\n";
            return false;
        }

        // Type check: must not change type
        if (new_value.index() != it->second.value.index()) {
            std::cout << "[ParamServer] REJECTED: type mismatch for '" << name << "'\n";
            return false;
        }

        ParamValue old_value = it->second.value;

        // Run callbacks (outside lock to avoid deadlock)
        lock.unlock();
        for (const auto& cb : callbacks_) {
            if (!cb(name, old_value, new_value)) {
                std::cout << "[ParamServer] REJECTED by callback: '" << name << "'\n";
                return false;
            }
        }
        lock.lock();

        it->second.value = std::move(new_value);
        std::cout << "[ParamServer] '" << name << "' = "
                  << param_value_str(it->second.value) << "\n";
        return true;
    }

    // Register change callback
    void add_on_set_parameters_callback(Callback cb) {
        std::lock_guard<std::mutex> lock(mtx_);
        callbacks_.push_back(std::move(cb));
    }

    bool has(const std::string& name) const {
        std::lock_guard<std::mutex> lock(mtx_);
        return params_.count(name) > 0;
    }

    void list_all() const {
        std::lock_guard<std::mutex> lock(mtx_);
        std::cout << "=== Parameters ===\n";
        for (const auto& [name, p] : params_) {
            std::cout << "  " << name
                      << " (" << param_type_name(p.value) << ")"
                      << (p.descriptor.read_only ? " [RO]" : "")
                      << " = " << param_value_str(p.value)
                      << "\n";
            if (!p.descriptor.description.empty())
                std::cout << "    → " << p.descriptor.description << "\n";
        }
    }
};

// ──────────────────────────────────────────────────────────────────────────
// Demo: Navigation node using parameter server
// ──────────────────────────────────────────────────────────────────────────
class NavNode {
    ParameterServer& params_;

public:
    explicit NavNode(ParameterServer& ps) : params_(ps) {
        // Declare parameters
        params_.declare("max_velocity",       1.5,
                         {"Maximum linear velocity in m/s"});
        params_.declare("max_angular_vel",    0.5,
                         {"Maximum angular velocity in rad/s"});
        params_.declare("robot_name",         std::string{"my_robot"},
                         {"Robot name for logging"});
        params_.declare("use_sim_time",       false,
                         {"Use simulation time", true});  // read-only
        params_.declare("sensor_topics",      std::vector<std::string>{"/scan", "/imu"},
                         {"Subscribed sensor topics"});
        params_.declare("pid_gains",          std::vector<double>{1.0, 0.1, 0.01},
                         {"PID gains: [kp, ki, kd]"});

        // Validation callback: reject negative velocities
        params_.add_on_set_parameters_callback(
            [](const std::string& name, const ParamValue& old_val, const ParamValue& new_val) {
                if (name == "max_velocity" || name == "max_angular_vel") {
                    if (std::holds_alternative<double>(new_val) &&
                        std::get<double>(new_val) <= 0.0) {
                        std::cout << "  [validation] " << name << " must be > 0\n";
                        return false;
                    }
                }
                return true;
            }
        );
    }

    void configure() {
        double max_vel = params_.get<double>("max_velocity");
        std::string name = params_.get<std::string>("robot_name");
        auto topics = params_.get<std::vector<std::string>>("sensor_topics");
        auto gains = params_.get<std::vector<double>>("pid_gains");

        std::cout << "\n[NavNode] Configured:\n";
        std::cout << "  robot=" << name << " max_vel=" << max_vel << "\n";
        std::cout << "  topics: ";
        for (const auto& t : topics) std::cout << t << " ";
        std::cout << "\n";
        std::cout << "  PID: kp=" << gains[0] << " ki=" << gains[1]
                  << " kd=" << gains[2] << "\n";
    }
};

int main() {
    std::cout << "===== Parameter Server Simulation =====\n\n";

    ParameterServer params;
    NavNode node(params);

    node.configure();

    std::cout << "\n--- Parameter updates ---\n";
    params.set("max_velocity", 2.0);                    // OK
    params.set("max_velocity", -1.0);                   // Rejected by validator
    params.set("use_sim_time", true);                   // Rejected: read-only
    params.set("robot_name", std::string{"robot_v2"});  // OK
    params.set("pid_gains", std::vector<double>{2.0, 0.2, 0.05});  // OK

    std::cout << "\n--- Type mismatch ---\n";
    params.set("max_velocity", std::string{"fast"});  // Rejected: type mismatch

    std::cout << "\n";
    params.list_all();

    return 0;
}
