# Singleton Pattern - Quick Reference

## Cách nhanh nhất: Meyers Singleton ⭐

```cpp
class MyClass {
private:
    MyClass() { }
public:
    static MyClass& getInstance() {
        static MyClass instance;
        return instance;
    }
};

// Sử dụng:
MyClass::getInstance().doSomething();
```

**Tại sao?**
- Thread-safe tự động (C++11+)
- Không memory leak
- Code đơn giản nhất
- Tự động cleanup

---

## Nếu cần reusable cho nhiều class: Template Singleton

```cpp
#include "Singleton.hpp"

class MyLogger : public Singleton<MyLogger> {
private:
    MyLogger() { }
    friend class Singleton<MyLogger>;
public:
    void log(string msg) { }
};

// Sử dụng:
MyLogger::getInstance().log("test");
```

---

## Điểm cần nhớ

| Đặc điểm | Nên làm | Không nên làm |
|---------|--------|---------------|
| **Constructor** | Private | Public |
| **Copy** | = delete | Cho phép |
| **getInstance** | Trả về reference | Trả về pointer |
| **Static variable** | Dùng | Tránh |
| **Thread-safety** | Meyers hoặc mutex | Basic pattern |
| **Memory** | Tự động | Manual new/delete |

---

## Các cách làm (từ tệ → tốt)

```cpp
// ❌ TỆtoday: Global variable
MyClass globalObj;

// ⚠️ Ổn nhưng cũ: Basic Singleton
static MyClass* instance = nullptr;
if (instance == nullptr) {
    instance = new MyClass();
}
return instance;

// ✅ TỐT: Meyers Singleton
static MyClass instance;
return instance;

// ✅✅ TỐT HƠN: Với abstract base class (dễ test)
class ILogger { virtual void log(string) = 0; };
class Logger : public ILogger, public Singleton<Logger> { };
```

---

## Ví dụ thực tế

```cpp
// Application configuration
class AppConfig : public Singleton<AppConfig> {
private:
    AppConfig() { }
    friend class Singleton<AppConfig>;
    string dbHost = "localhost";
public:
    string getDbHost() { return dbHost; }
};

// Logging
class Logger : public Singleton<Logger> {
private:
    Logger() { }
    friend class Singleton<Logger>;
public:
    void log(string msg) {
        cout << "[LOG] " << msg << endl;
    }
};

// Usage in your service
class UserService {
    void createUser(string name) {
        Logger::getInstance().log("Creating: " + name);
        string db = AppConfig::getInstance().getDbHost();
    }
};
```

---

## Khi nào dùng Singleton

✅ **Hợp lý:**
- Configuration/Settings
- Logger
- Database pool
- Thread pool
- Cache
- Application state

❌ **Không hợp lý:**
- Business logic
- Services (multiple instances)
- Repository (multiple per entity)
- Khi cần test dễ dàng
- Khi cần flexibility

---

## Chạy các ví dụ

```bash
cd /home/tvn/code_ws/Cpp/Pattern/Singleton/build

# Chạy riêng
./03_meyers_singleton          # Recommended!
./05_practical_example         # Real-world usage
./07_template_singleton        # Reusable template

# Chạy tất cả
make run_all
```

---

## File structure

```
Singleton/
├── 01_basic_singleton.cpp             # Cơ bản (không an toàn)
├── 02_thread_safe_singleton.cpp       # Với mutex (cũ)
├── 03_meyers_singleton.cpp            # ⭐ Khuyến nghị
├── 04_shared_singleton.cpp            # Với smart pointer
├── 05_practical_example.cpp           # Ví dụ thực tế
├── 06_comparison_and_antipatterns.cpp # So sánh cách làm
├── 07_template_singleton.cpp          # Template (reusable)
├── Singleton.hpp                      # Template header
├── CMakeLists.txt                     # Build script
└── README.md                          # Tài liệu chi tiết
```

---

## Lỗi thường gặp

```cpp
// ❌ LỖI: Quên delete static pointer
static MyClass* instance = nullptr;
// -> Memory leak!

// ✅ ĐÚNG: Dùng static reference
static MyClass& getInstance() {
    static MyClass instance;  // Auto cleanup
    return instance;
}

// ❌ LỖI: Cho phép copy
MyClass obj1 = MyClass::getInstance();  // Compile error!

// ✅ ĐÚNG: Delete copy
MyClass(const MyClass&) = delete;
MyClass& operator=(const MyClass&) = delete;
```

---

## So sánh nhanh (Quick Table)

```
Meyers        vs  Double-Check  vs  shared_ptr
=========================================
⭐ Đơn giản   vs  ⚠️ Phức tạp    vs  ⭐ Tốt
✅ An toàn    vs  ✅ An toàn     vs  ✅ An toàn
✅ Auto       vs  ❌ Manual      vs  ✅ Auto RC
✅ C++11+     vs  ⚠️ C++11+      vs  ✅ C++11+
✅✅ KHU. DÙNG vs  ⚠️ Khi cần    vs  ✅ Dùng nếu cần RC
```
