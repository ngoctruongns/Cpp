/**
 * PHASE 1 - Bài 02: Rule of Five
 *
 * Mục tiêu:
 *  - Hiểu khi nào cần tự định nghĩa special member functions
 *  - Implement đủ 5: destructor, copy ctor, copy assign, move ctor, move assign
 *  - Phân biệt Rule of Zero vs Rule of Five
 *  - Tránh double-free và resource leak
 *
 * Compile: g++ -std=c++17 -O0 02_rule_of_five.cpp -o out
 */

#include <iostream>
#include <cstring>
#include <utility>
#include <string>
#include <vector>

// ─── Ví dụ 1: Class quản lý raw resource (cần Rule of Five) ──────────────
class RawBuffer {
public:
    char*  data_;
    size_t size_;

    // 1. Constructor
    explicit RawBuffer(size_t size, char fill = 0)
        : data_(new char[size]), size_(size)
    {
        std::memset(data_, fill, size_);
        std::cout << "[RawBuffer] Constructed size=" << size_ << "\n";
    }

    // 2. Destructor — giải phóng tài nguyên
    ~RawBuffer() {
        std::cout << "[RawBuffer] Destructed size=" << size_ << "\n";
        delete[] data_;
        data_ = nullptr;
    }

    // 3. Copy constructor — deep copy
    RawBuffer(const RawBuffer& other)
        : data_(new char[other.size_]), size_(other.size_)
    {
        std::memcpy(data_, other.data_, size_);
        std::cout << "[RawBuffer] COPY constructed size=" << size_ << "\n";
    }

    // 4. Copy assignment — deep copy + self-assignment guard
    RawBuffer& operator=(const RawBuffer& other) {
        if (this == &other) return *this;  // self-assignment guard

        // Giải phóng tài nguyên cũ
        delete[] data_;

        // Copy tài nguyên mới
        size_ = other.size_;
        data_ = new char[size_];
        std::memcpy(data_, other.data_, size_);
        std::cout << "[RawBuffer] COPY assigned size=" << size_ << "\n";
        return *this;
    }

    // 5. Move constructor — "ăn cắp" tài nguyên, O(1)
    RawBuffer(RawBuffer&& other) noexcept
        : data_(other.data_), size_(other.size_)
    {
        // Để other ở trạng thái valid nhưng rỗng → destructor safe
        other.data_ = nullptr;
        other.size_ = 0;
        std::cout << "[RawBuffer] MOVE constructed size=" << size_ << "\n";
    }

    // 6. Move assignment
    RawBuffer& operator=(RawBuffer&& other) noexcept {
        if (this == &other) return *this;

        // Giải phóng tài nguyên hiện tại
        delete[] data_;

        // "Ăn cắp" từ other
        data_ = other.data_;
        size_ = other.size_;
        other.data_ = nullptr;
        other.size_ = 0;
        std::cout << "[RawBuffer] MOVE assigned size=" << size_ << "\n";
        return *this;
    }

    void print() const {
        if (!data_) { std::cout << "[RawBuffer] (empty/moved-from)\n"; return; }
        std::cout << "[RawBuffer] size=" << size_
                  << " data[0]=" << static_cast<int>(data_[0]) << "\n";
    }
};

void demo_rule_of_five() {
    std::cout << "\n=== Rule of Five Demo ===\n";

    RawBuffer a(64, 'A');  // Constructor
    a.print();

    RawBuffer b(a);        // Copy constructor → deep copy
    b.print();

    RawBuffer c(128, 'C'); // Constructor
    c = a;                 // Copy assignment → deep copy
    c.print();

    RawBuffer d(std::move(a));  // Move constructor → a bị rỗng
    d.print();
    a.print();  // a bây giờ là moved-from state

    RawBuffer e(32, 'E');
    e = std::move(b);      // Move assignment → b bị rỗng
    e.print();
    b.print();  // b bây giờ là moved-from state
}

// ─── Ví dụ 2: Rule of Zero — dùng RAII types → không cần viết gì ─────────
// Nếu class chỉ dùng std::string, std::vector, etc. thì compiler
// tự tạo ra các special member functions đúng → Rule of Zero
class SensorConfig {
public:
    std::string name;       // std::string tự lo copy/move
    std::vector<int> ports; // std::vector tự lo copy/move

    SensorConfig(std::string n, std::vector<int> p)
        : name(std::move(n)), ports(std::move(p)) {}
    // Không cần viết destructor, copy/move ctor/assign → compiler tự generate
};

void demo_rule_of_zero() {
    std::cout << "\n=== Rule of Zero Demo ===\n";
    SensorConfig cfg{"lidar", {0, 1, 2}};
    SensorConfig cfg2 = cfg;          // copy (compiler-generated)
    SensorConfig cfg3 = std::move(cfg); // move (compiler-generated)
    std::cout << "cfg2.name=" << cfg2.name << "\n";
    std::cout << "cfg3.name=" << cfg3.name << "\n";
    std::cout << "cfg.name (moved-from)='" << cfg.name << "'\n";
}

// ─── Ví dụ 3: Lỗi phổ biến — thiếu Rule of Five → double free ────────────
// Class này BAD: có destructor nhưng không viết copy ctor/assign
// → compiler tạo shallow copy → double free/dangling pointer
class BadBuffer {
public:
    int* data_;
    explicit BadBuffer(int val) : data_(new int(val)) {}
    ~BadBuffer() { delete data_; }  // có destructor
    // THIẾU: copy ctor, copy assign, move ctor, move assign
    // Compiler tạo shallow copy → disaster!
};

void demo_shallow_copy_problem() {
    std::cout << "\n=== Shallow Copy Problem (commented out to avoid crash) ===\n";
    std::cout << "BadBuffer b1(42);\n";
    std::cout << "BadBuffer b2 = b1;  // <-- shallow copy! b1.data_ == b2.data_\n";
    std::cout << "// When b2 destructs → delete data_\n";
    std::cout << "// When b1 destructs → delete data_ AGAIN → UNDEFINED BEHAVIOR!\n";

    // Đoạn code sau sẽ gây crash/undefined behavior:
    // BadBuffer b1(42);
    // BadBuffer b2 = b1;  // double free!
}

int main() {
    demo_rule_of_five();
    demo_rule_of_zero();
    demo_shallow_copy_problem();

    std::cout << "\n=== Destructors called in reverse order ===\n";
    return 0;
}
