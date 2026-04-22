/**
 * PHASE 2 - Bài 04: SFINAE & if constexpr
 *
 * Mục tiêu:
 *  - Hiểu SFINAE (Substitution Failure Is Not An Error)
 *  - Dùng std::enable_if để chọn overload dựa trên type
 *  - Dùng if constexpr (C++17) — cách hiện đại hơn SFINAE
 *  - Type traits: std::is_integral, std::is_floating_point, ...
 *
 * Compile: g++ -std=c++17 04_sfinae_enable_if.cpp -o out
 */

#include <iostream>
#include <type_traits>
#include <string>
#include <vector>
#include <sstream>
#include <cstring>

// ─── Ví dụ 1: SFINAE cơ bản với enable_if ────────────────────────────────
// Chỉ compile nếu T là integer type
template<typename T>
std::enable_if_t<std::is_integral_v<T>, std::string>
describe(T val) {
    return "integer: " + std::to_string(val);
}

// Chỉ compile nếu T là floating point type
template<typename T>
std::enable_if_t<std::is_floating_point_v<T>, std::string>
describe(T val) {
    std::ostringstream oss;
    oss << "float: " << val;
    return oss.str();
}

// Chỉ compile nếu T là std::string
template<typename T>
std::enable_if_t<std::is_same_v<T, std::string>, std::string>
describe(T val) {
    return "string: \"" + val + "\"";
}

// ─── Ví dụ 2: if constexpr (C++17) — CÁCH KHUYẾN NGHỊ ───────────────────
// Một hàm duy nhất, xử lý tất cả types bằng compile-time branching
template<typename T>
std::string describe_modern(const T& val) {
    if constexpr (std::is_integral_v<T>) {
        return "integer: " + std::to_string(val);
    } else if constexpr (std::is_floating_point_v<T>) {
        std::ostringstream oss;
        oss << "float: " << val;
        return oss.str();
    } else if constexpr (std::is_same_v<T, std::string>) {
        return "string: \"" + val + "\"";
    } else {
        return "unknown type";
    }
}

// ─── Ví dụ 3: Serialize to bytes — thực tế trong robot communication ─────
template<typename T>
std::vector<uint8_t> serialize(const T& val) {
    if constexpr (std::is_trivially_copyable_v<T>) {
        // POD types: copy bytes trực tiếp (int, float, struct đơn giản)
        std::vector<uint8_t> bytes(sizeof(T));
        std::memcpy(bytes.data(), &val, sizeof(T));
        return bytes;
    } else if constexpr (std::is_same_v<T, std::string>) {
        // String: length + data
        std::vector<uint8_t> bytes(sizeof(uint32_t) + val.size());
        uint32_t len = static_cast<uint32_t>(val.size());
        std::memcpy(bytes.data(), &len, sizeof(len));
        std::memcpy(bytes.data() + sizeof(len), val.data(), val.size());
        return bytes;
    } else {
        static_assert(std::is_trivially_copyable_v<T> || std::is_same_v<T, std::string>,
                      "Type must be trivially copyable or std::string");
    }
}

// ─── Ví dụ 4: has_print — detect nếu type có method print() ─────────────
// Kỹ thuật: detection idiom (pre-C++20, dùng void_t)
template<typename T, typename = void>
struct has_print : std::false_type {};

template<typename T>
struct has_print<T, std::void_t<decltype(std::declval<T>().print())>>
    : std::true_type {};

// Helper variable template
template<typename T>
inline constexpr bool has_print_v = has_print<T>::value;

// Hàm tự động gọi print() nếu có, hoặc dùng operator<< nếu không
template<typename T>
void smart_print(const T& val) {
    if constexpr (has_print_v<T>) {
        val.print();
    } else {
        std::cout << val << "\n";
    }
}

struct WithPrint {
    int x;
    void print() const { std::cout << "[WithPrint] x=" << x << "\n"; }
};

struct WithoutPrint {
    int x;
};
// operator<< for WithoutPrint
std::ostream& operator<<(std::ostream& os, const WithoutPrint& w) {
    return os << "[WithoutPrint] x=" << w.x;
}

// ─── Ví dụ 5: Conditional type selection ────────────────────────────────
// std::conditional<cond, TypeIfTrue, TypeIfFalse>
template<bool UseDouble>
using NumericType = std::conditional_t<UseDouble, double, float>;

