/**
 * PHASE 2 - Bài 10: Generic Robot Message Serializer (Bài tổng hợp)
 *
 * Kết hợp toàn bộ Phase 2:
 *  - Variadic templates: serialize arbitrary field lists
 *  - Fold expressions: format all fields in one line
 *  - Template specialization: type-specific serializers
 *  - SFINAE / type traits: detect container, arithmetic, message
 *  - if constexpr: compile-time dispatch
 *  - Policy-based: choose output format (binary/json/text)
 *  - CRTP: auto-generate serialize/deserialize for messages
 *
 * Compile: g++ -std=c++17 10_generic_robot_msg.cpp -o out
 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <type_traits>
#include <stdexcept>
#include <cstring>
#include <memory>

// ══════════════════════════════════════════════════════════════════════════
// PART 1: Type serialization traits & policies
// ══════════════════════════════════════════════════════════════════════════

// Detect vector<T>
template<typename T>
struct is_vector : std::false_type {};
template<typename T>
struct is_vector<std::vector<T>> : std::true_type {};
template<typename T>
inline constexpr bool is_vector_v = is_vector<T>::value;

// Detect array<T,N>
template<typename T>
struct is_std_array : std::false_type {};
template<typename T, size_t N>
struct is_std_array<std::array<T,N>> : std::true_type {};
template<typename T>
inline constexpr bool is_std_array_v = is_std_array<T>::value;

// ─── Primary value serializer (JSON-like text) ───────────────────────────
template<typename T>
std::string serialize_value(const T& val) {
    if constexpr (std::is_same_v<T, std::string>) {
        return "\"" + val + "\"";
    } else if constexpr (std::is_same_v<T, bool>) {
        return val ? "true" : "false";
    } else if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(val);
    } else if constexpr (is_vector_v<T>) {
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < val.size(); ++i) {
            if (i) oss << ",";
            oss << serialize_value(val[i]);
        }
        oss << "]";
        return oss.str();
    } else if constexpr (is_std_array_v<T>) {
        std::ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < val.size(); ++i) {
            if (i) oss << ",";
            oss << serialize_value(val[i]);
        }
        oss << "]";
        return oss.str();
    } else {
        // Assumes the type has a serialize() method (via CRTP below)
        return val.serialize();
    }
}

// ─── Primary value deserializer (simplified — numbers only) ─────────────
template<typename T>
T deserialize_value(const std::string& s) {
    if constexpr (std::is_same_v<T, std::string>) {
        // Un-quote
        if (s.size() >= 2 && s.front() == '"') return s.substr(1, s.size()-2);
        return s;
    } else if constexpr (std::is_same_v<T, bool>) {
        return s == "true" || s == "1";
    } else if constexpr (std::is_same_v<T, int> || std::is_same_v<T, int32_t>) {
        return std::stoi(s);
    } else if constexpr (std::is_same_v<T, uint32_t> || std::is_same_v<T, uint64_t>) {
        return static_cast<T>(std::stoul(s));
    } else if constexpr (std::is_same_v<T, float>) {
        return std::stof(s);
    } else if constexpr (std::is_same_v<T, double>) {
        return std::stod(s);
    } else {
        return T{};
    }
}

// ══════════════════════════════════════════════════════════════════════════
// PART 2: CRTP base for automatic message serialization
// ══════════════════════════════════════════════════════════════════════════

template<typename Derived>
class SerializableMessage {
public:
    using SharedPtr = std::shared_ptr<Derived>;

    // Derived must implement: void serialize_fields(std::ostringstream&) const
    std::string serialize() const {
        std::ostringstream oss;
        oss << "{";
        static_cast<const Derived*>(this)->serialize_fields(oss);
        oss << "}";
        return oss.str();
    }

    void print() const {
        std::cout << Derived::TYPE_NAME << ": " << serialize() << "\n";
    }

    static SharedPtr create() { return std::make_shared<Derived>(); }
};

// Helper macro-like template for field serialization
// (In real use, consider reflection libraries for C++23)
template<typename T>
void append_field(std::ostringstream& oss, bool& first,
                  const std::string& name, const T& val) {
    if (!first) oss << ",";
    first = false;
    oss << "\"" << name << "\":" << serialize_value(val);
}

// ══════════════════════════════════════════════════════════════════════════
// PART 3: Concrete message types
// ══════════════════════════════════════════════════════════════════════════

struct Header : public SerializableMessage<Header> {
    static constexpr const char* TYPE_NAME = "std_msgs/Header";

    uint64_t    stamp_ns  = 0;
    std::string frame_id;

    void serialize_fields(std::ostringstream& oss) const {
        bool f = true;
        append_field(oss, f, "stamp_ns",  stamp_ns);
        append_field(oss, f, "frame_id",  frame_id);
    }
};

struct LaserScan : public SerializableMessage<LaserScan> {
    static constexpr const char* TYPE_NAME = "sensor_msgs/LaserScan";

    Header             header;
    float              angle_min       = 0.0f;
    float              angle_max       = 0.0f;
    float              angle_increment = 0.0f;
    float              range_min       = 0.0f;
    float              range_max       = 0.0f;
    std::vector<float> ranges;
    std::vector<float> intensities;

    void serialize_fields(std::ostringstream& oss) const {
        bool f = true;
        append_field(oss, f, "header",          header.serialize());
        append_field(oss, f, "angle_min",        angle_min);
        append_field(oss, f, "angle_max",        angle_max);
        append_field(oss, f, "angle_increment",  angle_increment);
        append_field(oss, f, "range_min",        range_min);
        append_field(oss, f, "range_max",        range_max);
        append_field(oss, f, "ranges",           ranges);
        append_field(oss, f, "intensities",      intensities);
    }
};

struct Imu : public SerializableMessage<Imu> {
    static constexpr const char* TYPE_NAME = "sensor_msgs/Imu";

    Header header;
    std::array<double, 3> linear_acceleration  = {};
    std::array<double, 3> angular_velocity      = {};
    std::array<double, 4> orientation           = {};  // quaternion

    void serialize_fields(std::ostringstream& oss) const {
        bool f = true;
        append_field(oss, f, "header",              header.serialize());
        append_field(oss, f, "linear_acceleration", linear_acceleration);
        append_field(oss, f, "angular_velocity",    angular_velocity);
        append_field(oss, f, "orientation",         orientation);
    }
};

struct Twist : public SerializableMessage<Twist> {
    static constexpr const char* TYPE_NAME = "geometry_msgs/Twist";

    std::array<double, 3> linear  = {};
    std::array<double, 3> angular = {};

    void serialize_fields(std::ostringstream& oss) const {
        bool f = true;
        append_field(oss, f, "linear",  linear);
        append_field(oss, f, "angular", angular);
    }
};

// ══════════════════════════════════════════════════════════════════════════
// PART 4: Generic message bus (type-erased, variadic)
// ══════════════════════════════════════════════════════════════════════════

class MessageBus {
    std::unordered_map<std::string, std::vector<std::string>> log_;

public:
    template<typename MsgT>
    void publish(const std::string& topic, const MsgT& msg) {
        std::string serialized = msg.serialize();
        log_[topic].push_back(serialized);
        std::cout << "[Bus] publish '" << topic << "': "
                  << serialized.substr(0, 80)
                  << (serialized.size() > 80 ? "..." : "") << "\n";
    }

    void print_history(const std::string& topic) const {
        auto it = log_.find(topic);
        if (it == log_.end()) { std::cout << "No history for " << topic << "\n"; return; }
        std::cout << "History for '" << topic << "' (" << it->second.size() << " msgs):\n";
        for (const auto& s : it->second) std::cout << "  " << s << "\n";
    }
};

// ══════════════════════════════════════════════════════════════════════════
// MAIN
// ══════════════════════════════════════════════════════════════════════════
int main() {
    std::cout << "===== Generic Robot Message Serializer =====\n\n";

    // --- Build messages ---
    Header hdr;
    hdr.stamp_ns = 1234567890ULL;
    hdr.frame_id = "laser_frame";
    std::cout << "--- Header ---\n";
    hdr.print();

    LaserScan scan;
    scan.header      = hdr;
    scan.angle_min   = -1.5708f;
    scan.angle_max   =  1.5708f;
    scan.angle_increment = 0.01f;
    scan.range_min   = 0.1f;
    scan.range_max   = 30.0f;
    scan.ranges      = {1.0f, 1.5f, 2.0f, 2.5f, 3.0f};
    scan.intensities = {100.0f, 200.0f, 150.0f, 180.0f, 220.0f};
    std::cout << "\n--- LaserScan ---\n";
    scan.print();

    Imu imu;
    imu.header = hdr;
    imu.linear_acceleration  = {0.01, -0.02,  9.81};
    imu.angular_velocity     = {0.1,   0.0,  -0.05};
    imu.orientation          = {0.0, 0.0, 0.0, 1.0};
    std::cout << "\n--- IMU ---\n";
    imu.print();

    Twist cmd;
    cmd.linear  = {0.5, 0.0, 0.0};
    cmd.angular = {0.0, 0.0, 0.2};
    std::cout << "\n--- Twist ---\n";
    cmd.print();

    // --- Publish through the message bus ---
    std::cout << "\n--- Message Bus ---\n";
    MessageBus bus;
    bus.publish("/scan",   scan);
    bus.publish("/imu",    imu);
    bus.publish("/cmd_vel", cmd);
    bus.publish("/scan",   scan);  // second publish

    std::cout << "\n--- Bus History ---\n";
    bus.print_history("/scan");

    // --- SharedPtr factory (CRTP provides create()) ---
    std::cout << "\n--- SharedPtr factory ---\n";
    auto scan_ptr = LaserScan::create();
    scan_ptr->header.frame_id = "base_laser";
    scan_ptr->ranges = {5.0f, 6.0f};
    scan_ptr->print();

    return 0;
}
