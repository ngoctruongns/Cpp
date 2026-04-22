/**
 * PHASE 2 - Bài 06: Custom Type Traits
 *
 * Mục tiêu:
 *  - Hiểu cách type traits được xây dựng
 *  - Tự viết is_pointer, remove_const, is_container
 *  - Dùng integral_constant, true_type, false_type
 *  - Ứng dụng: compile-time introspection cho ROS2 message types
 *
 * Compile: g++ -std=c++17 06_type_traits_custom.cpp -o out
 */

#include <iostream>
#include <type_traits>
#include <string>
#include <vector>
#include <list>
#include <array>
#include <memory>

// ─── Building block: integral_constant ───────────────────────────────────
// std::integral_constant<bool, true>  == std::true_type
// std::integral_constant<bool, false> == std::false_type
// Both expose ::value (bool) and operator bool()

// ─── Ví dụ 1: Custom is_pointer ──────────────────────────────────────────
// Primary template: assume NOT a pointer
template<typename T>
struct my_is_pointer : std::false_type {};

// Specialization for T*: IS a pointer
template<typename T>
struct my_is_pointer<T*> : std::true_type {};

// Specialization for const T*: also a pointer
template<typename T>
struct my_is_pointer<const T*> : std::true_type {};

// Helper variable template (C++17 style)
template<typename T>
inline constexpr bool my_is_pointer_v = my_is_pointer<T>::value;

void demo_is_pointer() {
    std::cout << "\n=== Custom is_pointer ===\n";
    std::cout << "my_is_pointer<int>       = " << my_is_pointer_v<int>       << "\n";
    std::cout << "my_is_pointer<int*>      = " << my_is_pointer_v<int*>      << "\n";
    std::cout << "my_is_pointer<const int*>= " << my_is_pointer_v<const int*><< "\n";
    std::cout << "my_is_pointer<int&>      = " << my_is_pointer_v<int&>      << "\n";
    std::cout << "my_is_pointer<double*>   = " << my_is_pointer_v<double*>   << "\n";

    // Verify against std version
    static_assert(my_is_pointer_v<int*>  == std::is_pointer_v<int*>);
    static_assert(my_is_pointer_v<int>   == std::is_pointer_v<int>);
}

// ─── Ví dụ 2: Custom remove_const ────────────────────────────────────────
template<typename T>
struct my_remove_const { using type = T; };

template<typename T>
struct my_remove_const<const T> { using type = T; };

template<typename T>
using my_remove_const_t = typename my_remove_const<T>::type;

// Also: remove_reference
template<typename T>
struct my_remove_ref { using type = T; };

template<typename T>
struct my_remove_ref<T&> { using type = T; };

template<typename T>
struct my_remove_ref<T&&> { using type = T; };

template<typename T>
using my_remove_ref_t = typename my_remove_ref<T>::type;

// Combine: remove_cvref (remove const/volatile + reference) — std has this in C++20
template<typename T>
struct my_remove_cvref {
    using type = my_remove_const_t<
                   std::remove_volatile_t<
                     my_remove_ref_t<T>>>;
};

template<typename T>
using my_remove_cvref_t = typename my_remove_cvref<T>::type;

void demo_remove_traits() {
    std::cout << "\n=== Custom remove_const/ref ===\n";

    static_assert(std::is_same_v<my_remove_const_t<const int>, int>);
    static_assert(std::is_same_v<my_remove_const_t<int>,       int>);
    static_assert(std::is_same_v<my_remove_ref_t<int&>,        int>);
    static_assert(std::is_same_v<my_remove_ref_t<int&&>,       int>);
    static_assert(std::is_same_v<my_remove_cvref_t<const int&>, int>);
    static_assert(std::is_same_v<my_remove_cvref_t<int&&>,      int>);

    std::cout << "All remove_const/ref static_asserts passed!\n";

    // Print type names via is_same
    using T1 = my_remove_const_t<const double>;
    using T2 = my_remove_ref_t<std::string&>;
    std::cout << "remove_const<const double> == double: "
              << std::is_same_v<T1, double> << "\n";
    std::cout << "remove_ref<string&> == string: "
              << std::is_same_v<T2, std::string> << "\n";
}

