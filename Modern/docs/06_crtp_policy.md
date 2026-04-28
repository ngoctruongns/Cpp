# CRTP & Policy-Based Design

> **Bài tập liên quan:** Phase2 / 08, 09, 10

---

## 1. CRTP — Curiously Recurring Template Pattern

**Ý tưởng:** Base class nhận Derived class làm template argument → có thể gọi method của Derived mà **không cần virtual** (static polymorphism).

```cpp
template<typename Derived>
class Base {
public:
    void interface() {
        // Downcast to Derived — compile-time, không có vtable
        static_cast<Derived*>(this)->implementation();
    }
};

class MyClass : public Base<MyClass> {
public:
    void implementation() {
        std::cout << "MyClass impl\n";
    }
};

MyClass obj;
obj.interface();  // gọi implementation() mà không cần virtual
```

---

## 2. CRTP vs Virtual — So sánh

```cpp
// Virtual: dynamic dispatch (vtable lookup)
class VirtualBase {
public:
    virtual void speak() = 0;
    void speak_twice() { speak(); speak(); }  // 2 vtable lookups
    virtual ~VirtualBase() = default;
};

// CRTP: static dispatch (inline, zero overhead)
template<typename D>
class CRTPBase {
public:
    void speak() { static_cast<D*>(this)->speak_impl(); }
    void speak_twice() {
        static_cast<D*>(this)->speak_impl();  // inlined at compile time
        static_cast<D*>(this)->speak_impl();
    }
};

class Dog : public CRTPBase<Dog> {
public:
    void speak_impl() { std::cout << "Woof\n"; }
};
```

| | Virtual | CRTP |
|-|---------|------|
| Dispatch | Runtime (vtable) | Compile-time |
| Overhead | vtable pointer, indirect call | Zero (inline) |
| Polymorphism | Runtime (base ptr) | Compile-time only |
| Dùng khi | Cần runtime switching | Hiệu năng cao, type biết tại compile |

**CRTP không thể:** Lưu `CRTPBase<?>*` trong một container chung (vì mỗi Derived là type khác nhau).

---

## 3. CRTP cho Mixin — Thêm behavior vào class

```cpp
// Mixin: thêm serialization
template<typename Derived>
class Serializable {
public:
    std::string to_json() const {
        return static_cast<const Derived*>(this)->serialize_impl();
    }
};

// Mixin: thêm comparison operators từ một operator==
template<typename Derived>
class EqualityComparable {
public:
    bool operator!=(const Derived& other) const {
        return !static_cast<const Derived*>(this)->operator==(other);
    }
};

// Class kết hợp nhiều mixin
class Point
    : public Serializable<Point>
    , public EqualityComparable<Point>
{
    int x_, y_;
public:
    bool operator==(const Point& o) const { return x_ == o.x_ && y_ == o.y_; }
    std::string serialize_impl() const {
        return "{\"x\":" + std::to_string(x_) + ",\"y\":" + std::to_string(y_) + "}";
    }
};
```

---

## 4. CRTP cho Static Interface (Concept enforcement)

```cpp
// Đảm bảo Derived implement đúng interface tại compile time
template<typename Derived>
class SensorInterface {
public:
    double read() { return static_cast<Derived*>(this)->read_impl(); }
    std::string name() { return static_cast<Derived*>(this)->name_impl(); }

protected:
    // Trick: kiểm tra method tồn tại trong constructor
    SensorInterface() {
        (void)static_cast<double(Derived::*)()>(&Derived::read_impl);  // compile error nếu thiếu
    }
};
```

---

## 5. Policy-Based Design

**Ý tưởng:** Tách behaviour ra thành các **Policy class** độc lập, inject vào class chính qua template parameter. Kết quả: cấu hình linh hoạt tại compile time, không overhead runtime.

```cpp
// Policies
struct ConsoleOutput {
    static void write(const std::string& msg) { std::cout << msg << "\n"; }
};

struct FileOutput {
    static void write(const std::string& msg) {
        std::ofstream f("log.txt", std::ios::app);
        f << msg << "\n";
    }
};

struct NoTimestamp {
    static std::string decorate(const std::string& msg) { return msg; }
};

struct WithTimestamp {
    static std::string decorate(const std::string& msg) {
        auto now = std::chrono::system_clock::now();
        // ... format time
        return "[timestamp] " + msg;
    }
};

// Logger kết hợp các policies
template<
    typename OutputPolicy  = ConsoleOutput,
    typename FormatPolicy  = NoTimestamp
>
class Logger {
public:
    void log(const std::string& msg) {
        OutputPolicy::write(FormatPolicy::decorate(msg));
    }
};

// Sử dụng — khác nhau tại compile time, không tốn runtime overhead
Logger<>                              basic;     // console, no timestamp
Logger<FileOutput>                    to_file;   // file, no timestamp
Logger<ConsoleOutput, WithTimestamp>  with_ts;   // console, with timestamp
```

---

## 6. Generic Robot Message (CRTP + Policies kết hợp)

```cpp
// Policy: serialization format
struct JsonFormat {
    template<typename T>
    static std::string serialize(const T& msg);
};

struct BinaryFormat {
    template<typename T>
    static std::vector<uint8_t> serialize(const T& msg);
};

// CRTP base với policy
template<typename Derived, typename SerializePolicy = JsonFormat>
class RobotMessage {
public:
    auto serialize() const {
        return SerializePolicy::serialize(*static_cast<const Derived*>(this));
    }
    uint64_t timestamp() const { return timestamp_ns_; }
private:
    uint64_t timestamp_ns_{0};
};

// Concrete message
struct OdomMsg : public RobotMessage<OdomMsg> {
    double x, y, theta;
};
```

---

## 7. Tóm tắt

```
CRTP:          Static polymorphism, zero overhead, mixin pattern
Policy-based:  Behavior injection qua template param, compile-time config
Kết hợp:       CRTP cho interface + Policy cho behavior → flexible, zero-cost
```
