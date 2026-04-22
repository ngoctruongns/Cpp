/**
 * PHASE 2 - Bài 01: Variadic Templates & Fold Expressions
 *
 * Mục tiêu:
 *  - Hiểu parameter pack (typename... Args)
 *  - Đệ quy template vs fold expression (C++17)
 *  - Ứng dụng: log utility, event system, type-safe containers
 *
 * Compile: g++ -std=c++17 01_variadic_templates.cpp -o out
 */

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <type_traits>

// ─── Ví dụ 1: Hàm sum — cách cũ (đệ quy) vs C++17 (fold) ────────────────

// Cách cũ: đệ quy template
template<typename T>
T sum_recursive(T val) { return val; }  // base case

template<typename T, typename... Rest>
T sum_recursive(T first, Rest... rest) {
    return first + sum_recursive(rest...);
}

// C++17: fold expression — gọn hơn nhiều
template<typename... Args>
auto sum_fold(Args... args) {
    return (args + ...);  // unary right fold: (a + (b + (c + ...)))
}

// ─── Ví dụ 2: Print tất cả arguments (dùng fold + comma operator) ────────
template<typename... Args>
void print_all(Args&&... args) {
    // Binary left fold với operator,
    // Mỗi arg được in, cách nhau bởi space
    ((std::cout << args << ' '), ...);
    std::cout << '\n';
}

// ─── Ví dụ 3: Type-safe Logger — ứng dụng thực tế ───────────────────────
enum class LogLevel { DEBUG, INFO, WARN, ERROR };

template<typename... Args>
std::string format_log(LogLevel level, Args&&... args) {
    const char* level_str[] = {"DEBUG", "INFO ", "WARN ", "ERROR"};
    std::ostringstream oss;
    oss << "[" << level_str[static_cast<int>(level)] << "] ";
    // Fold expression: nối tất cả args vào oss
    (oss << ... << std::forward<Args>(args));
    return oss.str();
}

template<typename... Args>
void log_info(Args&&... args) {
    std::cout << format_log(LogLevel::INFO, std::forward<Args>(args)...) << '\n';
}

template<typename... Args>
void log_warn(Args&&... args) {
    std::cout << format_log(LogLevel::WARN, std::forward<Args>(args)...) << '\n';
}

// ─── Ví dụ 4: sizeof... — đếm số arguments ───────────────────────────────
template<typename... Types>
void count_args(Types...) {
    std::cout << "Number of args: " << sizeof...(Types) << '\n';
}

// ─── Ví dụ 5: all_of / any_of với fold ───────────────────────────────────
// Check tất cả điều kiện (giống std::all_of nhưng variadic)
template<typename... Bools>
bool all_true(Bools... bools) {
    return (... && bools);  // unary left fold
}

template<typename... Bools>
bool any_true(Bools... bools) {
    return (... || bools);  // unary left fold
}

// ─── Ví dụ 6: Lưu nhiều giá trị cùng type vào vector ────────────────────
template<typename T, typename... Args>
std::vector<T> make_vector(Args&&... args) {
    // Check at compile time: tất cả Args phải convertible về T
    static_assert((std::is_convertible_v<Args, T> && ...),
                  "All arguments must be convertible to T");
    return {static_cast<T>(args)...};
}

// ─── Ví dụ 7: Gọi hàm cho từng element trong pack ───────────────────────
// Kỹ thuật này dùng nhiều trong ROS2 callback registration
template<typename Func, typename... Args>
void for_each_arg(Func&& func, Args&&... args) {
    (func(std::forward<Args>(args)), ...);
}

// ─── Ví dụ 8: Minimal Tuple implementation ───────────────────────────────
// Hiểu cách std::tuple được implement
template<typename... Types> struct MyTuple {};

template<typename Head, typename... Tail>
struct MyTuple<Head, Tail...> : MyTuple<Tail...> {
    Head value;
    explicit MyTuple(Head h, Tail... tail) : MyTuple<Tail...>(tail...), value(h) {}
};

// Base case
template<>
struct MyTuple<> {};

int main() {
    // === Ví dụ 1: sum ===
    std::cout << "=== Sum ===\n";
    std::cout << "sum_recursive(1,2,3,4) = " << sum_recursive(1, 2, 3, 4) << "\n";
    std::cout << "sum_fold(1.5, 2.0, 3.5) = " << sum_fold(1.5, 2.0, 3.5) << "\n";

    // === Ví dụ 2: print_all ===
    std::cout << "\n=== Print All ===\n";
    print_all("robot:", "x=", 1.5, "y=", 2.3, "vx=", 0.5);

    // === Ví dụ 3: Logger ===
    std::cout << "\n=== Logger ===\n";
    log_info("Robot started at position (", 0.0, ", ", 0.0, ")");
    log_warn("Battery low: ", 15, "% remaining");
    std::cout << format_log(LogLevel::ERROR, "Sensor timeout after ", 500, "ms") << '\n';

    // === Ví dụ 4: sizeof... ===
    std::cout << "\n=== sizeof... ===\n";
    count_args(1, 2.0, "hello", true);  // 4

    // === Ví dụ 5: all/any ===
    std::cout << "\n=== all_of / any_of ===\n";
    bool sensors_ok = all_true(true, true, true);
    bool has_error  = any_true(false, true, false);
    std::cout << "All sensors OK: " << sensors_ok << "\n";
    std::cout << "Has error: " << has_error << "\n";

    // === Ví dụ 6: make_vector ===
    std::cout << "\n=== make_vector ===\n";
    auto readings = make_vector<double>(1.5, 2, 3.14f, 4);
    for (auto v : readings) std::cout << v << " ";
    std::cout << "\n";

    // === Ví dụ 7: for_each_arg ===
    std::cout << "\n=== for_each_arg ===\n";
    std::vector<std::string> node_names;
    for_each_arg(
        [&node_names](const std::string& name) { node_names.push_back(name); },
        "camera_node", "lidar_node", "planner_node", "controller_node"
    );
    std::cout << "Registered " << node_names.size() << " nodes: ";
    for (const auto& n : node_names) std::cout << n << " ";
    std::cout << "\n";

    return 0;
}

/**
 * ═══ BÀI TẬP TỰ LÀM ═══════════════════════════════════════════════════════
 *
 * 1. Viết `max_of(args...)` dùng fold expression trả về giá trị lớn nhất.
 *
 * 2. Viết `make_shared_vec<Base>(args...)` nhận variadic arguments là các
 *    derived class instances, trả về vector<shared_ptr<Base>>.
 *    Ví dụ: make_shared_vec<Sensor>(LidarSensor{}, IMUSensor{}, CameraNode{})
 *
 * 3. Viết `invoke_all(funcs...)` nhận variadic callable objects, gọi tất cả
 *    chúng với cùng một argument. Dùng trong event system.
 *
 * 4. [Nâng cao] Viết `TypeList<T...>` với:
 *    - `at<N>` lấy type thứ N
 *    - `contains<T>` check xem T có trong list không
 *    - `size` = số lượng types
 *    Đây là kỹ thuật dùng trong ROS2 message introspection.
 */
