# Variadic Templates & Fold Expressions

> **Bài tập liên quan:** Phase2 / 01, 02

---

## 1. Variadic Templates

Cho phép hàm/class nhận **số lượng type argument tùy ý**.

```cpp
template<typename... Args>   // Args là parameter pack
void print(Args... args);    // args là pack expansion
```

### Cú pháp quan trọng

| Cú pháp | Ý nghĩa |
|---------|---------|
| `typename... Args` | Khai báo type pack |
| `Args...` | Expand pack thành danh sách type |
| `args...` | Expand pack thành danh sách value |
| `sizeof...(Args)` | Số lượng phần tử trong pack |

---

## 2. Đệ quy Template (C++11/14)

Cách cổ điển: tách phần tử đầu, đệ quy cho phần còn lại.

```cpp
// Base case (pack rỗng)
void print() {}

// Recursive case
template<typename T, typename... Rest>
void print(T first, Rest... rest) {
    std::cout << first << " ";
    print(rest...);    // đệ quy với rest
}

print(1, "hello", 3.14);  // output: 1 hello 3.14
```

**Nhược điểm:** Nhiều instantiation → compile time dài, code phình.

---

## 3. Fold Expressions (C++17) — Cách hiện đại

Áp dụng operator cho toàn bộ pack trong một biểu thức.

```
Unary right fold:  (pack op ...)          → (a op (b op c))
Unary left fold:   (... op pack)          → ((a op b) op c)
Binary right fold: (pack op ... op init)  → (a op (b op (c op init)))
Binary left fold:  (init op ... op pack)  → (((init op a) op b) op c)
```

```cpp
// Sum
template<typename... Args>
auto sum(Args... args) {
    return (args + ...);          // unary right fold
}
sum(1, 2, 3, 4);  // = 1 + (2 + (3 + 4)) = 10

// Print (dùng comma operator)
template<typename... Args>
void print_all(Args&&... args) {
    ((std::cout << args << ' '), ...);  // left fold với ,
    std::cout << '\n';
}

// Kiểm tra tất cả điều kiện (AND)
template<typename... Bools>
bool all_true(Bools... bs) {
    return (... && bs);  // left fold
}

// Kiểm tra ít nhất một (OR)
template<typename... Bools>
bool any_true(Bools... bs) {
    return (... || bs);
}
```

---

## 4. Ứng dụng: Type-safe Logger

```cpp
enum class LogLevel { DEBUG, INFO, WARN, ERROR };

template<typename... Args>
void log(LogLevel level, Args&&... args) {
    const char* lvl[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    std::cout << "[" << lvl[int(level)] << "] ";
    (std::cout << ... << std::forward<Args>(args));  // binary left fold
    std::cout << '\n';
}

log(LogLevel::INFO, "Sensor ", 42, " connected at t=", 3.14);
// [INFO] Sensor 42 connected at t=3.14
```

---

## 5. Variadic Class Template

```cpp
// Tuple-like: lưu nhiều type khác nhau
template<typename... Types>
struct TypeList {};

using MyTypes = TypeList<int, double, std::string>;

// Visitor pattern
template<typename Visitor, typename... Types>
void visit_all(Visitor&& v, Types&&... items) {
    (v(std::forward<Types>(items)), ...);
}
```

---

## 6. `sizeof...` — Đếm phần tử trong pack

```cpp
template<typename... Args>
void show_count(Args... args) {
    std::cout << "Count: " << sizeof...(Args) << "\n";  // type pack
    std::cout << "Count: " << sizeof...(args) << "\n";  // value pack
}
show_count(1, "hello", 3.14);  // Count: 3
```

---

## 7. Tóm tắt

```
C++11/14:  Đệ quy template (verbose nhưng linh hoạt)
C++17:     Fold expressions (gọn, ưu tiên dùng)

(args + ...)      — sum
((os << args), ...) — print all
(... && conds)    — all conditions true
```