// ─── Ví dụ 3: Custom is_container ────────────────────────────────────────
// Detect if T has begin(), end(), size() → is a container

// SFINAE-based detection using void_t (C++17)
template<typename T, typename = void>
struct my_is_container : std::false_type {};

template<typename T>
struct my_is_container<T, std::void_t<
    decltype(std::declval<T>().begin()),
    decltype(std::declval<T>().end()),
    decltype(std::declval<T>().size())
>> : std::true_type {};

template<typename T>
inline constexpr bool my_is_container_v = my_is_container<T>::value;

void demo_is_container() {
    std::cout << "\n=== Custom is_container ===\n";
    std::cout << "is_container<vector<int>>  = " << my_is_container_v<std::vector<int>>  << "\n";
    std::cout << "is_container<list<double>> = " << my_is_container_v<std::list<double>> << "\n";
    std::cout << "is_container<string>       = " << my_is_container_v<std::string>       << "\n";
    std::cout << "is_container<int>          = " << my_is_container_v<int>               << "\n";
    std::cout << "is_container<int[5]>       = " << my_is_container_v<int[5]>            << "\n";
    std::cout << "is_container<array<int,3>> = " << my_is_container_v<std::array<int,3>> << "\n";
}

// ─── Ví dụ 4: ROS2 message type detection ───────────────────────────────
// In ROS2: messages have a nested type alias 'SharedPtr' and 'ConstSharedPtr'
// We can detect this to write generic message utilities

template<typename T, typename = void>
struct is_ros_message : std::false_type {};

template<typename T>
struct is_ros_message<T, std::void_t<
    typename T::SharedPtr,
    typename T::ConstSharedPtr
>> : std::true_type {};

template<typename T>
inline constexpr bool is_ros_message_v = is_ros_message<T>::value;

// Mock ROS message structs
struct LaserScan {
    using SharedPtr      = std::shared_ptr<LaserScan>;
    using ConstSharedPtr = std::shared_ptr<const LaserScan>;
    std::vector<float> ranges;
};

struct NotAMessage { int data; };

// Generic function that behaves differently for ROS messages
template<typename T>
void process(const T& obj) {
    if constexpr (is_ros_message_v<T>) {
        std::cout << "[ROS msg] processing message\n";
    } else {
        std::cout << "[plain]   processing plain object\n";
    }
}

void demo_ros_type_traits() {
    std::cout << "\n=== ROS2 message detection ===\n";
    std::cout << "is_ros_message<LaserScan>    = " << is_ros_message_v<LaserScan>    << "\n";
    std::cout << "is_ros_message<NotAMessage>  = " << is_ros_message_v<NotAMessage>  << "\n";
    std::cout << "is_ros_message<int>          = " << is_ros_message_v<int>          << "\n";

    LaserScan scan;
    NotAMessage plain{42};
    process(scan);
    process(plain);
    process(123);
}

// ─── Ví dụ 5: conditional — select type at compile time ──────────────────
template<bool Condition, typename TrueType, typename FalseType>
struct my_conditional { using type = TrueType; };

template<typename TrueType, typename FalseType>
struct my_conditional<false, TrueType, FalseType> { using type = FalseType; };

template<bool C, typename T, typename F>
using my_conditional_t = typename my_conditional<C, T, F>::type;

void demo_conditional() {
    std::cout << "\n=== Custom conditional ===\n";
    using T1 = my_conditional_t<true,  int,    double>;
    using T2 = my_conditional_t<false, int,    double>;
    using T3 = my_conditional_t<(sizeof(int) == 4), int32_t, int64_t>;

    std::cout << "conditional<true,int,double> == int:    " << std::is_same_v<T1, int>    << "\n";
    std::cout << "conditional<false,int,double>== double: " << std::is_same_v<T2, double> << "\n";
    std::cout << "conditional<4==4,i32,i64>    == i32:   " << std::is_same_v<T3, int32_t><< "\n";
}

int main() {
    demo_is_pointer();
    demo_remove_traits();
    demo_is_container();
    demo_ros_type_traits();
    demo_conditional();
    return 0;
}