template<typename T>
struct StorageType {
    // Nếu T > 4 bytes, dùng reference để tránh copy; ngược lại dùng value
    using type = std::conditional_t<(sizeof(T) > 4), const T&, T>;
};

// ─── Ví dụ 6: ROS2-style type check ─────────────────────────────────────
// Giả lập ROS2 message traits
template<typename T, typename = void>
struct is_ros_message : std::false_type {};

template<typename T>
struct is_ros_message<T, std::void_t<decltype(std::declval<T>().header)>>
    : std::true_type {};

struct StampedPose {
    struct { uint64_t stamp; std::string frame_id; } header;
    double x, y, z;
};

struct RawSensor {
    double value;
    // không có header
};

int main() {
    // === Ví dụ 1 & 2: describe ===
    std::cout << "=== SFINAE describe ===\n";
    std::cout << describe(42) << "\n";
    std::cout << describe(3.14) << "\n";
    std::cout << describe(std::string("hello")) << "\n";

    std::cout << "\n=== if constexpr describe_modern ===\n";
    std::cout << describe_modern(100) << "\n";
    std::cout << describe_modern(2.718f) << "\n";
    std::cout << describe_modern(std::string("ROS2")) << "\n";

    // === Ví dụ 3: serialize ===
    std::cout << "\n=== Serialize ===\n";
    auto int_bytes = serialize(42);
    std::cout << "int 42 serialized: " << int_bytes.size() << " bytes\n";

    auto str_bytes = serialize(std::string("lidar"));
    std::cout << "string 'lidar' serialized: " << str_bytes.size() << " bytes\n";

    // === Ví dụ 4: has_print detection ===
    std::cout << "\n=== has_print detection ===\n";
    std::cout << "WithPrint has print(): " << has_print_v<WithPrint> << "\n";
    std::cout << "WithoutPrint has print(): " << has_print_v<WithoutPrint> << "\n";
    std::cout << "int has print(): " << has_print_v<int> << "\n";

    smart_print(WithPrint{42});
    smart_print(WithoutPrint{99});
    smart_print(3.14);

    // === Ví dụ 5: conditional type ===
    std::cout << "\n=== Conditional Types ===\n";
    NumericType<true>  high_prec = 3.141592653589793;
    NumericType<false> low_prec  = 3.14f;
    std::cout << "high precision: " << high_prec << "\n";
    std::cout << "low precision: " << low_prec << "\n";

    // === Ví dụ 6: ROS2 message trait ===
    std::cout << "\n=== ROS2 Message Trait ===\n";
    std::cout << "StampedPose is ROS msg: " << is_ros_message<StampedPose>::value << "\n";
    std::cout << "RawSensor is ROS msg: " << is_ros_message<RawSensor>::value << "\n";

    return 0;
}

/**
 * ═══ BÀI TẬP TỰ LÀM ═══════════════════════════════════════════════════════
 *
 * 1. Viết `safe_cast<To>(from)` dùng if constexpr:
 *    - Nếu To và From cùng type → trả về trực tiếp
 *    - Nếu To là floating point và From là integral → static_cast
 *    - Nếu To là integral và From là floating point → round + static_cast
 *    - Ngược lại → static_assert fail
 *
 * 2. Viết `print_container(container)` dùng if constexpr:
 *    - Nếu container có method .key_type (map) → in dạng "key: value"
 *    - Nếu không → in dạng "[a, b, c]"
 *
 * 3. Viết detection idiom `has_serialize<T>` check xem T có method
 *    `std::vector<uint8_t> serialize() const` không.
 *    Dùng để tự động chọn serialization strategy trong generic message bus.
 *
 * ═══ LIÊN KẾT ROS2 ═════════════════════════════════════════════════════════
 *
 * ROS2 dùng rosidl traits để detect message properties:
 *   rosidl_typesupport_introspection_cpp::MessageMembers
 *   ros2::message_traits::IsMessage<T>
 *   ros2::message_traits::HasHeader<T>
 *
 * TypeAdapter dùng if constexpr:
 *   template<typename AdaptedType, typename RosType>
 *   struct TypeAdapter {
 *       static void convert_to_ros(const AdaptedType&, RosType&);
 *       static void convert_to_custom(const RosType&, AdaptedType&);
 *   };
 */
