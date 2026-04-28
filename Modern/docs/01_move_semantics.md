# Move Semantics & Rule of Five

> **Bài tập liên quan:** Phase1 / 01, 02

---

## 1. lvalue vs rvalue

| | lvalue | rvalue |
|--|--------|--------|
| Định nghĩa | Có địa chỉ, tên xác định | Tạm thời, không có tên |
| Ví dụ | `int a = 5;` → `a` là lvalue | `5`, `a + b`, `func()` là rvalue |
| Bind với | `T&` (lvalue ref) | `T&&` (rvalue ref) |

```cpp
int a = 10;
int& ra  = a;    // OK: lvalue ref → lvalue
int&& rr = 20;   // OK: rvalue ref → rvalue (temporary)
// int& rb = 10; // ERROR: lvalue ref không bind rvalue
// int&& r2 = a; // ERROR: rvalue ref không bind lvalue
```

---

## 2. std::move

`std::move` **không di chuyển** gì cả — nó chỉ **cast lvalue thành rvalue reference** để kích hoạt move constructor/assignment.

```cpp
std::vector<int> a = {1, 2, 3};
std::vector<int> b = std::move(a);  // b "ăn cắp" data của a
// a giờ ở trạng thái valid nhưng unspecified (thường rỗng)
```

**Quy tắc:** Sau khi `std::move(x)`, đừng dùng `x` nữa (trừ khi gán lại).

---

## 3. Move Constructor & Move Assignment

```cpp
class Buffer {
    int* data_; size_t size_;
public:
    // Move constructor: nhận rvalue ref, "ăn cắp" resource
    Buffer(Buffer&& other) noexcept
        : data_(other.data_), size_(other.size_)
    {
        other.data_ = nullptr;  // ← nullify nguồn để tránh double-free
        other.size_ = 0;
    }

    // Move assignment
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            delete[] data_;          // giải phóng resource hiện tại
            data_ = other.data_;
            size_ = other.size_;
            other.data_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }
};
```

**Quan trọng:** Luôn đánh dấu `noexcept` cho move operations — STL containers chỉ dùng move thay copy khi move được đảm bảo `noexcept`.

---

## 4. Rule of Five

Khi class **tự quản lý resource** (raw pointer, file handle, socket...), phải tự định nghĩa đủ 5 special functions:

| # | Function | Mục đích |
|---|----------|----------|
| 1 | Destructor | Giải phóng resource |
| 2 | Copy constructor | Deep copy |
| 3 | Copy assignment | Deep copy + self-assignment guard |
| 4 | Move constructor | Chuyển ownership, nullify nguồn |
| 5 | Move assignment | Giải phóng cũ, chuyển ownership |

```cpp
class Resource {
public:
    ~Resource();                              // 1
    Resource(const Resource&);               // 2
    Resource& operator=(const Resource&);    // 3
    Resource(Resource&&) noexcept;           // 4
    Resource& operator=(Resource&&) noexcept; // 5
};
```

### Rule of Zero (ưu tiên hơn)
Nếu dùng RAII wrappers (`std::vector`, `std::unique_ptr`...) để quản lý resource thì **không cần viết bất kỳ special function nào** — compiler tự generate đúng.

```cpp
class GoodClass {
    std::vector<int> data_;    // vector tự lo copy/move/destroy
    std::string name_;
    // Không cần viết gì thêm — Rule of Zero
};
```

---

## 5. Copy vs Move — Performance

```
Copy: O(n) — sao chép toàn bộ data
Move: O(1) — chỉ copy vài pointer/size
```

**Khi nào move được gọi tự động:**
- Return local variable (NRVO/RVO, hoặc move nếu không áp dụng)
- Truyền temporary vào hàm
- Push vào container: `vec.push_back(std::move(obj))`
- Khi `std::move()` được gọi tường minh

---

## 6. Tóm tắt nhanh

```cpp
T val;             // lvalue
T&  ref = val;     // lvalue reference
T&& rref = T{};    // rvalue reference

std::move(val)     // cast lvalue → rvalue (không di chuyển thật)
std::forward<T>(x) // perfect forwarding (xem doc 03)
```
