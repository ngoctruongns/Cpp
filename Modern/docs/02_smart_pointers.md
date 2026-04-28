# Smart Pointers

> **Bài tập liên quan:** Phase1 / 03, 04, 05

---

## 1. Tại sao cần Smart Pointer?

Raw pointer buộc phải `delete` thủ công → dễ leak, double-free, dangling pointer.
Smart pointer dùng **RAII**: resource gắn với lifetime của object, tự dọn khi ra scope.

```cpp
// Cách cũ — nguy hiểm
Sensor* s = new LidarSensor("lidar");
// nếu throw exception ở đây → leak!
delete s;

// Smart pointer — an toàn
auto s = std::make_unique<LidarSensor>("lidar");
// tự động delete khi ra scope, kể cả khi có exception
```

---

## 2. `std::unique_ptr` — Exclusive Ownership

Chỉ **một** owner tại một thời điểm. Không copy, chỉ move.

```cpp
#include <memory>

// Tạo
auto p = std::make_unique<MyClass>(args...);  // ← luôn dùng make_unique

// Truy cập
p->method();   // như raw pointer
(*p).field;

// Transfer ownership
auto p2 = std::move(p);  // p == nullptr sau đây
// hoặc truyền vào hàm
void take(std::unique_ptr<MyClass> ptr);
take(std::move(p));

// Release raw pointer (sau đây ta phải tự delete!)
MyClass* raw = p.release();

// Reset (destroy object hiện tại, nhận object mới hoặc nullptr)
p.reset(new MyClass{});
p.reset();   // destroy và set nullptr
```

### Dùng trong factory / containers

```cpp
// Factory
std::unique_ptr<Sensor> make_sensor(const std::string& type) {
    if (type == "lidar") return std::make_unique<LidarSensor>("lidar");
    if (type == "imu")   return std::make_unique<IMUSensor>("imu");
    return nullptr;
}

// Container
std::vector<std::unique_ptr<Sensor>> sensors;
sensors.push_back(std::make_unique<LidarSensor>("front"));
sensors.push_back(std::make_unique<IMUSensor>("body"));
for (const auto& s : sensors) {
    std::cout << s->read() << "\n";
}
```

### Dùng với base class (polymorphism)

```cpp
std::unique_ptr<Base> ptr = std::make_unique<Derived>();
ptr->virtual_method();  // OK: dynamic dispatch
```

---

## 3. `std::shared_ptr` — Shared Ownership

Nhiều owner có thể chia sẻ cùng một object. Dùng **reference counting**: khi count = 0, object bị destroy.

```cpp
auto sp1 = std::make_shared<SensorBuffer>("imu");
std::cout << sp1.use_count();  // 1

{
    auto sp2 = sp1;              // copy → count = 2
    auto sp3 = sp1;              // copy → count = 3
    std::cout << sp1.use_count(); // 3
}  // sp2, sp3 hết scope → count = 1

// sp1 hết scope → count = 0 → object bị destroy
```

### Khi nào dùng shared_ptr thay unique_ptr?

| Tình huống | Dùng |
|------------|------|
| Một owner duy nhất | `unique_ptr` |
| Chia sẻ giữa nhiều object (vd: nhiều ROS2 node cùng đọc sensor) | `shared_ptr` |
| Observer cần tham chiếu mà không muốn ownership | `weak_ptr` |

---

## 4. `std::weak_ptr` — Non-owning Reference

`weak_ptr` **không tăng** reference count. Dùng để:
- Tránh **circular reference** (vòng tròn → memory leak)
- Observer theo dõi object mà không giữ nó sống

```cpp
std::weak_ptr<Node> weak = shared_ptr_to_node;

// Phải lock() trước khi dùng
if (auto sp = weak.lock()) {   // sp là shared_ptr, valid nếu object còn sống
    sp->do_something();
} else {
    // object đã bị destroy
}
```

---

## 5. Circular Reference & Cách Sửa

```cpp
// ❌ VẤN ĐỀ: circular reference → memory leak
struct Node {
    std::shared_ptr<Node> next;   // A → B
    std::shared_ptr<Node> prev;   // B → A  ← vòng tròn!
};

// ✅ SỬA: dùng weak_ptr cho chiều ngược
struct Node {
    std::shared_ptr<Node> next;   // forward link — có ownership
    std::weak_ptr<Node>   prev;   // backward link — không có ownership
};
```

**Nguyên tắc:** Trong quan hệ parent-child, parent dùng `shared_ptr` đến child, child dùng `weak_ptr` ngược lại parent.

---

## 6. So sánh nhanh

| | `unique_ptr` | `shared_ptr` | `weak_ptr` |
|-|-------------|-------------|------------|
| Ownership | Exclusive (1 owner) | Shared (nhiều owner) | Không có |
| Copy | ❌ | ✅ (tăng count) | ✅ |
| Move | ✅ | ✅ | ✅ |
| Overhead | Zero | Ref count (heap alloc) | Minimal |
| Dùng khi | Mặc định | Shared lifetime | Observer, break cycle |

---

## 7. Pitfalls thường gặp

```cpp
// ❌ Đừng tạo shared_ptr từ raw pointer đã có shared_ptr khác
int* raw = new int(42);
auto sp1 = std::shared_ptr<int>(raw);
auto sp2 = std::shared_ptr<int>(raw);  // DOUBLE FREE!

// ✅ Luôn dùng make_shared / make_unique
auto sp = std::make_shared<int>(42);

// ❌ Đừng trả unique_ptr bằng std::move nếu không cần
// (NRVO sẽ tự move cho bạn)
auto make() {
    auto p = std::make_unique<Foo>();
    return p;  // OK, không cần std::move
}
```
