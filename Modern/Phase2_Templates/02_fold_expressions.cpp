/**
 * PHASE 2 - Bài 02: Fold Expressions (C++17)
 *
 * Mục tiêu:
 *  - Hiểu 4 dạng fold: unary left/right, binary left/right
 *  - Thay thế đệ quy template bằng fold expression
 *  - Ứng dụng: sum, all_of, any_of, print, type pack
 *
 * Compile: g++ -std=c++17 02_fold_expressions.cpp -o out
 */

#include <iostream>
#include <string>
#include <vector>
#include <type_traits>

// ─── 4 dạng fold expression ───────────────────────────────────────────────
// Unary right:  (pack op ...)        →  (a op (b op (c op ...)))
// Unary left:   (... op pack)        →  (((... op a) op b) op c)
// Binary right: (pack op ... op init)→  (a op (b op (c op init)))
// Binary left:  (init op ... op pack)→  (((init op a) op b) op c)

// ─── Ví dụ 1: Sum với fold ───────────────────────────────────────────────
template<typename... Args>
auto sum(Args... args) {
    return (args + ...);     // unary right fold
}

template<typename... Args>
auto product(Args... args) {
    return (args * ...);     // unary right fold
}

// Với init value (identity element) → safe nếu pack rỗng
template<typename... Args>
auto sum_safe(Args... args) {
    return (0 + ... + args); // binary left fold  → (((0+a1)+a2)+a3)
}

template<typename... Args>
auto product_safe(Args... args) {
    return (1 * ... * args); // binary left fold
}

void demo_arithmetic_fold() {
    std::cout << "\n=== Arithmetic Fold ===\n";
    std::cout << "sum(1,2,3,4,5)     = " << sum(1, 2, 3, 4, 5) << "\n";
    std::cout << "product(1,2,3,4,5) = " << product(1, 2, 3, 4, 5) << "\n";
    std::cout << "sum_safe()         = " << sum_safe() << "\n";  // OK, returns 0
    std::cout << "sum_safe(10,20,30) = " << sum_safe(10, 20, 30) << "\n";
}

// ─── Ví dụ 2: All / Any với fold ────────────────────────────────────────
template<typename... Bools>
bool all_of(Bools... bs) {
    return (bs && ...);   // unary right fold — short-circuits
}

template<typename... Bools>
bool any_of(Bools... bs) {
    return (bs || ...);   // unary right fold — short-circuits
}

template<typename... Bools>
bool none_of(Bools... bs) {
    return !(bs || ...);
}

void demo_logical_fold() {
    std::cout << "\n=== Logical Fold ===\n";
    std::cout << "all_of(true,true,true)   = " << all_of(true, true, true)  << "\n";
    std::cout << "all_of(true,false,true)  = " << all_of(true, false, true) << "\n";
    std::cout << "any_of(false,false,true) = " << any_of(false, false, true)<< "\n";
    std::cout << "none_of(false,false)     = " << none_of(false, false)     << "\n";
}

// ─── Ví dụ 3: Print với comma operator fold ──────────────────────────────
template<typename... Args>
void print_space(Args&&... args) {
    ((std::cout << args << ' '), ...);  // unary left fold with comma op
    std::cout << '\n';
}

template<typename Sep, typename... Args>
void print_sep(Sep sep, Args&&... args) {
    bool first = true;
    // Binary left fold không có comma → dùng lambda trick
    auto print_one = [&](const auto& v) {
        if (!first) std::cout << sep;
        std::cout << v;
        first = false;
    };
    (print_one(args), ...);
    std::cout << '\n';
}

void demo_print_fold() {
    std::cout << "\n=== Print Fold ===\n";
    print_space(1, 2.5, "hello", 'x', true);
    print_sep(", ", "lidar", "imu", "camera", "gps");
}

// ─── Ví dụ 4: sizeof... + fold để count ──────────────────────────────────
template<typename... Args>
constexpr size_t count_args(Args...) {
    return sizeof...(Args);
}

// Count how many args satisfy a predicate (using fold on int + ternary)
template<typename Pred, typename... Args>
size_t count_if_fold(Pred pred, Args&&... args) {
    return (0 + ... + (pred(args) ? 1 : 0));  // binary left fold
}

void demo_count_fold() {
    std::cout << "\n=== Count Fold ===\n";
    std::cout << "count_args(1,2,3,4) = " << count_args(1, 2, 3, 4) << "\n";

    auto is_positive = [](auto x) { return x > 0; };
    std::cout << "count_if positive(-1, 2, -3, 4, 5) = "
              << count_if_fold(is_positive, -1, 2, -3, 4, 5) << "\n";  // 3
}

// ─── Ví dụ 5: Push to vector via fold ────────────────────────────────────
template<typename T, typename... Args>
void push_many(std::vector<T>& vec, Args&&... args) {
    (vec.push_back(std::forward<Args>(args)), ...);  // unary left fold
}

void demo_push_fold() {
    std::cout << "\n=== Push Fold ===\n";
    std::vector<int> v;
    push_many(v, 10, 20, 30, 40, 50);
    std::cout << "vector: ";
    for (int x : v) std::cout << x << ' ';
    std::cout << '\n';
}

// ─── Ví dụ 6: Type check fold ────────────────────────────────────────────
// Check nếu tất cả args cùng type T
template<typename T, typename... Args>
constexpr bool all_same_as() {
    return (std::is_same_v<T, Args> && ...);  // fold over types
}

void demo_type_fold() {
    std::cout << "\n=== Type Fold ===\n";
    constexpr bool b1 = all_same_as<int, int, int, int>();
    constexpr bool b2 = all_same_as<int, int, double, int>();
    std::cout << "all int  (int,int,int):       " << b1 << "\n";
    std::cout << "all int  (int,double,int):    " << b2 << "\n";
}

int main() {
    demo_arithmetic_fold();
    demo_logical_fold();
    demo_print_fold();
    demo_count_fold();
    demo_push_fold();
    demo_type_fold();
    return 0;
}
