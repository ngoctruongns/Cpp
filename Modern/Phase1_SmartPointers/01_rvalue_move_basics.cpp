/**
 * PHASE 1 - Bài 01: Rvalue References & std::move
 *
 * Mục tiêu:
 *  - Phân biệt lvalue / rvalue
 *  - Hiểu rvalue reference (T&&)
 *  - Dùng std::move để tránh copy tốn kém
 *  - Đo hiệu năng copy vs move
 *
 * Compile: g++ -std=c++17 -O0 01_rvalue_move_basics.cpp -o out
 */

#include <iostream>
#include <vector>
#include <string>
#include <chrono>

// ─── Ví dụ 1: Lvalue vs Rvalue ────────────────────────────────────────────
void lvalue_vs_rvalue() {
    int a = 10;       // 'a' là lvalue, 10 là rvalue
    int& ra = a;      // OK: lvalue ref bind lvalue
    // int& rb = 10;  // ERROR: lvalue ref không bind rvalue

    int&& rr = 20;    // OK: rvalue ref bind rvalue (temporary)
    // int&& rr2 = a; // ERROR: rvalue ref không bind lvalue

    std::cout << "lvalue a = " << a << "\n";
    std::cout << "rvalue ref rr = " << rr << "\n";
}

// ─── Ví dụ 2: Heavy class để đo copy vs move ─────────────────────────────
class HeavyBuffer {
public:
    std::vector<int> data;

    explicit HeavyBuffer(size_t size) : data(size, 42) {
        std::cout << "[HeavyBuffer] Constructed, size=" << size << "\n";
    }

    // Copy constructor: sao chép toàn bộ data → chậm
    HeavyBuffer(const HeavyBuffer& other) : data(other.data) {
        std::cout << "[HeavyBuffer] COPY constructor, size=" << data.size() << "\n";
    }

    // Move constructor: "ăn cắp" data → nhanh O(1)
    HeavyBuffer(HeavyBuffer&& other) noexcept : data(std::move(other.data)) {
        std::cout << "[HeavyBuffer] MOVE constructor, size=" << data.size() << "\n";
        // other.data giờ rỗng → trạng thái valid nhưng unspecified
    }

    // Copy assignment
    HeavyBuffer& operator=(const HeavyBuffer& other) {
        if (this != &other) data = other.data;
        std::cout << "[HeavyBuffer] COPY assignment\n";
        return *this;
    }

    // Move assignment
    HeavyBuffer& operator=(HeavyBuffer&& other) noexcept {
        if (this != &other) data = std::move(other.data);
        std::cout << "[HeavyBuffer] MOVE assignment\n";
        return *this;
    }

    ~HeavyBuffer() {
        std::cout << "[HeavyBuffer] Destroyed, size=" << data.size() << "\n";
    }
};

// ─── Ví dụ 3: Hàm trả về theo value → RVO/NRVO ───────────────────────────
HeavyBuffer make_buffer(size_t size) {
    HeavyBuffer buf(size);  // Compiler thường dùng RVO, không cần std::move
    return buf;             // Nếu không có RVO → tự động move (không copy)
}

// ─── Ví dụ 4: std::move trong thực tế ────────────────────────────────────
void move_into_vector() {
    std::cout << "\n=== Move vào vector ===\n";

    std::string s = "Hello ROS2";
    std::vector<std::string> vec;

    // Không dùng move: copy s vào vector, s còn nguyên
    vec.push_back(s);
    std::cout << "After push_back(s), s = \"" << s << "\"\n";

    // Dùng std::move: "chuyển" s vào vector, s trở nên rỗng
    vec.push_back(std::move(s));
    std::cout << "After push_back(move(s)), s = \"" << s << "\"\n";  // rỗng!
    std::cout << "vec[0]=" << vec[0] << ", vec[1]=" << vec[1] << "\n";
}

// ─── Bài tập: Đo thời gian copy vs move ──────────────────────────────────
void benchmark_copy_vs_move() {
    std::cout << "\n=== Benchmark: Copy vs Move ===\n";
    const size_t N = 1'000'000;

    HeavyBuffer big(N);

    auto t1 = std::chrono::high_resolution_clock::now();
    HeavyBuffer copied = big;      // COPY: O(N)
    auto t2 = std::chrono::high_resolution_clock::now();
    HeavyBuffer moved = std::move(big);  // MOVE: O(1)
    auto t3 = std::chrono::high_resolution_clock::now();

    auto copy_us = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    auto move_us = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();

    std::cout << "Copy time: " << copy_us << " µs\n";
    std::cout << "Move time: " << move_us << " µs\n";
    std::cout << "Move is ~" << (copy_us / std::max(move_us, (long)1)) << "x faster\n";
}

int main() {
    std::cout << "=== Lvalue vs Rvalue ===\n";
    lvalue_vs_rvalue();

    std::cout << "\n=== Copy vs Move Constructor ===\n";
    {
        HeavyBuffer b1(100);
        HeavyBuffer b2 = b1;             // copy
        HeavyBuffer b3 = std::move(b1); // move
    }

    move_into_vector();
    benchmark_copy_vs_move();

    std::cout << "\n=== make_buffer (RVO/move) ===\n";
    HeavyBuffer result = make_buffer(50);

    return 0;
}

/**
 * ═══ BÀI TẬP TỰ LÀM ═══════════════════════════════════════════════════════
 *
 * 1. Thêm `noexcept` vào move constructor của HeavyBuffer. Chạy lại và quan
 *    sát vector<HeavyBuffer> push_back — tại sao noexcept quan trọng?
 *
 * 2. Viết class `SensorData` chứa std::vector<double> readings. Implement
 *    đủ Rule of 5 với print statements. Test trong main().
 *
 * 3. Sau khi std::move(s), s có giá trị gì? Có được dùng tiếp không?
 *    (Trả lời: valid nhưng unspecified state — có thể assign lại, không nên read)
 *
 * 4. Tại sao trong make_buffer() KHÔNG nên viết `return std::move(buf)`?
 *    (Hint: NRVO — Named Return Value Optimization bị disabled khi dùng move)
 */
