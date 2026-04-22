/**
 * PHASE 1 - Bài 09: Perfect Forwarding & Forwarding References
 *
 * Mục tiêu:
 *  - Hiểu forwarding reference (T&&) vs rvalue reference
 *  - std::forward bảo toàn value category khi truyền qua template
 *  - Tại sao cần forward: wrapper, factory, emplace
 *  - Universal reference trong lambda (C++20)
 *
 * Compile: g++ -std=c++17 -O0 09_perfect_forwarding.cpp -o out
 */

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <type_traits>

// ─── Helper: in ra value category ────────────────────────────────────────
void handle(const std::string& s) { std::cout << "  handle(const&): " << s << "\n"; }
void handle(std::string&&      s) { std::cout << "  handle(&&):     " << s << "\n"; }

// ─── Ví dụ 1: Vấn đề không có perfect forwarding ─────────────────────────
// Wrapper dùng T&& nhưng không forward → luôn gửi lvalue vào handle
template<typename T>
void bad_wrapper(T&& val) {
    // WRONG: trong thân hàm, mọi named param đều là lvalue!
    // Dù T&& là rvalue ref, 'val' khi đứng trong body là lvalue
    handle(val);  // luôn gọi handle(const&)
}

// ─── Ví dụ 2: Fix với std::forward ───────────────────────────────────────
template<typename T>
void good_wrapper(T&& val) {
    // std::forward<T>(val): nếu T=string& → forward lvalue
    //                        nếu T=string  → forward rvalue
    handle(std::forward<T>(val));
}

void demo_forwarding_basics() {
    std::cout << "\n=== Perfect Forwarding Basics ===\n";

    std::string s = "hello";

    std::cout << "bad_wrapper(s) [lvalue]:\n";
    bad_wrapper(s);                          // T=string& → val là lvalue

    std::cout << "bad_wrapper(string{rvalue}) [rvalue]:\n";
    bad_wrapper(std::string{"rvalue"});       // T=string (deduced) → but still lvalue!

    std::cout << "\ngood_wrapper(s) [lvalue]:\n";
    good_wrapper(s);                         // → handle(const&)

    std::cout << "good_wrapper(string{rvalue}) [rvalue]:\n";
    good_wrapper(std::string{"rvalue"});      // → handle(&&)

    std::cout << "good_wrapper(std::move(s)) [rvalue from lvalue]:\n";
    good_wrapper(std::move(s));              // → handle(&&)
}

// ─── Ví dụ 3: Factory function với perfect forwarding ────────────────────
class Actuator {
public:
    std::string name;
    double      init_angle;

    Actuator(const std::string& n, double a) : name(n), init_angle(a) {
        std::cout << "[+] Actuator '" << name << "' (copy ctor path)\n";
    }
    Actuator(std::string&& n, double a) : name(std::move(n)), init_angle(a) {
        std::cout << "[+] Actuator '" << name << "' (move ctor path)\n";
    }
};

// make_actuator: forward args → correct ctor selected
template<typename... Args>
std::unique_ptr<Actuator> make_actuator(Args&&... args) {
    return std::make_unique<Actuator>(std::forward<Args>(args)...);
}

void demo_factory_forwarding() {
    std::cout << "\n=== Factory with Perfect Forwarding ===\n";

    std::string name = "servo_0";

    // lvalue → copy ctor path
    auto a1 = make_actuator(name, 0.0);

    // rvalue (temporary) → move ctor path
    auto a2 = make_actuator(std::string{"servo_1"}, 45.0);

    // std::move(lvalue) → move ctor path
    auto a3 = make_actuator(std::move(name), 90.0);
    std::cout << "name after move: '" << name << "'\n";  // empty/unspecified
}

// ─── Ví dụ 4: emplace_back vs push_back ──────────────────────────────────
class HeavyObj {
public:
    std::string id;
    std::vector<double> data;

    HeavyObj(const std::string& s, size_t n) : id(s), data(n, 0.0) {
        std::cout << "  [HeavyObj] constructed '" << id << "'\n";
    }
    HeavyObj(const HeavyObj&) {
        std::cout << "  [HeavyObj] COPY constructed\n";
    }
    HeavyObj(HeavyObj&&) noexcept {
        std::cout << "  [HeavyObj] MOVE constructed\n";
    }
};

void demo_emplace_vs_push() {
    std::cout << "\n=== emplace_back vs push_back ===\n";

    std::vector<HeavyObj> v;
    v.reserve(4);  // prevent realloc for clarity

    std::cout << "push_back(HeavyObj{...}):\n";
    v.push_back(HeavyObj("obj0", 1000));  // construct + move

    std::cout << "emplace_back(args...):\n";
    v.emplace_back("obj1", 1000);  // construct IN PLACE — no move!

    // emplace_back forwards args → directly constructs → zero copies/moves
}

// ─── Ví dụ 5: Forwarding trong lambda (C++20 auto&& param) ───────────────
void demo_lambda_forwarding() {
    std::cout << "\n=== Lambda with Forwarding (C++20) ===\n";

    // C++20: auto&& trong lambda param = forwarding reference
    auto forward_to_handle = [](auto&& val) {
        handle(std::forward<decltype(val)>(val));
    };

    std::string s = "test";
    forward_to_handle(s);              // → handle(const&)
    forward_to_handle(std::move(s));   // → handle(&&)
    forward_to_handle(std::string{"tmp"});  // → handle(&&)
}

// ─── Ví dụ 6: Phân biệt T&& forwarding ref vs rvalue ref ─────────────────
// T&& trong template = forwarding reference (phụ thuộc type deduction)
// int&& (concrete type) = rvalue reference (chỉ bind rvalue)
void demo_fwd_vs_rvalue_ref() {
    std::cout << "\n=== Forwarding Reference vs Rvalue Reference ===\n";

    // Rvalue reference: chỉ bind rvalue
    // void f(int&&);  — f(x) với x là lvalue sẽ không compile

    // Forwarding reference: bind cả lvalue và rvalue
    auto fwd = [](auto&& x) {
        if constexpr (std::is_lvalue_reference_v<decltype(x)>) {
            std::cout << "  Forwarding ref received LVALUE\n";
        } else {
            std::cout << "  Forwarding ref received RVALUE\n";
        }
    };

    int n = 5;
    fwd(n);        // lvalue
    fwd(42);       // rvalue (literal)
    fwd(std::move(n));  // rvalue (cast)
}

int main() {
    demo_forwarding_basics();
    demo_factory_forwarding();
    demo_emplace_vs_push();
    demo_lambda_forwarding();
    demo_fwd_vs_rvalue_ref();
    return 0;
}
