# Template Constraints: Specialization, SFINAE, if constexpr, Type Traits, Concepts

> **Bài tập liên quan:** Phase2 / 03, 04, 05, 06, 07

---

## 1. Template Specialization

### Full Specialization — một type cụ thể

```cpp
// Primary template
template<typename T>
struct Serializer {
    static std::string to_string(const T& v) {
        return std::to_string(v);
    }
};

// Full specialization cho std::string
template<>
struct Serializer<std::string> {
    static std::string to_string(const std::string& v) {
        return "\"" + v + "\"";  // wrap trong quotes
    }
};
```

### Partial Specialization — một pattern type

```cpp
// Specialization cho pointer types
template<typename T>
struct Serializer<T*> {
    static std::string to_string(const T* p) {
        return p ? "ptr:" + std::to_string(*p) : "null";
    }
};
```

---

## 2. SFINAE — Substitution Failure Is Not An Error

Khi compiler thử substitute type vào template mà thất bại, nó **không báo lỗi** mà bỏ qua overload đó.

`std::enable_if<condition, ReturnType>` tạo ra lỗi substitution nếu condition = false.

```cpp
// Chỉ enable khi T là integer
template<typename T>
std::enable_if_t<std::is_integral_v<T>, std::string>
describe(T val) { return "int: " + std::to_string(val); }

// Chỉ enable khi T là floating point
template<typename T>
std::enable_if_t<std::is_floating_point_v<T>, std::string>
describe(T val) { return "float: " + std::to_string(val); }

describe(42);    // → "int: 42"
describe(3.14);  // → "float: 3.14"
```

**SFINAE trong template parameter (phổ biến hơn):**

```cpp
template<typename T, std::enable_if_t<std::is_integral_v<T>, int> = 0>
void process(T val) { /* integer path */ }
```

---

## 3. `if constexpr` (C++17) — Cách hiện đại thay SFINAE

Compile-time branching bên trong một hàm template duy nhất. Code không được chọn **không được compile** → không lỗi.

```cpp
template<typename T>
std::string describe(const T& val) {
    if constexpr (std::is_integral_v<T>) {
        return "int: " + std::to_string(val);
    } else if constexpr (std::is_floating_point_v<T>) {
        return "float: " + std::to_string(val);
    } else if constexpr (std::is_same_v<T, std::string>) {
        return "string: " + val;
    } else {
        static_assert(false, "Unsupported type");  // compile error nếu không match
    }
}
```

**So sánh SFINAE vs if constexpr:**

| | SFINAE + enable_if | if constexpr |
|-|-------------------|--------------|
| C++ version | C++11 | C++17 |
| Dễ đọc | ❌ Verbose | ✅ Rõ ràng |
| Nhiều overload | Cần nhiều hàm | Một hàm duy nhất |
| Khi nào SFINAE vẫn cần | Khi cần loại overload khỏi set hoàn toàn |

---

## 4. Type Traits

`<type_traits>` cung cấp compile-time thông tin về type.

```cpp
// Kiểm tra type category
std::is_integral_v<T>           // int, char, bool, ...
std::is_floating_point_v<T>     // float, double
std::is_pointer_v<T>
std::is_reference_v<T>
std::is_class_v<T>
std::is_enum_v<T>
std::is_same_v<T, U>            // T và U là cùng type?
std::is_base_of_v<Base, Derived>
std::is_convertible_v<From, To>

// Type transformation
std::remove_const_t<const int>   // → int
std::remove_reference_t<int&>    // → int
std::remove_cv_t<const volatile int>  // → int
std::add_pointer_t<int>          // → int*
std::decay_t<int[]>              // → int* (array → pointer decay)
std::underlying_type_t<MyEnum>   // → int (hoặc type underlying enum)
```

### Custom type trait

```cpp
// Kiểm tra T có method .serialize() không
template<typename T, typename = void>
struct has_serialize : std::false_type {};

template<typename T>
struct has_serialize<T, std::void_t<decltype(std::declval<T>().serialize())>>
    : std::true_type {};

template<typename T>
constexpr bool has_serialize_v = has_serialize<T>::value;
```

---

## 5. Concepts (C++20)

Concepts đặt tên cho constraints, lỗi compiler rõ ràng hơn SFINAE.

```cpp
#include <concepts>

// Định nghĩa concept
template<typename T>
concept Numeric = std::is_arithmetic_v<T>;

template<typename T>
concept Serializable = requires(T t) {
    { t.serialize() } -> std::convertible_to<std::string>;
};

// Dùng concept
template<Numeric T>
T add(T a, T b) { return a + b; }

// Hoặc requires clause
template<typename T>
    requires Serializable<T>
void save(const T& obj) { save_to_file(obj.serialize()); }

// Hoặc abbreviated (gọn nhất)
void save(Serializable auto& obj) { save_to_file(obj.serialize()); }
```

**Lỗi compiler với Concepts rõ ràng hơn SFINAE nhiều:**
- SFINAE: *"no matching function for call to..."* (dài, khó đọc)
- Concepts: *"constraint 'Numeric' was not satisfied"* (rõ ràng)

---

## 6. Tóm tắt — Khi nào dùng gì?

```
C++11:  SFINAE + enable_if       (vẫn gặp trong codebase cũ)
C++17:  if constexpr             (ưu tiên trong hàm template)
C++17:  type traits              (cả C++11 trở lên)
C++20:  Concepts                 (ưu tiên khi dùng C++20)
```

**Thứ tự ưu tiên (hiện đại nhất):**
1. Concepts (C++20) — rõ nhất
2. `if constexpr` (C++17) — đủ cho hầu hết cases
3. `enable_if` / SFINAE — khi cần backward compat
