/**
 * PHASE 2 - Bài 05: if constexpr (C++17)
 *
 * Mục tiêu:
 *  - Compile-time branching với if constexpr
 *  - Thay thế SFINAE phức tạp bằng if constexpr đơn giản
 *  - Kết hợp với type_traits để xử lý các type khác nhau
 *  - Ứng dụng: generic serializer, message converter
 *
 * Compile: g++ -std=c++17 05_if_constexpr.cpp -o out
 */

#include <iostream>
#include <string>
#include <vector>
#include <type_traits>
#include <sstream>
#include <tuple>

// ─── Ví dụ 1: if constexpr vs if runtime ──────────────────────────────────

// WRONG: if thường không loại bỏ code ở compile time → compile error
// template<typename T>
// void print_size_WRONG(T val) {
//     if (std::is_integral_v<T>)
//         std::cout << val.size();  // compile error for int even though branch not taken
// }

// CORRECT: if constexpr loại bỏ branch không match → không compile branch đó
template<typename T>
void print_info(T val) {
    std::cout << "Type: ";
    if constexpr (std::is_integral_v<T>) {
        std::cout << "integral, value=" << val
                  << ", is_signed=" << std::is_signed_v<T> << "\n";
    } else if constexpr (std::is_floating_point_v<T>) {
        std::cout << "float, value=" << val << "\n";
    } else if constexpr (std::is_same_v<T, std::string>) {
        std::cout << "string, value=\"" << val
                  << "\", length=" << val.size() << "\n";  // OK: .size() compiled only for string
    } else {
        std::cout << "unknown\n";
    }
}

void demo_if_constexpr_basics() {
    std::cout << "\n=== if constexpr Basics ===\n";
    print_info(42);
    print_info(3.14f);
    print_info(3.14);
    print_info(std::string{"hello"});
    print_info('c');
}

// ─── Ví dụ 2: Generic to_string (thay thế overloading phức tạp) ─────────
template<typename T>
std::string to_str(const T& val) {
    if constexpr (std::is_same_v<T, std::string>) {
        return val;
    } else if constexpr (std::is_same_v<T, bool>) {
        return val ? "true" : "false";
    } else if constexpr (std::is_arithmetic_v<T>) {
        return std::to_string(val);
    } else if constexpr (std::is_pointer_v<T>) {
        if (val == nullptr) return "nullptr";
        std::ostringstream oss;
        oss << static_cast<const void*>(val);
        return oss.str();
    } else {
        return "[non-printable]";
    }
}

void demo_to_str() {
    std::cout << "\n=== Generic to_str with if constexpr ===\n";
    std::cout << to_str(42)                << "\n";
    std::cout << to_str(3.14)              << "\n";
    std::cout << to_str(true)              << "\n";
    std::cout << to_str(std::string{"hi"}) << "\n";
    int n = 7;
    std::cout << to_str(&n)                << "\n";  // pointer address
}

// ─── Ví dụ 3: Recursive variadic unpacking với if constexpr ──────────────
// In tuple-like structure: nếu không có if constexpr, cần base case overload
template<size_t I, typename Tuple>
void print_tuple_impl(const Tuple& t) {
    if constexpr (I < std::tuple_size_v<Tuple>) {
        std::cout << std::get<I>(t);
        if constexpr (I + 1 < std::tuple_size_v<Tuple>) std::cout << ", ";
        print_tuple_impl<I + 1>(t);  // compile-time recursion
    }
}

template<typename... Args>
void print_tuple(const std::tuple<Args...>& t) {
    std::cout << '(';
    print_tuple_impl<0>(t);
    std::cout << ')' << '\n';
}

void demo_tuple_print() {
    std::cout << "\n=== Tuple Print with if constexpr ===\n";
    auto t1 = std::make_tuple(1, 2.5, std::string{"ros2"}, true);
    print_tuple(t1);

    auto t2 = std::make_tuple("lidar", 3.14f, 42);
    print_tuple(t2);
}

// ─── Ví dụ 4: Generic ROS2-style message serializer ──────────────────────
// Serialize any struct whose fields are arithmetic or string
template<typename T>
void serialize_field(std::ostringstream& oss, const std::string& name, const T& val) {
    oss << name << ": ";
    if constexpr (std::is_arithmetic_v<T>) {
        oss << val;
    } else if constexpr (std::is_same_v<T, std::string>) {
        oss << "\"" << val << "\"";
    } else if constexpr (std::is_same_v<T, std::vector<float>> ||
                         std::is_same_v<T, std::vector<double>>) {
        oss << "[";
        for (size_t i = 0; i < val.size(); ++i) {
            if (i) oss << ", ";
            oss << val[i];
        }
        oss << "]";
    } else {
        oss << "<complex>";
    }
    oss << "\n";
}

struct Header { std::string frame_id; uint64_t stamp_ns; };
struct LaserScan {
    Header             header;
    std::vector<float> ranges;
    float angle_min, angle_max;
};

std::string serialize_laser(const LaserScan& s) {
    std::ostringstream oss;
    oss << "LaserScan:\n";
    serialize_field(oss, "  frame_id",  s.header.frame_id);
    serialize_field(oss, "  stamp_ns",  s.header.stamp_ns);
    serialize_field(oss, "  ranges",    s.ranges);
    serialize_field(oss, "  angle_min", s.angle_min);
    serialize_field(oss, "  angle_max", s.angle_max);
    return oss.str();
}

void demo_ros2_serializer() {
    std::cout << "\n=== ROS2 Message Serializer with if constexpr ===\n";
    LaserScan scan;
    scan.header = {"laser_frame", 1000000};
    scan.ranges = {1.0f, 1.5f, 2.0f, 2.5f};
    scan.angle_min = -1.5708f;
    scan.angle_max =  1.5708f;
    std::cout << serialize_laser(scan);
}

// ─── Ví dụ 5: Compile-time selection với if constexpr ────────────────────
// Choose between add vs string concatenation at compile time
template<typename T>
T combine(T a, T b) {
    if constexpr (std::is_arithmetic_v<T>) {
        return a + b;  // numeric addition
    } else if constexpr (std::is_same_v<T, std::string>) {
        return a + " " + b;  // string concatenation with space
    } else {
        static_assert(sizeof(T) == 0, "combine: unsupported type");
    }
}

void demo_combine() {
    std::cout << "\n=== Compile-time combine ===\n";
    std::cout << combine(3, 4)                               << "\n";   // 7
    std::cout << combine(1.5, 2.5)                           << "\n";   // 4.0
    std::cout << combine(std::string{"hello"}, std::string{"world"}) << "\n"; // "hello world"
}

int main() {
    demo_if_constexpr_basics();
    demo_to_str();
    demo_tuple_print();
    demo_ros2_serializer();
    demo_combine();
    return 0;
}
