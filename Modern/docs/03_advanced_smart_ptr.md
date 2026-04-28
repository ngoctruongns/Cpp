# Advanced Smart Pointers: Custom Deleter, enable_shared_from_this, Perfect Forwarding

> **Bài tập liên quan:** Phase1 / 06, 07, 08, 09, 10

---

## 1. Custom Deleter

Dùng khi resource không thể `delete` thông thường: file handle, C API, hardware resource.

### Với `unique_ptr`

```cpp
// Deleter là type parameter thứ 2
using FilePtr = std::unique_ptr<FILE, decltype(&fclose)>;
FilePtr f(fopen("log.txt", "w"), &fclose);  // tự gọi fclose khi out of scope

// Lambda deleter
auto deleter = [](int* p) {
    std::cout << "custom delete\n";
    delete p;
};
std::unique_ptr<int, decltype(deleter)> p(new int(42), deleter);

// Hardware resource
struct GpioDeleter {
    void operator()(GpioHandle* h) { gpio_close(h); }
};
std::unique_ptr<GpioHandle, GpioDeleter> gpio(gpio_open(PIN));
```

### Với `shared_ptr`

```cpp
// shared_ptr nhận deleter ở constructor, không cần type param
auto sp = std::shared_ptr<FILE>(fopen("log.txt", "w"), &fclose);

// Lambda
auto sp2 = std::shared_ptr<int>(new int(42), [](int* p) {
    std::cout << "shared_ptr custom delete\n";
    delete p;
});
```

---

## 2. `enable_shared_from_this`

**Vấn đề:** Bên trong method của class, muốn lấy `shared_ptr` trỏ vào `this` mà không tạo ra shared_ptr mới (sẽ có ref count riêng → double-free).

```cpp
// ❌ SAI: tạo shared_ptr mới từ this → 2 ref count độc lập → double-free
class Node {
    void subscribe() {
        auto sp = std::shared_ptr<Node>(this);  // NGUY HIỂM!
    }
};

// ✅ ĐÚNG: kế thừa enable_shared_from_this
class Node : public std::enable_shared_from_this<Node> {
public:
    void subscribe() {
        auto sp = shared_from_this();  // lấy shared_ptr hợp lệ, cùng ref count
        event_bus.subscribe(sp);
    }
};

// Điều kiện bắt buộc: object phải đã được quản lý bởi shared_ptr khi gọi
auto node = std::make_shared<Node>();
node->subscribe();  // OK

// Node node_stack;
// node_stack.subscribe();  // ❌ UB: chưa có shared_ptr quản lý node_stack
```

**Ứng dụng ROS2:** `rclcpp::Node` kế thừa `enable_shared_from_this` — đây là lý do code ROS2 luôn dùng `make_shared<MyNode>()`.

---

## 3. Perfect Forwarding (`std::forward`)

**Vấn đề:** Universal reference (`T&&`) nhận được cả lvalue lẫn rvalue, nhưng khi forward vào hàm khác, nó trở thành lvalue (vì đã có tên). `std::forward` khôi phục value category gốc.

```cpp
// Không có perfect forwarding: mất rvalue-ness
template<typename T>
void wrapper_bad(T&& arg) {
    process(arg);             // arg là lvalue ở đây dù T có thể là rvalue ref
}

// Với std::forward: giữ nguyên value category
template<typename T>
void wrapper(T&& arg) {
    process(std::forward<T>(arg));  // nếu T = int& → forward lvalue
                                     // nếu T = int  → forward rvalue
}
```

### Universal Reference vs Rvalue Reference

```cpp
template<typename T>
void f(T&& x);     // Universal reference (T được deduced) — nhận cả hai

void g(int&& x);   // Rvalue reference thuần (không deduced) — chỉ nhận rvalue
```

### Ứng dụng: make_unique tự viết

```cpp
template<typename T, typename... Args>
std::unique_ptr<T> my_make_unique(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}
```

---

## 4. Smart Pointers trong Containers

```cpp
// Vector of unique_ptr — không thể copy vector
std::vector<std::unique_ptr<Sensor>> sensors;
sensors.push_back(std::make_unique<LidarSensor>("front"));
sensors.emplace_back(std::make_unique<IMUSensor>("imu"));

// Map
std::map<std::string, std::unique_ptr<Sensor>> sensor_map;
sensor_map["lidar"] = std::make_unique<LidarSensor>("lidar");

// Vector of shared_ptr — có thể copy
std::vector<std::shared_ptr<Node>> nodes;
auto n = std::make_shared<Node>("n1");
nodes.push_back(n);   // ref count tăng
nodes.push_back(n);   // cùng object, count = 3
```

---

## 5. Robot Resource Manager Pattern

Kết hợp tất cả:

```cpp
class RobotResourceManager {
    std::map<std::string, std::unique_ptr<Sensor>>  sensors_;   // exclusive
    std::vector<std::shared_ptr<DataProcessor>>     procs_;     // shared
    std::weak_ptr<EventBus>                         event_bus_; // non-owning

public:
    template<typename T, typename... Args>
    T* add_sensor(const std::string& name, Args&&... args) {
        auto s = std::make_unique<T>(std::forward<Args>(args)...);
        auto* ptr = s.get();  // raw pointer trước khi move
        sensors_[name] = std::move(s);
        return ptr;
    }
};
```

---

## 6. Tóm tắt

| Kỹ thuật | Dùng khi |
|----------|----------|
| Custom deleter | Resource không dùng `delete` (C API, hardware) |
| `enable_shared_from_this` | Cần `shared_ptr<this>` bên trong method |
| `std::forward<T>` | Wrapper template cần forward arg mà không mất value category |
| Smart ptr in container | Luôn dùng, tránh raw pointer trong container |
