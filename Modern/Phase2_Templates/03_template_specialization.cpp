/**
 * PHASE 2 - Bài 03: Template Specialization
 *
 * Mục tiêu:
 *  - Full specialization: override cho type cụ thể
 *  - Partial specialization: override cho class template
 *  - Specialization cho pointer types
 *  - Ứng dụng: type-specific serialization (ROS2 messages)
 *
 * Compile: g++ -std=c++17 03_template_specialization.cpp -o out
 */

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <type_traits>

// ─── Ví dụ 1: Full specialization của function template ─────────────────
template<typename T>
T max_val(T a, T b) {
    std::cout << "[primary] max_val called\n";
    return (a > b) ? a : b;
}

// Full specialization for const char*: compare strings, not pointers
template<>
const char* max_val<const char*>(const char* a, const char* b) {
    std::cout << "[spec: const char*] max_val called\n";
    return (std::string(a) > std::string(b)) ? a : b;
}

// Full specialization for bool (example: XOR semantic)
template<>
bool max_val<bool>(bool a, bool b) {
    std::cout << "[spec: bool] max_val (OR semantic)\n";
    return a || b;  // redefine "max" for bool
}

void demo_function_spec() {
    std::cout << "\n=== Function Template Specialization ===\n";
    std::cout << "max_val(3, 7)       = " << max_val(3, 7) << "\n";
    std::cout << "max_val(3.14, 2.72) = " << max_val(3.14, 2.72) << "\n";
    std::cout << "max_val('a','z')    = " << max_val('a', 'z') << "\n";
    std::cout << "max_val(\"apple\",\"zebra\") = " << max_val("apple", "zebra") << "\n";
    std::cout << "max_val(true, false) = " << max_val(true, false) << "\n";
}

// ─── Ví dụ 2: Full specialization của class template ────────────────────

// Generic serializer: convert to string
template<typename T>
class Serializer {
public:
    static std::string serialize(const T& val) {
        return std::to_string(val);  // works for numeric types
    }
    static T deserialize(const std::string& s) {
        if constexpr (std::is_same_v<T, int>)    return std::stoi(s);
        if constexpr (std::is_same_v<T, float>)  return std::stof(s);
        if constexpr (std::is_same_v<T, double>) return std::stod(s);
        return T{};  // fallback
    }
};

// Full specialization for std::string
template<>
class Serializer<std::string> {
public:
    static std::string serialize(const std::string& val) {
        return "\"" + val + "\"";  // wrap in quotes
    }
    static std::string deserialize(const std::string& s) {
        // Remove surrounding quotes
        if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
            return s.substr(1, s.size() - 2);
        return s;
    }
};

// Full specialization for bool
template<>
class Serializer<bool> {
public:
    static std::string serialize(bool val) {
        return val ? "true" : "false";
    }
    static bool deserialize(const std::string& s) {
        return (s == "true" || s == "1");
    }
};

void demo_class_full_spec() {
    std::cout << "\n=== Class Template Full Specialization ===\n";
    std::cout << "int:    " << Serializer<int>::serialize(42) << "\n";
    std::cout << "double: " << Serializer<double>::serialize(3.14) << "\n";
    std::cout << "string: " << Serializer<std::string>::serialize("hello") << "\n";
    std::cout << "bool:   " << Serializer<bool>::serialize(true)  << "\n";

    std::cout << "deserialize int '99': "   << Serializer<int>::deserialize("99") << "\n";
    std::cout << "deserialize bool 'true': " << Serializer<bool>::deserialize("true") << "\n";
    std::cout << "deserialize string: "
              << Serializer<std::string>::deserialize("\"world\"") << "\n";
}

// ─── Ví dụ 3: Partial specialization (class template only) ───────────────

// Generic storage: uses copy
template<typename T>
class Storage {
    T value_;
public:
    explicit Storage(const T& v) : value_(v) {
        std::cout << "[Storage<T>] copy stored\n";
    }
    T get() const { return value_; }
};

// Partial specialization for pointer types: store and optionally dereference
template<typename T>
class Storage<T*> {
    T* ptr_;
public:
    explicit Storage(T* p) : ptr_(p) {
        std::cout << "[Storage<T*>] pointer stored\n";
    }
    T* get() const { return ptr_; }
    T& deref() const { return *ptr_; }
};

// Partial specialization for std::vector<T>
template<typename T>
class Storage<std::vector<T>> {
    std::vector<T> data_;
public:
    explicit Storage(std::vector<T> v) : data_(std::move(v)) {
        std::cout << "[Storage<vector<T>>] vector stored, size=" << data_.size() << "\n";
    }
    size_t size() const { return data_.size(); }
    const T& at(size_t i) const { return data_.at(i); }
    const std::vector<T>& get() const { return data_; }
};

void demo_partial_spec() {
    std::cout << "\n=== Partial Specialization ===\n";

    Storage<int> si(42);
    std::cout << "Storage<int>.get() = " << si.get() << "\n";

    int x = 100;
    Storage<int*> sp(&x);
    std::cout << "Storage<int*>.deref() = " << sp.deref() << "\n";

    Storage<std::vector<double>> sv({1.1, 2.2, 3.3});
    std::cout << "Storage<vector<double>>[1] = " << sv.at(1) << "\n";
}

// ─── Ví dụ 4: Specialization cho ROS2-style type dispatch ────────────────
// Imagine different ROS message types needing different processing

struct LaserScan { std::vector<float> ranges; };
struct Imu       { double ax, ay, az; };
struct Image     { int width, height; std::vector<uint8_t> data; };

template<typename MsgT>
class MessagePrinter {
public:
    static void print(const MsgT&) {
        std::cout << "[MessagePrinter] Unknown message type\n";
    }
};

template<>
class MessagePrinter<LaserScan> {
public:
    static void print(const LaserScan& msg) {
        std::cout << "[LaserScan] ranges count=" << msg.ranges.size() << "\n";
    }
};

template<>
class MessagePrinter<Imu> {
public:
    static void print(const Imu& msg) {
        std::cout << "[Imu] accel=(" << msg.ax << "," << msg.ay << "," << msg.az << ")\n";
    }
};

template<>
class MessagePrinter<Image> {
public:
    static void print(const Image& msg) {
        std::cout << "[Image] " << msg.width << "x" << msg.height
                  << " pixels=" << msg.data.size() << "\n";
    }
};

void demo_ros2_type_dispatch() {
    std::cout << "\n=== ROS2-style type dispatch via specialization ===\n";
    LaserScan scan{{1.0f, 2.0f, 3.0f, 4.0f}};
    Imu imu{0.1, -0.2, 9.81};
    Image img{640, 480, std::vector<uint8_t>(640*480*3)};

    MessagePrinter<LaserScan>::print(scan);
    MessagePrinter<Imu>::print(imu);
    MessagePrinter<Image>::print(img);
}

int main() {
    demo_function_spec();
    demo_class_full_spec();
    demo_partial_spec();
    demo_ros2_type_dispatch();
    return 0;
}
